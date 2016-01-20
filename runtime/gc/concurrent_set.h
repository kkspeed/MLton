#if (defined (MLTON_GC_INTERNAL_TYPES))
typedef struct GC_Concurrent_Set {
    pthread_mutex mutex;

} *GC_Concurrent_Set;

#endif /* MLTON_GC_INTERNAL_TYPES */
