#
# GenerateVersionHeader.cmake
#

set(DEVPROMPT_VERSION "unknown")

if(EXISTS "${SRC_ROOT}/.git")
    execute_process(
        COMMAND git describe --tags --match "v[0-9]*" --always --dirty
        WORKING_DIRECTORY "${SRC_ROOT}"
        OUTPUT_VARIABLE DEVPROMPT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    string(REGEX REPLACE "^v" "" DEVPROMPT_VERSION "${DEVPROMPT_VERSION}")
endif()

configure_file(
    "${SRC_ROOT}/src/Version.hpp.in"
    "${BIN_ROOT}/generated/Version.hpp"
    @ONLY
)
