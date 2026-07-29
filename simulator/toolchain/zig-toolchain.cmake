# Chaine de compilation reproductible sans installation prealable de Visual
# Studio ni de MSYS2 : utilise `zig cc`/`zig c++` (paquet pip "ziglang"),
# cible x86_64-windows-gnu. Voir simulator/README.md.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

get_filename_component(ZIG_TOOLCHAIN_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

set(CMAKE_C_COMPILER "${ZIG_TOOLCHAIN_DIR}/zigcc.cmd")
set(CMAKE_CXX_COMPILER "${ZIG_TOOLCHAIN_DIR}/zigcxx.cmd")
set(CMAKE_AR "${ZIG_TOOLCHAIN_DIR}/zigar.cmd" CACHE FILEPATH "" FORCE)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
