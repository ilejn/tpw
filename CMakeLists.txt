cmake_minimum_required(VERSION 2.8)
project(tpw)


include(TestCXXAcceptsFlag)
check_cxx_accepts_flag("--std=c++11" CPP11_SUPPORTED_FLAG)
if (NOT CPP11_SUPPORTED_FLAG)
	message(FATAL_ERROR "C++11 is required")
endif()
add_definitions(-std=c++11)

set(TARANTOOL_C_LIB
  "/somewhere/tarantool-c/"
  CACHE STRING "directory with libtb binary"
  )

link_directories(
	${TARANTOOL_C_LIB}
	)

add_library(tpw
	tpw.cpp
	)

target_link_libraries(tpw
	LINK_PRIVATE	tb
	)

add_executable(tpwtest 
	test.cpp
	)

target_link_libraries(tpwtest
	tpw
	)

	
