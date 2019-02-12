#ifndef _contract_hpp_INCLUDED
#define _contract_hpp_INCLUDED

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif /* _MSC_VER */

#ifndef NSIGNALS
  #define RAISE_CONTRACT_VIOLATION do { raise (CaDiCaL::Solver::contract_violation_signal); } while (0)
#else
  #define RAISE_CONTRACT_VIOLATION do {} while (0)
#endif


#define CONTRACT_VIOLATED(...) \
do { \
  fflush (stdout); \
  fprintf (stderr, \
    "*** 'CaDiCaL' invalid API usage of '%s' in '%s': ", \
    __PRETTY_FUNCTION__, __FILE__); \
  fprintf (stderr, __VA_ARGS__); \
  fputc ('\n', stderr); \
  RAISE_CONTRACT_VIOLATION; \
  abort (); \
} while (0)

#define REQUIRE(COND, ...) \
do { \
  if ((COND)) break; \
  CONTRACT_VIOLATED (__VA_ARGS__); \
} while (0)

#define REQUIRE_INITIALIZED() \
do { \
  REQUIRE(this, "solver uninitialized"); \
} while (0)

#define REQUIRE_VALID_LIT(LIT) \
do { \
  REQUIRE ((((int) (LIT)) != INT_MIN), \
    "invalid literal '%ld'", (long)(LIT)); \
} while (0)

#endif
