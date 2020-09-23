#ifndef GL_EXAMPLE_U_TRACE_H
#define GL_EXAMPLE_U_TRACE_H

#include "defs.h"

#ifndef TARG_UWASM
#ifdef GL_EXAMPLE_RELEASE
#define Is_Trace(cond, msg)
#define If_Trace(cond, statements)
#else
#define Is_Trace(cond, msg) if ((cond)) { fprintf msg; }
#define If_Trace(cond, statements) if (cond) statements
#endif
#define TFile stdout
#endif
#define AssertDev(Condition, Params) \
        (Condition ? (void) 1 \
        :(Quit_with_tracing ( __FILE__, __LINE__, __func__),	\
        Assertion_Failure_Print Params) );

#define SBAR "-----------------------------------------------------------------------\n"
#define DBAR "=======================================================================\n"
#define UIMPL "NYI"
#define NYI "NYI"
#define ILLEGAL_INSTRUCTION "ILLEGAL_INSTRUCTION"

/* Only in non-release mode */
#ifndef TARG_UWASM
#ifdef GL_EXAMPLE_RELEASE
#define Is_True(cond, msg)
#else
#define Is_True AssertDev
#endif
#endif
#define TRACE_ENABLE_LEVEL 1000

// This is vulnerable to changes
typedef enum {
  TP_PARSE     = 1,
  TP_GL        = 2,
  TP_SHADER    = 3,
  TP_VERTEX    = 4,
  TRACE_KIND_MAX = TP_VERTEX + 1,
} TRACE_KIND;

extern UINT32 Set_tracing_opt(TRACE_KIND kind, UINT32 level);
extern bool Tracing(TRACE_KIND);
extern void Quit_with_tracing(const char *, UINT32, const char *); // Quiting
extern void Assertion_Failure_Print ( const char *fmt, ... ); // Printf-like function


#endif //GL_EXAMPLE_U_TRACE_H
