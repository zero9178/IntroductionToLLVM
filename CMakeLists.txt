cmake_minimum_required(VERSION 3.10)
project(SimpleC )

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

find_package(LLVM REQUIRED)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

add_executable(SimpleC main.cpp Lexer.cpp Lexer.hpp Parser.cpp Parser.hpp Codegen.cpp Codegen.hpp)
llvm_map_components_to_libnames(llvm_all ${LLVM_TARGETS_TO_BUILD} Passes)
target_link_libraries(SimpleC ${llvm_all})

#if (NOT LLVM_ENABLE_RTTI)
#    if (MSVC)
#        string(REGEX REPLACE "/GR" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
#    else ()
#        string(REGEX REPLACE "-frtti" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
#    endif ()
#endif ()
