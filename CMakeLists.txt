cmake_minimum_required(VERSION 2.6.3)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

message( STATUS "cmake version ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" )

include(CheckIncludeFiles)
include(CheckLibraryExists)

find_library(HAVE_HIREDIS hiredis)
  check_library_exists(
    hiredis
    redisCommand
    ""
    HAVE_HIREDIS
    )
check_include_files("hiredis/hiredis.h" HAVE_HIREDIS_H)

if((NOT HAVE_HIREDIS) OR (NOT HAVE_HIREDIS_H))
      message(FATAL_ERROR "Cannot find hiredis")
endif((NOT HAVE_HIREDIS) OR (NOT HAVE_HIREDIS_H))

add_subdirectory(kvshl)

add_executable(kvsns_test kvsns_test.c kvsns.c)
target_link_libraries(kvsns_test kvshl_redis hiredis)

