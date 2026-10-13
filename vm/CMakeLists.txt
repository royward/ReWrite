cmake_minimum_required(VERSION 3.20)
project(rewrite_vm VERSION 0.0.9 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(RW_SANITIZE "Build with address and undefined-behaviour sanitizers" OFF)

# ---- Shared compile settings, inherited by everything that links it ----
add_library(rw_vmsettings INTERFACE)
target_compile_features(rw_vmsettings INTERFACE c_std_11)
target_compile_options(rw_vmsettings INTERFACE
    -Wall -Wextra -pedantic -ggdb3
    $<$<COMPILE_LANGUAGE:CXX>:-Wredundant-move>
    $<$<C_COMPILER_ID:Clang>:-fno-limit-debug-info>
    $<$<BOOL:${RW_SANITIZE}>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(rw_vmsettings INTERFACE
    $<$<BOOL:${RW_SANITIZE}>:-fsanitize=address,undefined>
)
set(CMAKE_C_EXTENSIONS OFF)

# ---- The VM as a library ----
add_library(rwvm STATIC
    src/vm.c
)
target_include_directories(rwvm PUBLIC include generated)
target_link_libraries(rwvm PUBLIC rw_vmsettings)

# ---- Applications ----
function(rw_app name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE rwvm)
    target_compile_definitions(${name} PRIVATE RW_ROOT="${CMAKE_SOURCE_DIR}")
endfunction()

rw_app(rw_disassembler apps/disassembler.c)
rw_app(vm_gen apps/vm_gen.c)
