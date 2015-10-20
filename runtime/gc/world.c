/* Copyright (C) 1999-2008 Henry Cejtin, Matthew Fluet, Suresh
 *    Jagannathan, and Stephen Weeks.
 * Copyright (C) 1997-2000 NEC Research Institute.
 *
 * MLton is released under a BSD-style license.
 * See the file MLton-LICENSE for details.
 */

void loadWorldFromFILE (GC_state s, FILE *f) {
    die("loadWorldFromFILE not implemented\n");
}

void loadWorldFromFileName (GC_state s, const char *fileName) {
    die("loadWorldFromFileName not implemented\n");
}

int saveWorldToFILE (GC_state s, FILE *f) {
    die("saveWorldToFILE not implemented\n");
    return 0;
}

void GC_saveWorld (GC_state s, NullString8_t fileName) {
    die("GC_saveWorld not implemented\n");
}

C_Errno_t(Bool_t) GC_getSaveWorldStatus (GC_state s) {
    return (Bool_t)(s->saveWorldStatus);
}
