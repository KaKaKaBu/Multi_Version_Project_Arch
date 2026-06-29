set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mcs51)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MCS51_TOOLCHAIN_BIN "" CACHE PATH "SDCC toolchain bin directory (optional; also read from env)")

set(_mcs51_search_hints "")
if(NOT "${MCS51_TOOLCHAIN_BIN}" STREQUAL "")
    list(APPEND _mcs51_search_hints "${MCS51_TOOLCHAIN_BIN}")
endif()
if(DEFINED ENV{MCS51_TOOLCHAIN_BIN} AND NOT "$ENV{MCS51_TOOLCHAIN_BIN}" STREQUAL "")
    list(APPEND _mcs51_search_hints "$ENV{MCS51_TOOLCHAIN_BIN}")
endif()

find_program(CMAKE_C_COMPILER sdcc
    PATHS ${_mcs51_search_hints}
    REQUIRED
    DOC "SDCC C compiler"
)
find_program(CMAKE_SIZE size
    PATHS ${_mcs51_search_hints}
    DOC "size utility"
)

set(CMAKE_C_COMPILER_ID_RUN TRUE)
set(CMAKE_C_COMPILER_ID SDCC)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_C_FLAGS_INIT "-mmcs51 --std-sdcc99 --model-small --stack-auto")
set(CMAKE_EXECUTABLE_SUFFIX ".ihx")

unset(_mcs51_search_hints)
