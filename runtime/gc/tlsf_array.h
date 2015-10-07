#if (defined (MLTON_GC_INTERNAL_TYPES))

typedef struct GC_TLSF_array {
    struct GC_TLSF_array* next;
    Word32_t array_header;
    Word32_t array_length;
    Word32_t array_ml_header;
}* GC_TLSF_array;

typedef struct GC_TLSF_heap {
    pointer start;
    GC_TLSF_array allocatedArray;
} *GC_TLSF_heap;

#endif  // MLTON_GC_INTERNAL_TYPES

#if (defined (MLTON_GC_INTERNAL_FUNCS))
bool createTLSFArrayHeap(GC_state s, GC_TLSF_heap h,
                         size_t desiredSize);
#endif  // MLTON_GC_INTERNAL_FUNCS
