cmake_minimum_required(VERSION 3.20)
project(rewrite_cpp VERSION 0.0.9 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

option(RW_SANITIZE "Build with address and undefined-behaviour sanitizers" OFF)

include(FetchContent)
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.6.2
)
FetchContent_MakeAvailable(CLI11)

# ---- Shared compile settings, inherited by everything that links it ----
add_library(rw_settings INTERFACE)
target_compile_features(rw_settings INTERFACE cxx_std_17)
target_compile_options(rw_settings INTERFACE
    -Wall -Wextra -pedantic -Wredundant-move -ggdb3
    $<$<CXX_COMPILER_ID:Clang>:-fno-limit-debug-info>
    $<$<BOOL:${RW_SANITIZE}>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(rw_settings INTERFACE
    $<$<BOOL:${RW_SANITIZE}>:-fsanitize=address,undefined>
)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- The interpreter as a library ----
add_library(rwinterp STATIC
    src/lexer.cpp
    src/parser.cpp
    src/program.cpp
    src/program_call_internal.cpp
    src/util.cpp
    src/data_element.cpp
)
target_include_directories(rwinterp PUBLIC include)
target_link_libraries(rwinterp PUBLIC rw_settings)

# ---- Applications ----
function(rw_app name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE rwinterp CLI11::CLI11)
    target_compile_definitions(${name} PRIVATE RW_ROOT="${CMAKE_SOURCE_DIR}")
endfunction()

rw_app(rewrite_cpp apps/rewrite.cpp)    # general-purpose runner (the old src/main.cpp)
rw_app(rw_vmgen    apps/vmgen.cpp)      # runs the VM generator, writes the .inc files
rw_app(rw_compile  apps/compile.cpp)    # runs the compiler, writes .rwo/.ll and .h

# add_executable(rewrite_cpp
#     src/lexer.cpp
#     src/parser.cpp
#     src/program.cpp
#     src/program_call_internal.cpp
#     src/main.cpp
#     src/util.cpp
#     src/data_element.cpp
# )
#
# target_include_directories(rewrite_cpp PRIVATE include)
# target_link_libraries(rewrite_cpp PRIVATE CLI11::CLI11)
# target_compile_options(rewrite_cpp PRIVATE -Wall -Wextra -pedantic -Wredundant-move $<$<CXX_COMPILER_ID:Clang>:-fno-limit-debug-info>)
# if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#     target_compile_options(rewrite_cpp PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
#     target_link_options(rewrite_cpp PRIVATE -fsanitize=address,undefined)
# endif()
