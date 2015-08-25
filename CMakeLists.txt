cmake_minimum_required(VERSION 2.8)
project(tpw)


include(TestCXXAcceptsFlag)
check_cxx_accepts_flag("--std=c++11" CPP11_SUPPORTED_FLAG)
if (NOT CPP11_SUPPORTED_FLAG)
	message(FATAL_ERROR "C++11 is required")
endif()
add_definitions(-std=c++11)

set(TARANTOOL_C_INCLUDE_DIR
  "/somewhere/tarantool-c/src/include"
  CACHE STRING "directory with tp.h"
  )

set(TARANTOOL_C_THIRD_PARTY
  "/somewhere/tarantool-c/src/third_party"
  CACHE STRING "directory with third party stuff"
  )

set(TARANTOOL_C_LIBRARIES
  "/somewhere/tarantool-c/"
  CACHE STRING "directory with libtb binary"
  )

set(MSGPUCK_INCLUDE_DIR
  "/somewhere/tarantool-c/src/third_party/msgpuck"
  CACHE STRING "directory with msgpuck.h"
  )

include_directories(
	${TARANTOOL_C_INCLUDE_DIR}
	${TARANTOOL_C_THIRD_PARTY}
	${MSGPUCK_INCLUDE_DIR}
	)

link_directories(
	${TARANTOOL_C_LIBRARIES}
	)

add_library(tpw
	tpw.cpp
	)

target_link_libraries(tpw	LINK_PRIVATE
	tarantoolnet
	tarantool
	)

add_executable(tpwtest 
	test.cpp
	)

target_link_libraries(tpwtest
	tpw
	)

	
