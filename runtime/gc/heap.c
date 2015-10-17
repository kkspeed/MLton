/* Copyright (C) 2009-2012 Matthew Fluet.
 * Copyright (C) 2005-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void displayHeap (__attribute__ ((unused)) GC_state s,
                  GC_heap heap,
                  FILE *stream) {
}


void initHeap (__attribute__ ((unused)) GC_state s,
               GC_heap h) {
  h->size = 0;
  h->start = NULL;
}

/* sizeofHeapDesired (s, l, cs)
 *
 * returns the desired heap size for a heap with l bytes live,
 * given that the current heap size is cs.
 */
size_t sizeofHeapDesired (GC_state s, size_t liveSize, size_t currentSize) {
  size_t liveMapsSize, liveWithMapsSize;
  size_t currentMapsSize, currentWithMapsSize;
  size_t resSize, resWithMapsSize;
  size_t syslimSize, syslimWithMapsSize;
  LOCAL_USED_FOR_ASSERT size_t syslimMapsSize;
  double ratio;

  syslimWithMapsSize = alignDown (SIZE_MAX, s->sysvals.pageSize);
  syslimSize = invertSizeofCardMapAndCrossMap (s, syslimWithMapsSize);
  syslimMapsSize = sizeofCardMapAndCrossMap (s, syslimSize);
  assert (syslimSize + syslimMapsSize <= syslimWithMapsSize);

  liveSize = align (liveSize, s->sysvals.pageSize);
  if (syslimSize < liveSize)
    die ("Out of memory with system-limit heap size %s.\n",
         uintmaxToCommaString(syslimSize));
  liveMapsSize = sizeofCardMapAndCrossMap (s, liveSize);
  liveWithMapsSize = liveSize + liveMapsSize;

  assert (isAligned (currentSize, s->sysvals.pageSize));
  currentMapsSize = sizeofCardMapAndCrossMap (s, currentSize);
  currentWithMapsSize = currentSize + currentMapsSize;

  ratio = (double)s->sysvals.ram / (double)liveWithMapsSize;

  if (ratio >= s->controls.ratios.live + s->controls.ratios.grow) {
    /* Cheney copying fits in RAM with desired ratios.live. */
    resWithMapsSize = (size_t)(liveWithMapsSize * s->controls.ratios.live);
    /* If the heap is currently close in size to what we want, leave
     * it alone.  Favor growing over shrinking.
     */
    if (0.5 * currentWithMapsSize <= resWithMapsSize
        and resWithMapsSize <= 1.1 * currentWithMapsSize) {
      resWithMapsSize = currentWithMapsSize;
    } else {
      resWithMapsSize = align (resWithMapsSize, s->sysvals.pageSize);
    }
  } else if (s->controls.ratios.grow >= s->controls.ratios.copy
             and ratio >= 2.0 * s->controls.ratios.copy) {
    /* Split RAM in half.  Round down by pageSize so that the total
     * amount of space taken isn't greater than RAM once rounding
     * happens.  This is so resizeHeapSecondary doesn't get confused
     * and free a semispace in a misguided attempt to avoid paging.
     */
    resWithMapsSize = alignDown (s->sysvals.ram / 2, s->sysvals.pageSize);
  } else if (ratio >= s->controls.ratios.copy + s->controls.ratios.grow) {
    /* Cheney copying fits in RAM. */
    resWithMapsSize = s->sysvals.ram - (size_t)(s->controls.ratios.grow * liveWithMapsSize);
    /* If the heap isn't too much smaller than what we want, leave it
     * alone.  On the other hand, if it is bigger we want to leave
     * resWithMapsSize as is so that the heap is shrunk, to try to
     * avoid paging.
     */
    if (1.0 * currentWithMapsSize <= resWithMapsSize
        and resWithMapsSize <= 1.1 * currentWithMapsSize) {
      resWithMapsSize = currentWithMapsSize;
    } else {
      resWithMapsSize = align (resWithMapsSize, s->sysvals.pageSize);
    }
  } else if (ratio >= s->controls.ratios.markCompact) {
    /* Mark compact fits in RAM.  It doesn't matter what the current
     * size is.  If the heap is currently smaller, we are using
     * copying and should switch to mark-compact.  If the heap is
     * currently bigger, we want to shrink back to RAM to avoid
     * paging.
     */
    resWithMapsSize = s->sysvals.ram;
  } else { /* Required live ratio. */
    double resWithMapsSizeD = liveWithMapsSize * (double)(s->controls.ratios.markCompact);
    if (resWithMapsSizeD > (double)syslimWithMapsSize) {
      resWithMapsSize = syslimWithMapsSize;
    } else {
      resWithMapsSize = align ((size_t)resWithMapsSizeD, s->sysvals.pageSize);
    }
    /* If the current heap is bigger than resWithMapsSize, then
     * shrinking always sounds like a good idea.  However, depending
     * on what pages the VM keeps around, growing could be very
     * expensive, if it involves paging the entire heap.  Hopefully
     * the copy loop in growHeap will make the right thing happen.
     */
  }
  if (s->controls.fixedHeap > 0) {
    if (resWithMapsSize > s->controls.fixedHeap / 2)
      resWithMapsSize = s->controls.fixedHeap;
    else
      resWithMapsSize = s->controls.fixedHeap / 2;
    if (resWithMapsSize < liveWithMapsSize)
      die ("Out of memory with fixed heap size %s.",
           uintmaxToCommaString(s->controls.fixedHeap));
  } else if (s->controls.maxHeap > 0) {
    if (resWithMapsSize > s->controls.maxHeap)
      resWithMapsSize = s->controls.maxHeap;
    if (resWithMapsSize < liveWithMapsSize)
      die ("Out of memory with max heap size %s.",
           uintmaxToCommaString(s->controls.maxHeap));
  }
  resSize = invertSizeofCardMapAndCrossMap (s, resWithMapsSize);
  assert (isAligned (resSize, s->sysvals.pageSize));
  if (DEBUG_RESIZING)
    fprintf (stderr, "%s = sizeofHeapDesired (%s, %s)\n",
             uintmaxToCommaString(resSize),
             uintmaxToCommaString(liveSize),
             uintmaxToCommaString(currentSize));
  assert (resSize >= liveSize);
  return resSize;
}

void releaseHeap (GC_state s, GC_heap h) {
  if (NULL == h->start)
    return;
  GC_release (h->start, h->size);
  initHeap (s, h);
}

/* createHeap (s, h, desiredSize, minSize)
 *
 * allocates a heap of the size necessary to work with desiredSize
 * live data, and ensures that at least minSize is available.  It
 * returns TRUE if it is able to allocate the space, and returns FALSE
 * if it is unable.
 */
bool createHeap (GC_state s, GC_heap h,
                 size_t desiredSize,
                 size_t minSize)
{
    pointer newStart;
    newStart = GC_mmapAnon (NULL, desiredSize);;
    h->start = newStart;
    h->size = desiredSize;
    return TRUE;
}
