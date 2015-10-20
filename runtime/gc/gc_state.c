/* Copyright (C) 2009,2012 Matthew Fluet.
 * Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void displayGCState (GC_state s, FILE *stream) {
  fprintf (stream,
           "GC state\n");
  fprintf (stream, "\tcurrentThread = "FMTOBJPTR"\n", s->currentThread);
  displayThread (s, (GC_thread)(objptrToPointer (s->currentThread, s->heap.start)
                                + offsetofThread (s)),
                 stream);
  /* fprintf (stream, "\tgenerational\n"); */
  /* displayGenerationalMaps (s, &s->generationalMaps, */
  /*                          stream); */
  fprintf (stream, "\theap\n");
  displayHeap (s, &s->heap,
               stream);
  fprintf (stream,
           "\tlimit = "FMTPTR"\n"
           "\tstackBottom = "FMTPTR"\n"
           "\tstackTop = "FMTPTR"\n",
           (uintptr_t)s->limit,
           (uintptr_t)s->stackBottom,
           (uintptr_t)s->stackTop);
}

size_t sizeofGCStateCurrentStackUsed (GC_state s) {
  return (size_t)(s->stackTop - s->stackBottom);
}

void setGCStateCurrentThreadAndStack (GC_state s) {
  GC_thread thread;
  GC_stack stack;

  thread = getThreadCurrent (s);
  s->exnStack = thread->exnStack;
  stack = getStackCurrent (s);
  s->stackBottom = getStackBottom (s, stack);
  s->stackTop = getStackTop (s, stack);
  s->stackLimit = getStackLimit (s, stack);
}

bool GC_getAmOriginal (GC_state s) {
  return s->amOriginal;
}
void GC_setAmOriginal (GC_state s, bool b) {
  s->amOriginal = b;
}

void GC_setControlsMessages (GC_state s, bool b) {
  s->controls.messages = b;
}

void GC_setControlsSummary (GC_state s, bool b) {
  s->controls.summary = b;
}

void GC_setControlsRusageMeasureGC (GC_state s, bool b) {
  s->controls.rusageMeasureGC = b;
}

uintmax_t GC_getCumulativeStatisticsBytesAllocated (GC_state s) {
  return s->cumulativeStatistics.bytesAllocated;
}

uintmax_t GC_getCumulativeStatisticsNumCopyingGCs (GC_state s) {
  return s->cumulativeStatistics.numCopyingGCs;
}

uintmax_t GC_getCumulativeStatisticsNumMarkCompactGCs (GC_state s) {
  return s->cumulativeStatistics.numMarkCompactGCs;
}

uintmax_t GC_getCumulativeStatisticsNumMinorGCs (GC_state s) {
  return s->cumulativeStatistics.numMinorGCs;
}

size_t GC_getCumulativeStatisticsMaxBytesLive (GC_state s) {
  return s->cumulativeStatistics.maxBytesLive;
}

void GC_setHashConsDuringGC (GC_state s, bool b) {
  s->hashConsDuringGC = b;
}

size_t GC_getLastMajorStatisticsBytesLive (GC_state s) {
  return s->lastMajorStatistics.bytesLive;
}


pointer GC_getCallFromCHandlerThread (GC_state s) {
  pointer p = objptrToPointer (s->callFromCHandlerThread, s->heap.start);
  return p;
}

void GC_setCallFromCHandlerThread (GC_state s, pointer p) {
  objptr op = pointerToObjptr (p, s->heap.start);
  s->callFromCHandlerThread = op;
}

pointer GC_getCurrentThread (GC_state s) {
  pointer p = objptrToPointer (s->currentThread, s->heap.start);
  return p;
}

pointer GC_getSavedThread (GC_state s) {
  pointer p;

  assert(s->savedThread != BOGUS_OBJPTR);
  p = objptrToPointer (s->savedThread, s->heap.start);
  s->savedThread = BOGUS_OBJPTR;
  return p;
}

void GC_setSavedThread (GC_state s, pointer p) {
  objptr op;

  assert(s->savedThread == BOGUS_OBJPTR);
  op = pointerToObjptr (p, s->heap.start);
  s->savedThread = op;
}

void GC_setSignalHandlerThread (GC_state s, pointer p) {
  objptr op = pointerToObjptr (p, s->heap.start);
  s->signalHandlerThread = op;
}

struct rusage* GC_getRusageGCAddr (GC_state s) {
  return &(s->cumulativeStatistics.ru_gc);
}

sigset_t* GC_getSignalsHandledAddr (GC_state s) {
  return &(s->signalsInfo.signalsHandled);
}

sigset_t* GC_getSignalsPendingAddr (GC_state s) {
  return &(s->signalsInfo.signalsPending);
}

void GC_setGCSignalHandled (GC_state s, bool b) {
  s->signalsInfo.gcSignalHandled = b;
}

bool GC_getGCSignalPending (GC_state s) {
  return (s->signalsInfo.gcSignalPending);
}

void GC_setGCSignalPending (GC_state s, bool b) {
  s->signalsInfo.gcSignalPending = b;
}
