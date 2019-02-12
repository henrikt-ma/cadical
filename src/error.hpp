#ifndef _error_hpp_INCLUDED
#define _error_hpp_INCLUDED

#define ERROR_START(...) \
do { \
  fflush (stdout); \
  fputs ("*** 'CaDiCaL' error: ", stderr); \
  fprintf (stderr, __VA_ARGS__); \
} while (0)

#define ERROR_END() \
do { \
  fputc ('\n', stderr); \
  fflush (stderr); \
  abort (); \
} while (0)

#define ERROR(...) \
do { \
  ERROR_START (__VA_ARGS__); \
  ERROR_END (fmt); \
} while (0)

#endif
