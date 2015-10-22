/* Copyright (C) 2009-2010,2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void minorGC (GC_state s) {
    //    minorCheneyCopyGC (s);
}

void enterGC (GC_state s) {
  if (s->profiling.isOn) {
    /* We don't need to profileEnter for count profiling because it
     * has already bumped the counter.  If we did allow the bump, then
     * the count would look like function(s) had run an extra time.
     */
    if (s->profiling.stack
        and not (PROFILE_COUNT == s->profiling.kind))
      GC_profileEnter (s);
  }
  s->amInGC = TRUE;
}

void leaveGC (GC_state s) {
  if (s->profiling.isOn) {
    if (s->profiling.stack
        and not (PROFILE_COUNT == s->profiling.kind))
      GC_profileLeave (s);
  }
  s->amInGC = FALSE;
}



void performUMGC(GC_state s,
                 size_t ensureObjectChunksAvailable,
                 size_t ensureArrayChunksAvailable,
                 bool fullGC) {

    if (DEBUG_MEM) {
        fprintf(stderr, "PerformUMGC\n");
        dumpUMHeap(s);
    }


#ifdef PROFILE_UMGC
    long t_start = getCurrentTime();
    fprintf(stderr, "[GC] ENTER: Free chunk: %d, Obj Version: %lld, GC Version: %lld\n",
            s->fl_chunks,
            s->object_alloc_version,
            s->gc_object_version);
#endif

    for (uint32_t i=0; i<s->root_set_size; i++) {
        //        pointer p = (s->root_sets[i]);
        //        foreachObjptrInObject(s, p, umDfsMarkObjectsMark, false);
        umDfsMarkObjectsMark(s, &(s->root_sets[i]));
        //umDfsMarkObjectsMark(s, (s->root_sets[i]));
    }

    fprintf(stderr, "[GC] MARK DONE, collecting!\n");
    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Chunk);
    pointer end = s->umheap.start + s->umheap.size - step;

    fprintf(stderr, "[GC] before collecting\n");
    for (pchunk=s->umheap.start;
         pchunk < end;
         pchunk+=step) {
        GC_UM_Chunk pc = (GC_UM_Chunk)pchunk;

        if ((pc->chunk_header & UM_CHUNK_IN_USE) &&
            pc->object_version < s->gc_object_version) {
            //            if (DEBUG_MEM) {
                fprintf(stderr, "Collecting: "FMTPTR", %d, %d\n",
                        (uintptr_t)pc, pc->sentinel, pc->object_version);
                //            }
                //            insertFreeChunk(s, &(s->umheap), pchunk);
        }

        /* if (!fullGC && s->fl_chunks >= ensureObjectChunksAvailable) { */
        /*     break; */
        /* } */
    }

    GC_TLSF_array current = s->tlsfarheap.allocatedArray;
    while (current->next) {
        if (current->next->object_version < s->gc_object_version) {
            pthread_mutex_lock(&(s->array_mutex));
            fprintf(stderr, "Collecting array, haha!\n");
            GC_TLSF_array tmp = current->next;
            current->next = current->next->next;
            tlsf_free((void*)tmp);
            pthread_mutex_unlock(&(s->array_mutex));
        } else {
            current = current->next;
        }
    }

    for (uint32_t i=0; i<s->root_set_size; i++) {
        //        pointer p = (s->root_sets[i]);
        //        foreachObjptrInObject(s, p, umDfsMarkObjectsUnMark, false);
        umDfsMarkObjectsUnMark(s, &(s->root_sets[i]));
    }

    //    fprintf(stderr, "GC returend!\n");
    //    foreachObjptrInObject(s, (pointer) currentStack, umDfsMarkObjectsUnMark, FALSE);
    //    foreachGlobalObjptr (s, umDfsMarkObjectsUnMark);
    //    s->root_set_size = 0;
    //    s->gc_object_version = s->object_alloc_version;
#ifdef PROFILE_UMGC
    long t_end = getCurrentTime();
    fprintf(stderr, "[GC] Time: %ld, Free chunk: %d, Free array chunk: %d, "
            "ensureArrayChunk: %d, GC version: %lld, object_alloc_version: %lld\n",
            t_end - t_start,
            s->fl_chunks,
            s->fl_array_chunks,
            ensureArrayChunksAvailable,
            s->gc_object_version,
            s->object_alloc_version);
#endif

}

void GC_collect_real(GC_state s, size_t bytesRequested, bool force) {
  fprintf(stderr, "Collecting!\n");
  performUMGC(s, 0, 0, true);
  if (DEBUG_MEM) {
      fprintf(stderr, "GC_collect done\n");
  }
}

void collectRootSet(GC_state s, objptr* opp)
{
    if (isObjptr(*opp) && *opp != BOGUS_OBJPTR)
        s->root_sets[s->root_set_size++] = *opp;
}

void GC_collect (GC_state s, size_t bytesRequested, bool force) {
    if (!force) {
        if (s->fl_chunks > 7500000)
            return;
    }

    if (pthread_mutex_trylock(&s->gc_stat_mutex) == 0) {
        if (s->gc_work == 0) {
            s->gc_object_version = s->object_alloc_version;
            s->object_alloc_version++;
            fprintf(stderr, "Object version: %lld\n", s->object_alloc_version);
            s->root_set_size = 0;
            foreachGlobalObjptr(s, collectRootSet);
            pointer top, bottom;
            unsigned int i;
            GC_returnAddress returnAddress;
            GC_frameLayout frameLayout;
            GC_frameOffsets frameOffsets;
            bottom = s->stackBottom; //getStackBottom (s, stack);
            top = s->stackTop;/* getStackTop (s, stack); */
            while (top > bottom) {
                returnAddress = *((GC_returnAddress*)(top - GC_RETURNADDRESS_SIZE));
                frameLayout = getFrameLayoutFromReturnAddress (s, returnAddress);
                frameOffsets = frameLayout->offsets;
                top -= frameLayout->size;
                for (i = 0 ; i < frameOffsets[0] ; ++i) {
                    callIfIsObjptr (s, collectRootSet,
                                    (objptr*)(top + frameOffsets[i + 1]));
                }
            }
            fprintf(stderr, "Got root set, size: %d, at GC version: %lld, "
                    "object version: %lld!\n", s->root_set_size, s->gc_object_version,
                    s->object_alloc_version);
            s->gc_work = 1;
        }
        //        s->gc_work = 0;
        //        GC_collect_real(s, 0, true);
        pthread_mutex_unlock(&s->gc_stat_mutex);
        //               pthread_yield();
        //        performUMGC(s, 0, 0, true);
    }
}
