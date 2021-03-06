
/*
hello.1.c:(.text+0xb8fa): undefined reference to `UM_Header_alloc'
hello.1.c:(.text+0xb912): undefined reference to `UM_Payload_alloc'
hello.1.c:(.text+0xb92d): undefined reference to `UM_CPointer_offset'
*/

#define DBG(x,y,z,m) fprintf (stderr, "%s:%d: %s("FMTPTR", %d, %d): %s\n", \
		__FILE__, __LINE__, __FUNCTION__, (uintptr_t)(x), (int)y, (int)z, m?m:"na")

/* define chunk structure (linked list)
 * define the free list
 */

/*
 * header
 * - 4 bytes MLton header (initialized in the ML code)
 * - 4 bytes next chunk pointer (initialize in the C code)
 */
Pointer
UM_Header_alloc(GC_state gc_stat,
                Pointer umfrontier,
                C_Size_t s)
{
    if (DEBUG_MEM)
        DBG(umfrontier, s, 0, "enter");

    return (umfrontier + s);
//    GC_UM_Chunk pchunk = (GC_UM_Chunk)umfrontier;
//    pchunk->chunk_header = UM_CHUNK_IN_USE;
//    GC_collect(gc_stat, 0, true);
//    GC_UM_Chunk pchunk = allocNextChunk(gc_stat, &(gc_stat->umheap));
//    return (umfrontier + s);
//	return (((Pointer)pchunk) + s);
}

Pointer
UM_Payload_alloc(GC_state gc_stat, Pointer umfrontier, C_Size_t s)
{
    if (DEBUG_MEM)
       DBG(umfrontier, s, 0, "enter");
    //    GC_collect(gc_stat, 0, false);
    //    GC_collect(gc_stat, 0, false);
    GC_UM_Chunk next_chunk = allocNextChunk(gc_stat, &(gc_stat->umheap));
    GC_UM_Chunk current_chunk = (GC_UM_Chunk) umfrontier;
    current_chunk->chunk_header= UM_CHUNK_IN_USE;

    if (DEBUG_MEM) {
        fprintf(stderr, "Sentinel: %d \n", current_chunk->sentinel);
        fprintf(stderr, "Nextchunk: "FMTPTR" \n", (uintptr_t) next_chunk);
    }
    current_chunk->next_chunk = NULL;

    if (s <= UM_CHUNK_PAYLOAD_SIZE) {
        if (DEBUG_MEM)
            DBG(umfrontier, s, 0, "move frontier to next chunk");
        return (Pointer)next_chunk->ml_object;
    }

    if (DEBUG_MEM)
       DBG(umfrontier, s, 0, "allocate next chunk");

    if (s > 2 * UM_CHUNK_PAYLOAD_SIZE) {
        die("BUG: Requiring allocation of more than 2 chunks\n");
    }
    /* Assume a maximum of 2 chunks
     * It can actually be 3 (fragmented chunks of LARGE objects)
     * TODO: Set header to represent chunked object
     */
    current_chunk->next_chunk = next_chunk;
/////////////////
    next_chunk->chunk_header = UM_CHUNK_IN_USE;
/////////////////
    GC_UM_Chunk next_chunk_next = allocNextChunk(gc_stat, &(gc_stat->umheap));
    next_chunk->next_chunk = NULL;

    if (DEBUG_MEM) {
        fprintf(stderr, "Next chunk next: place frontier at "FMTPTR"\n",
                (uintptr_t) next_chunk_next->ml_object);
    }

    return (Pointer) next_chunk_next->ml_object;
}


/*
 * calculate which chunk we need to look at
 *
 */
Pointer
UM_CPointer_offset(GC_state gc_stat, Pointer p, C_Size_t o, C_Size_t s)
{
    if (DEBUG_MEM)
       DBG(p, o, s, "enter");

    Pointer heap_end = (gc_stat->umheap).end;

    /* Not on our heap! */
    if (p < (gc_stat->umheap).start ||
        p >= heap_end) {
        if (DEBUG_MEM)
            DBG(p, o, s, "not UM Heap");
        return (p + o);
    }

    GC_UM_Chunk current_chunk = (GC_UM_Chunk) (p - 4);
    if (current_chunk->chunk_header == UM_CHUNK_HEADER_CLEAN)
        die("Visiting a chunk that is on free list!\n");

    /* On current chunk */
    /* TODO: currently 4 is hard-coded mlton's header size */
    if (o + s + 4 <= UM_CHUNK_PAYLOAD_SIZE) {
        if (DEBUG_MEM) {
            DBG(p, o, s, "current chunk");
            fprintf(stderr, " sentinel: %d, val: "FMTPTR"\n",
                    current_chunk->sentinel,
                    *(p + o));
        }
        return (p + o);
    }

    if (DEBUG_MEM)
       DBG(p, o, s, "go to next chunk");

    /* On next chunk */
    if (current_chunk->sentinel == UM_CHUNK_SENTINEL_UNUSED) {
        current_chunk->sentinel = o + 4;

        if (DEBUG_MEM) {
            fprintf(stderr, "Returning next chunk: "FMTPTR"\n",
                    (uintptr_t) current_chunk->next_chunk);
            fprintf(stderr, "Returning next chunk mlobject: "FMTPTR"\n",
                    (uintptr_t) current_chunk->next_chunk->ml_object);
        }

        if (current_chunk->next_chunk->chunk_header == UM_CHUNK_HEADER_CLEAN) {
            die("Next chunk on free list!\n");
        }

        return (Pointer)(current_chunk->next_chunk->ml_object);
    }

    if (DEBUG_MEM) {
        fprintf(stderr, "Multi-chunk: Go to next chunk: "FMTPTR", sentinel: %d\n",
                (uintptr_t) current_chunk->next_chunk,
                current_chunk->sentinel);
    }
    return (Pointer)(current_chunk->next_chunk + (o + 4) -
                     current_chunk->sentinel);
}

Pointer UM_Array_offset(GC_state gc_stat, Pointer base, C_Size_t index,
                        C_Size_t elemSize, C_Size_t offset) {
    Pointer heap_end = (gc_stat->umarheap).start + (gc_stat->umarheap).size;

    if (base < gc_stat->umarheap.start || base >= heap_end) {
        if (DEBUG_MEM) {
            fprintf(stderr, "UM_Array_offset: not current heap: "FMTPTR" offset: %d\n",
                    base + offset);
        }
        return base + index * elemSize + offset;
    }

    GC_UM_Array_Chunk fst_leaf = (GC_UM_Array_Chunk)
        (base - GC_HEADER_SIZE - GC_HEADER_SIZE);

    if (fst_leaf->array_num_chunks <= 1) {
        return ((Pointer)&(fst_leaf->ml_array_payload.ml_object[0])) + index * elemSize + offset;
    }

    GC_UM_Array_Chunk root = fst_leaf->root;

    if (DEBUG_MEM) {
        fprintf(stderr, "UM_Array_offset: "FMTPTR" root: "
                FMTPTR", index: %d size: %d offset %d, "
                " length: %d, chunk_num_objs: %d\n",
                (base - 8), root, index, elemSize, offset,
                fst_leaf->array_chunk_length,
                fst_leaf->array_chunk_numObjs);
    }

    size_t chunk_index = index / root->array_chunk_numObjs;
    GC_UM_Array_Chunk current = root;
    size_t i;

    if (DEBUG_MEM) {
        fprintf(stderr, " >> Start to fetch chunk index: %d\n", chunk_index);
    }

    while (true) {
        if (current->array_chunk_header == UM_CHUNK_HEADER_CLEAN)
            die("Visiting a chunk that is on free list!\n");

        i = chunk_index / current->array_chunk_fan_out;
        if (DEBUG_MEM) {
            fprintf(stderr, "  --> chunk_index: %d, current fan out: %d, "
                    "in chunk index: %d\n",
                    chunk_index,
                    current->array_chunk_fan_out,
                    i);
        }
        chunk_index = chunk_index % current->array_chunk_fan_out;
        current = current->ml_array_payload.um_array_pointers[i];
        if (current->array_chunk_type == UM_CHUNK_ARRAY_LEAF) {
            size_t chunk_offset = (index % root->array_chunk_numObjs) * elemSize + offset;
            Pointer res = ((Pointer)&(current->ml_array_payload.ml_object[0])) +
                chunk_offset;
            if (DEBUG_MEM) {
                fprintf(stderr,
                        " --> Chunk_addr: "FMTPTR", index: %d, chunk base: "FMTPTR", "
                        "offset: %d, addr "FMTPTR" word: %x, %d, "
                        " char: %c\n",
                        current,
                        index,
                        current->ml_array_payload.ml_object, chunk_offset, res,
                        *((Word32_t*)(res)),
                        *((Word32_t*)(res)),
                        *((char*)(res)));
            }
            return res;
        }
    }

    die("UM_Array_Offset: shouldn't be here!");
}
