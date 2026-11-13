#
# CMakeLists.txt
#

cmake_minimum_required(VERSION 3.20)
project(devprompt LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# --------------------------------------------------------------------
# Options
# --------------------------------------------------------------------

# option to enable static binary for fast startup and no runtime dependancies
option(DEVPROMPT_STATIC_RUNTIME "Statically link the C++ runtime" ON)

# default to debug build
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
  # /MT or /MTd instead of /MD, so the exe doesn't need the VC++ redistributable.
  if(DEVPROMPT_STATIC_RUNTIME)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
else()
  if(DEVPROMPT_STATIC_RUNTIME)
    add_link_options(-static-libgcc -static-libstdc++)
  endif()
endif()

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(ENABLE_SANITIZERS)
    add_library(devprompt_sanitizers INTERFACE)

    target_compile_options(devprompt_sanitizers INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
    )

    target_link_options(devprompt_sanitizers INTERFACE
        -fsanitize=address,undefined
    )
endif()


# --------------------------------------------------------------------
# Warnings
# --------------------------------------------------------------------

add_library(devprompt_warnings INTERFACE)

target_compile_options(devprompt_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        $<$<CONFIG:Debug>:
            -Wall
            -Wextra
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wpedantic
        >
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:
            /W4
            /permissive-
        >
    >
)


# --------------------------------------------------------------------
# Version (regenerated on every build)
# --------------------------------------------------------------------

add_custom_target(devprompt_version ALL
    COMMAND ${CMAKE_COMMAND}
        -DSRC_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
        -DBIN_ROOT=${CMAKE_CURRENT_BINARY_DIR}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/generated/Version.hpp
    COMMENT "Generating Version.hpp from git describe"
)


# --------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------

include(FetchContent)

# # ------------------------------------------------
# # libgit2
# # build local copy for static linking
# # strip out all features we're not using: no ssh,https,regex
# # ------------------------------------------------
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(BUILD_CLI OFF CACHE BOOL "" FORCE)
# set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(USE_SSH OFF CACHE BOOL "" FORCE)
# set(USE_HTTPS OFF CACHE BOOL "" FORCE)
# set(USE_HTTP_PARSER "builtin" CACHE STRING "" FORCE)
# set(REGEX_BACKEND "builtin" CACHE STRING "" FORCE)
# set(USE_GSSAPI OFF CACHE BOOL "" FORCE)
# set(USE_NTLMCLIENT OFF CACHE BOOL "" FORCE)
# set(USE_BUNDLED_ZLIB ON CACHE BOOL "" FORCE)
# 
# FetchContent_Declare(
#   libgit2
#   GIT_REPOSITORY https://github.com/libgit2/libgit2.git
#   GIT_TAG        v1.8.4
#   GIT_SHALLOW    TRUE
# )
# 
# FetchContent_MakeAvailable(libgit2)

# ------------------------------------------------
# Catch2
# ------------------------------------------------
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "" FORCE)
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(Catch2)


# --------------------------------------------------------------------
# Library
# build source as library for testing
# --------------------------------------------------------------------

add_library(devprompt_lib
  src/Options.cpp
  src/ShellCommand.cpp
  src/filesystem.cpp
  src/FormatString.cpp
  src/Variable.cpp
  src/VcsModule.cpp
  src/PythonModule.cpp
  src/SysModule.cpp
  src/PathModule.cpp
  src/OsModule.cpp
  src/ShellModule.cpp
  src/GitBackend.cpp
  src/SvnBackend.cpp
  src/styleparser.cpp
  src/runner.cpp
  src/CoreVariables.cpp
  src/FileScanner.cpp
)

target_include_directories(devprompt_lib PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# this is just for Tests to include the headers
target_include_directories(devprompt_lib INTERFACE
  src
)

target_link_libraries(devprompt_lib PRIVATE
  devprompt_warnings
)

if(WIN32)
  target_link_libraries(devprompt_lib PRIVATE
    ws2_32
    winhttp
    rpcrt4
    crypt32
    secur32
    ole32
  )
endif()


# --------------------------------------------------------------------
# Executable
# --------------------------------------------------------------------

add_executable(devprompt
  src/main.cpp
)

# main.cpp needs to see the generated Version.hpp file
target_include_directories(devprompt PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)

add_dependencies(devprompt devprompt_version)

target_link_libraries(devprompt PRIVATE
  devprompt_lib
  devprompt_warnings
)


# --------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------

include(CTest)
include(Catch)

add_executable(devprompt_tests
  tests/Expected.cpp
  tests/FormatString.cpp
  tests/OneShotThreadPool.cpp
  tests/ShellCommand.cpp
  tests/Style.cpp
  tests/jsonparser.cpp
  tests/styleparser.cpp
  tests/utils.cpp
  tests/utils_split_view.cpp
)

target_link_libraries(devprompt_tests PRIVATE
  devprompt_lib
  devprompt_warnings
  Catch2::Catch2WithMain
)

catch_discover_tests(devprompt_tests)


# --------------------------------------------------------------------
# Enable Sanitizers
# --------------------------------------------------------------------

if(ENABLE_SANITIZERS)
  target_link_libraries(devprompt_lib PRIVATE
    devprompt_sanitizers
  )

  target_link_libraries(devprompt PRIVATE
    devprompt_sanitizers
  )

  target_link_libraries(devprompt_tests PRIVATE
    devprompt_sanitizers
  )
endif()






# --------------------------------------------------------------------
# Install
# --------------------------------------------------------------------

include(GNUInstallDirs)

# use component install to skip the installs from libgit2
install(TARGETS devprompt
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT devprompt
)




# 
# ## release
# cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
# cmake --build build/release -j
# ## debug
# cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
# cmake --build build/debug -j
# ## debug+asan
# cmake -S . -B build/asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
# cmake --build build/asan -j
# ## test
# ctest --test-dir build/debug --output-on-failure
# ## clean
# cmake --build build/debug --target clean
# ## install
# sudo cmake --install build/release --component devprompt --strip
#
