/* Copyright (C) 1999-2007 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void switchToThread (GC_state s, objptr op) {
  if (DEBUG_THREADS) {
    GC_thread thread;
    GC_stack stack;

    thread = (GC_thread)(objptrToPointer (op, s->heap.start)
                         + offsetofThread (s));
    stack = (GC_stack)(objptrToPointer (thread->stack, s->heap.start));

    fprintf (stderr, "switchToThread ("FMTOBJPTR")  used = %"PRIuMAX
             "  reserved = %"PRIuMAX"\n",
             op, (uintmax_t)stack->used, (uintmax_t)stack->reserved);
  }
  s->currentThread = op;
  setGCStateCurrentThreadAndStack (s);
}

void GC_switchToThread (GC_state s, pointer p, size_t ensureBytesFree) {
  if (DEBUG_THREADS)
    fprintf (stderr, "GC_switchToThread ("FMTPTR", %"PRIuMAX")\n",
             (uintptr_t)p, (uintmax_t)ensureBytesFree);
    /* BEGIN: enter(s); */
  getStackCurrent(s)->used = sizeofGCStateCurrentStackUsed (s);
  getThreadCurrent(s)->exnStack = s->exnStack;
  getThreadCurrent(s)->bytesNeeded = ensureBytesFree;
  switchToThread (s, pointerToObjptr(p, s->heap.start));
  switchToSignalHandlerThreadIfNonAtomicAndSignalPending (s);
}
