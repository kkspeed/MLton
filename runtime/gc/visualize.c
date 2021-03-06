void printObjptr(GC_state s, objptr* opp) {
    fprintf(stdout, FMTPTR", ", *opp);
}

void dumpUMHeap(GC_state s) {
    freopen("heap_ref.txt", "a", stdout);
    pointer pchunk;
    size_t step = sizeof(struct GC_UM_Chunk);
    pointer end = s->umheap.start + s->umheap.size - step;

    for (pchunk=s->umheap.start;
         pchunk < end;
         pchunk += step) {
        GC_UM_Chunk pc = (GC_UM_Chunk)pchunk;
        if (pc->chunk_header & UM_CHUNK_IN_USE) {
            fprintf(stdout, "Normal: "FMTPTR" , "FMTPTR" -> ", pchunk, pchunk + 4);
            foreachObjptrInObject(s, pchunk + 4, printObjptr, false);
            fprintf(stdout, "\n");
        }
    }

    fprintf(stdout, "========= ARRAY =========\n");
    step = sizeof(struct GC_UM_Array_Chunk);
    end = s->umarheap.start + s->umarheap.size - step;

    //    if (s->umarheap.fl_array_chunks <= 2000) {
    for (pchunk=s->umarheap.start;
         pchunk < end;
         pchunk += step) {
        GC_UM_Array_Chunk pc = (GC_UM_Array_Chunk)pchunk;
        if (pc->array_chunk_header & UM_CHUNK_IN_USE) {
            fprintf(stdout, "Array: "FMTPTR" , "FMTPTR" -> ", pchunk, pchunk + 8);
            foreachObjptrInObject(s, pchunk + 8, printObjptr, false);
            fprintf(stdout, "\n");
        }
    }

    fprintf(stdout, "========= STACK =========\n");
    fprintf(stdout, "Stack: ");
    foreachObjptrInObject(s, (pointer) getStackCurrent(s), printObjptr, FALSE);
    fprintf(stdout, "\n");

    fclose(stdout);
}
