pointer GC_arrayAllocate(GC_state s,
                         __attribute__((unused)) size_t ensureBytesFree,
                         GC_arrayLength numElements,
                         GC_header header)
{
    size_t bytesPerElement;
    uint16_t bytesNonObjptrs;
    uint16_t numObjptrs;
    splitHeader(s, header, NULL, NULL, &bytesNonObjptrs, &numObjptrs);
    bytesPerElement = bytesNonObjptrs + (numObjptrs * OBJPTR_SIZE);

    size_t arraySize = bytesPerElement * numElements;
    size_t allocSize = sizeof(struct GC_TLSF_array) + arraySize;

    pthread_mutex_lock(&(s->array_mutex));
    pointer array = (pointer) tlsf_malloc(allocSize);


    /* Array criteria needs to be addressed */
    if (!array) {
        GC_collect(s, 0, true);
        fprintf(stderr, "Cannot allocate array, GC!\n");
        array = (pointer) tlsf_malloc(allocSize);
    }

    GC_TLSF_array arrayHeader = (GC_TLSF_array) array;
    arrayHeader->magic = 9933;
    arrayHeader->next = s->tlsfarheap.allocatedArray->next;
    s->tlsfarheap.allocatedArray->next = arrayHeader;

    pthread_mutex_unlock(&(s->array_mutex));
    arrayHeader->array_ml_header = header;
    arrayHeader->array_header = 0;
    arrayHeader->array_length = numElements;
    arrayHeader->object_version = s->object_alloc_version;
    pointer frontier = array + sizeof(struct GC_TLSF_array);
    pointer last = frontier + arraySize;

    if (1 <= numObjptrs and 0 < numElements) {
        pointer p;

        if (0 == bytesNonObjptrs)
            for (p = frontier; p < last; p += OBJPTR_SIZE)
                *((objptr*)p) = BOGUS_OBJPTR;
        else {
            /* Array with a mix of pointers and non-pointers. */
            size_t bytesObjptrs;

            bytesObjptrs = numObjptrs * OBJPTR_SIZE;

            for (p = frontier; p < last; ) {
                pointer next;

                p += bytesNonObjptrs;
                next = p + bytesObjptrs;
                assert (next <= last);
                for ( ; p < next; p += OBJPTR_SIZE)
                    *((objptr*)p) = BOGUS_OBJPTR;
            }
        }
    }

    //    fprintf(stderr, "Array magic... 0x%x, %d, 0x%x\n", arrayHeader,
    //            arrayHeader->magic, frontier);
    return frontier;
}

bool createTLSFArrayHeap(GC_state s, GC_TLSF_heap h,
                         size_t desiredSize)
{
    pointer newStart;
    newStart = GC_mmapAnon(NULL, desiredSize);
    if (newStart == (void*) -1) {
        fprintf(stderr, "[GC: MMap Failure]\n");
        return FALSE;
    }


    h->start = newStart;
    h->size = desiredSize;
    //    fprintf(stderr, "Mapping TLSF: 0x%x, size: %d\n", h->start, h->size);
    h->allocatedArray = (GC_TLSF_array)malloc(sizeof(struct GC_TLSF_array));
    h->allocatedArray->next = NULL;
    init_memory_pool(desiredSize, (void*) newStart);
}
