#define MAX_VERSION(a, b) ((a) > (b) ? (a) : (b))

void umDfsMarkObjectsUnMark(GC_state s, objptr *opp) {
    umDfsMarkObjects(s, opp, UNMARK_MODE);
}

void umDfsMarkObjectsMark(GC_state s, objptr *opp) {
    umDfsMarkObjects(s, opp, MARK_MODE);
}

bool isPointerMarked (pointer p) {
  return MARK_MASK & getHeader (p);
}

bool isPointerMarkedByMode (pointer p, GC_markMode m) {
  switch (m) {
  case MARK_MODE:
    return isPointerMarked (p);
  case UNMARK_MODE:
    return not isPointerMarked (p);
  default:
    die ("bad mark mode %u", m);
  }
}


void getObjectType(GC_state s, objptr *opp) {
    pointer p = objptrToPointer(*opp, s->heap.start);
    GC_header* headerp = getHeaderp(p);
    GC_header header = *headerp;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
    GC_objectTypeTag tag;
    splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

    if (DEBUG_MEM) {
        switch (tag) {
        case NORMAL_TAG:
            fprintf(stderr, "NORMAL!\n");
            if (p >= s->umheap.start &&
                p < s->umheap.start + s->umheap.size) {
                fprintf(stderr, "  ON UM HEAP: 0x%x!\n", p);
            } else {
                fprintf(stderr, "  NOT ON UM HEAP: 0x%x\n", p);
            }
            break;
        case WEAK_TAG:
            fprintf(stderr, "WEAK!\n");
            break;
        case ARRAY_TAG:
            fprintf(stderr, "ARRAY!\n");
            if (p >= s->tlsfarheap.start &&
                p < s->tlsfarheap.start + s->tlsfarheap.size) {
                fprintf(stderr, "  ON TLSF HEAP: 0x%x!\n", p);
            } else {
                fprintf(stderr, "  NOT ON TLSF HEAP: 0x%x\n", p);
            }
            break;
        case STACK_TAG:
            fprintf(stderr, "STACK: 0x%x\n", p);
            break;
        default:
            die("getObjetctType: swith: Shouldn't be here!\n");
        }
    }
}

void umDfsMarkObjects(GC_state s, objptr *opp, GC_markMode m) {
    pointer p = objptrToPointer(*opp, s->heap.start);
    if (DEBUG_MEM)
        fprintf(stderr, "original obj: 0x%x, obj: 0x%x\n",
                (uintptr_t)*opp, (uintptr_t)p);
    GC_header* headerp = getHeaderp(p);
    GC_header header = *headerp;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
    GC_objectTypeTag tag;
    splitHeader(s, header, &tag, NULL, &bytesNonObjptrs, &numObjptrs);

    /* if (tag == STACK_TAG) */
    /*     return; */

    //    if (DEBUG_MEM)


    /* Using MLton's header to track if it's marked */
    if (isPointerMarkedByMode(p, m)) {
        //        if (DEBUG_MEM)
        //        fprintf(stderr, "===== TYPE =====\n");
        getObjectType(s, opp);
        /* fprintf(stderr, FMTPTR" marked by mark_mode: %d, RETURN\n", */
        /*         (uintptr_t)p, */
        /*         (m == MARK_MODE)); */
        //        fprintf(stderr, "===== END TYPE =====\n");
        return;
    }

    getObjectType(s, opp);

    if (m == MARK_MODE) {
        if (DEBUG_MEM)
            fprintf(stderr, FMTPTR" mark b pheader: %x, header: %x\n",
                    (uintptr_t)p, *(getHeaderp(p)), header);

        header = header | MARK_MASK;
        *headerp = header;

        if (DEBUG_MEM)
            fprintf(stderr, FMTPTR" mark a pheader: %x, header: %x\n",
                    (uintptr_t)p, *(getHeaderp(p)), header);
    } else {
        if (DEBUG_MEM)
            fprintf(stderr, FMTPTR" unmark b pheader: %x, header: %x\n",
                    (uintptr_t)p, *(getHeaderp(p)), header);

        header = header & ~MARK_MASK;
        (*headerp) = header;

        if (DEBUG_MEM)
            fprintf(stderr, FMTPTR" unmark a pheader: %x, header: %x\n",
                    (uintptr_t)p, *(getHeaderp(p)), header);
    }

    if (tag == NORMAL_TAG) {
        if (p >= s->umheap.start &&
            p < (s->umheap.start + s->umheap.size)) {
            GC_UM_Chunk pchunk = (GC_UM_Chunk)(p - GC_NORMAL_HEADER_SIZE);
            pchunk->object_version = MAX_VERSION(s->gc_object_version, pchunk->object_version);

            if (DEBUG_MEM) {
                fprintf(stderr, "umDfsMarkObjects: chunk: "FMTPTR", sentinel: %d,"
                        " mark_mode: %d, objptrs: %d, version: %lld\n", (uintptr_t)pchunk,
                        pchunk->sentinel,
                        (m == MARK_MODE), numObjptrs, pchunk->object_version);
            }

            if (NULL != pchunk->next_chunk) {
                pchunk->next_chunk->object_version =
                    MAX_VERSION(s->gc_object_version,
                                pchunk->next_chunk->object_version);
            }
        }
    }

    if (tag == ARRAY_TAG && p >= s->tlsfarheap.start &&
        p < s->tlsfarheap.size + s->tlsfarheap.start) {
        GC_TLSF_array arrayHeader = (GC_TLSF_array)(p - sizeof(struct GC_TLSF_array));
        //        fprintf(stderr, "Array 0x%x, magic: %d\n", p, arrayHeader->magic);
        arrayHeader->object_version = MAX_VERSION(s->gc_object_version,
                                                  arrayHeader->object_version);
    }

    if (numObjptrs > 0 || tag == STACK_TAG) {
        if (m == MARK_MODE)
            foreachObjptrInObject(s, p, umDfsMarkObjectsMark, false);
        else
            foreachObjptrInObject(s, p, umDfsMarkObjectsUnMark, false);
    }
}

void markUMArrayChunks(GC_state s, GC_UM_Array_Chunk p, GC_markMode m) {
    if (DEBUG_MEM)
        fprintf(stderr, "markUMArrayChunks: %x: marking array markmode: %d, "
                "type: %d\n", p, m,
                p->array_chunk_type);

    if (m == MARK_MODE)
        p->array_chunk_header |= UM_CHUNK_HEADER_MASK;
    else
        p->array_chunk_header &= ~UM_CHUNK_HEADER_MASK;

    if (p->array_chunk_type == UM_CHUNK_ARRAY_INTERNAL) {
        int i = 0;
        for (i=0; i<UM_CHUNK_ARRAY_INTERNAL_POINTERS; i++) {
            GC_UM_Array_Chunk pcur = p->ml_array_payload.um_array_pointers[i];
            if (!pcur)
                break;
            markUMArrayChunks(s, pcur, m);
        }
    }
}
