# CMakeList.txt : CMake project for ChessAI, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.9.4)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ChessAI")

set(SOURCES
    src/main.cpp
    src/ChessAI.cpp
    src/bitboard.cpp
    include/ChessAI.h
    include/constant_eval.h
    include/bitboard.h
    include/utils.h

)
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
# Add source to this project's executable.
add_executable (ChessAI ${SOURCES} "include/ttable.h" "include/types.h" "include/constant_eval.h" "include/constant_search.h" "include/constant_bitboard.h" "include/constant_hash.h")
if( supported )
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()
target_include_directories(ChessAI
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ChessAI PROPERTY CXX_STANDARD 20)
endif()

if(MSVC)
    message("-- CMake run for msvc")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG /GS- /fp:fast /Gy")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:12000000")
elseif(CMAKE_COMPILER_IS_GNUCXX)
    message("-- CMake run for g++")
    set(CMAKE_CXX_FLAGS "-O3")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,15000000")
    

endif()


# TODO: Add tests and install targets if needed.
