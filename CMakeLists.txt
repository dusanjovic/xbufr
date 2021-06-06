cmake_minimum_required(VERSION 3.5)

file(STRINGS "VERSION" VERSION_STRING)

project(xbufr VERSION ${VERSION_STRING})

list(APPEND CMAKE_MODULE_PATH "cmake")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Wpedantic -Wunknown-pragmas -Wmissing-declarations -Wshadow")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wredundant-decls -Wpointer-arith -Wundef -Wbuiltin-macro-redefined -Wdiv-by-zero")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wignored-qualifiers -Wunreachable-code")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdisabled-optimization -Wformat=2 -Wformat-security -Winit-self -Wmissing-include-dirs -Wno-sign-conversion")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Woverloaded-virtual")

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Compiler is GNU")
    # profiling
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -no-pie")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -no-pie")
    #set(CMAKE_LFLAGS "${CMAKE_LFLAGS} -pg -no-pie")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always -Wcast-align -Wlogical-op -Wstrict-null-sentinel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfloat-conversion -Wdate-time ")
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Compiler is Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-defaulted-function-deleted")
  endif()

endif() # GNU or Clang

if(CMAKE_C_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996") #  warning C4996: 'getenv': This function or variable may be unsafe.
endif() # MSVC

# sanitizing
set(XBUFR_SANITIZE OFF)
if(XBUFR_SANITIZE)
  set(sanitize_flags "${sanitize_flags} -fsanitize=undefined -fsanitize=address -fno-omit-frame-pointer")
  #set(sanitize_flags "${sanitize_flags} -fsanitize=leak")
  #set(sanitize_flags "-fsanitize=thread")
  #set(sanitize_flags "${sanitize_flags} -fsanitize=safe-stack")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${sanitize_flags}")
  set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${sanitize_flags}")
endif()

if(UNIX AND NOT APPLE)
  include("cmake/os_release_info.cmake")
  get_os_release_info(os_name os_version os_codename)
  message(STATUS "OS name:     ${os_name}")
  message(STATUS "OS version:  ${os_version}")
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  if("${os_name}" STREQUAL "alpine" OR WIN32)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -no-pie")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie -static -static-libgcc -static-libstdc++ -Wl,-O1 -Wl,--gc-sections")
  endif()
endif()

enable_testing()

set(GIT_REVISION "Unknown")
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=40
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_REVISION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message(STATUS "GIT hash:    ${GIT_REVISION}")
else()
  message(STATUS "GIT not found")
endif()

configure_file(
    "${PROJECT_SOURCE_DIR}/apps/xbufr/version.h.in"
    "${PROJECT_SOURCE_DIR}/apps/xbufr/version.h"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(libs)
add_subdirectory(apps)
