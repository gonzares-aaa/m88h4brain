#ifndef COMPILER_H
#define COMPILER_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef UINT8
#define UINT8   unsigned char
#endif

#ifndef UINT16
#define UINT16  unsigned short
#endif

#ifndef UINT32
#define UINT32  unsigned int
#endif

#ifndef UINT
#define UINT    unsigned int
#endif

#ifndef SINT16
#define SINT16  signed short
#endif

#ifndef SINT32
#define SINT32  signed int
#endif

#ifndef BRESULT
#define BRESULT int
#endif

#ifndef OEMCHAR
#define OEMCHAR char
#endif

#define OEMTEXT(x)      x
#define OEMSPRINTF      sprintf
#define OEMSTRLEN       strlen

#define OSLANG_SJIS     1

#undef SUCCESS
#undef FAILURE
#define SUCCESS         0
#define FAILURE         1

#define _MALLOC(s, n)   malloc(s)
#define _MFREE(p)       free(p)
#define NELEMENTS(a)    (sizeof(a) / sizeof(a[0]))
#define TRACEOUT(a)

#define milsjis_ncpy(a, b, c)    strncpy(a, b, c)
#define milsjis_ncat(a, b, c)    strncat(a, b, c)
#define milsjis_cmp(a, b)        strcmp(a, b)
#define milstr_charsize(p)       (*(p) == '\0' ? 0 : (IsDBCSLeadByte((BYTE)*(p)) ? 2 : 1))

static __inline int milstr_kanji2nd(const char* p, int pos) {
    int i = 0;
    while (i < pos) {
        if (IsDBCSLeadByte((BYTE)p[i])) {
            i += 2;
        } else {
            i++;
        }
    }
    // posを飛び越えたら、posの位置にあった文字は2バイト目だったということ
    return (i > pos) ? 1 : 0;
}

#endif // COMPILER_H
