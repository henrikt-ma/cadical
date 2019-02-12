cmake_minimum_required (VERSION 2.8.8)
project(CaDiCal)

include(CheckCXXSymbolExists)

option(STATS "Include and enable expensive statistics code" OFF)
option(LOGGING "Include logging code (but disabled by default)" OFF)
option(CHECK "Compile with assertion checking (default for debug builds)" OFF)
option(COVERAGE "Compile with '-ftest-coverage -fprofile-arcs' for 'gcov'" OFF)
option(PROFILE "Compile with '-pg' to profile with 'gprof'" OFF)
option(REALLOC "Use C++ style allocators for all tables" ON)
option(UNLOCKED "Force compilation without unlocked IO" ON)
option(ALL "Shortcut for CHECK, LOGGING, and STATS" OFF)
option(QUIET "Exclude message and profiling code (logging and stats too)" OFF)

if(ALL)
  set(CHECK ON)
  set(LOGGING ON)
  set(STATS ON)
endif(ALL)

if(QUIET)
  set(LOGGING OFF)
  set(STATS OFF)
elseif(CMAKE_HOST_WIN32)
  message(FATAL_ERROR "The QUIET option is required to be set on Windows platforms.")
endif(QUIET)

if(UNLOCKED)
  # Simplified check for unlocked I/O.
  # Instead of trying the test program with both putc_unlocked and getc_unlocked,
  # just look for the putc_unlocked symbol.
  check_cxx_symbol_exists(putc_unlocked cstdio HAS_PUTC_UNLOCKED)
  if(NOT HAS_PUTC_UNLOCKED)
    set(UNLOCKED OFF)
  endif()
endif(UNLOCKED)

if(LOGGING)
  add_definitions(-DLOGGING)
endif(LOGGING)

if(STATS)
  add_definitions(-DSTATS)
endif()

if(QUIET)
  add_definitions(-DQUIET)
endif()

if(NOT CHECK)
  add_definitions(-DNDEBUG)
endif()

if(NOT REALLOC)
  add_definitions(-DNREALLOC)
endif()

if(NOT UNLOCKED)
  add_definitions(-DNUNLOCKED)
endif()

if(PROFILE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif(PROFILE)

if(COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs")
endif(COVERAGE)

set(LIB_SRC
  analyze.cpp
  arena.cpp
  backtrack.cpp
  bins.cpp
  cadical.cpp
  ccadical.cpp
  clause.cpp
  collect.cpp
  compact.cpp
  decide.cpp
  decompose.cpp
  duplicated.cpp
  elim.cpp
  ema.cpp
  extend.cpp
  external.cpp
  file.cpp
  format.cpp
  internal.cpp
  limit.cpp
  logging.cpp
  message.cpp
  minimize.cpp
  occs.cpp
  options.cpp
  packtrack.cpp
  parse.cpp
  probagate.cpp
  probe.cpp
  profile.cpp
  proof.cpp
  propagate.cpp
  reduce.cpp
  rephase.cpp
  report.cpp
  resources.cpp
  restart.cpp
  signal.cpp
  solution.cpp
  stats.cpp
  subsume.cpp
  transred.cpp
  util.cpp
  var.cpp
  vivify.cpp
  watch.cpp
)

set(CLI_SRC
  app.cpp
  main.cpp
)

set(INSTALL_HEADERS
# app.hpp
  arena.hpp
  bins.hpp
  cadical.hpp
  clause.hpp
  contract.hpp
  cover.hpp
  elim.hpp
  ema.hpp
  error.hpp
  external.hpp
  file.hpp
  flags.hpp
  format.hpp
  heap.hpp
  internal.hpp
  iterator.hpp
  level.hpp
  limit.hpp
  logging.hpp
  mem.hpp
  message.hpp
  occs.hpp
  options.hpp
  parse.hpp
  profile.hpp
  proof.hpp
  queue.hpp
  resources.hpp
  signal.hpp
  stats.hpp
  util.hpp
  var.hpp
  watch.hpp
)

# CMake style preparation of config.hpp.
string(TIMESTAMP CMAKEDATE "%Y-%m-%d %H:%M") # Will be abused to define CADICAL_COMPILED.
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../VERSION CADICAL_VERSION_FILE)
string(STRIP ${CADICAL_VERSION_FILE} CADICAL_VERSION)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(BUILD_TYPE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG})
elseif(CMAKE_BUILD_TYPE MATCHES Release)
  set(BUILD_TYPE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE})
elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
  set(BUILD_TYPE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
elseif(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
  set(BUILD_TYPE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_MINSIZEREL})
elseif(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE MATCHES "")
  set(BUILD_TYPE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
else()
  message("Warning: unrecognied CMAKE_BUILD_TYPE:" ${CMAKE_BUILD_TYPE})
  set(BUILD_TYPE_CXX_FLAGS "<N/A>")
endif()
set(GIT_REVISION "unknown")
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY "${local_dir}"
    OUTPUT_VARIABLE GIT_REVISION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()
configure_file(config-template.hpp config.hpp)

add_library(cadical ${LIB_SRC})
target_include_directories(cadical PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(cadical PRIVATE -Wall)

add_executable(cli ${CLI_SRC}) # Target should be renamed "cadical" when installed.
target_include_directories(cli PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cli LINK_PUBLIC cadical)


install(
  TARGETS cadical
  DESTINATION lib
)
install(
  FILES ${INSTALL_HEADERS}
  DESTINATION include
)
