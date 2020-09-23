//
// Created by xc5 on 2020/9/23.
//

#ifndef EXAMPLE_GL_DEFS_H
#define EXAMPLE_GL_DEFS_H


#if 0
#if __GNUC__
#if __x86_64__ || __ppc64__
#define __M64__
#else
#define __M32__
#endif
#endif
#endif

#define __M32__

#ifndef TARG_UWASM
typedef long long                 INT64;
typedef unsigned long long        UINT64;
typedef int                       INT32;
typedef unsigned int              UINT32;
typedef unsigned char             UINT8;
typedef char                      INT8;
typedef unsigned short            UINT16;
typedef short                     INT16;
typedef int                       BOOL;
#ifdef __M64__
typedef INT64                     INTPTR;
typedef UINT64                    UINTPTR;
#else
typedef INT32                     INTPTR;
typedef UINT32                    UINTPTR;
#endif
#endif

typedef float                     FLOAT32;
typedef double                    FLOAT64;

typedef UINT8 *                   BUFFER;
typedef const UINT8 *             CONST_BUFFER;

#endif //EXAMPLE_GL_DEFS_H
