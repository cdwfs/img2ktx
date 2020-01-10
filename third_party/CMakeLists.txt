cmake_minimum_required(VERSION 2.8)

# Suppress all warnings from external projects.
IF(${MSVC})
    SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS /W0)
    SET(CMAKE_CXX_FLAGS_RELEASE "/MP /MD" CACHE STRING "C++ compiler options (release builds)" FORCE)
    SET(CMAKE_CXX_FLAGS_DEBUG   "/MP /MDd" CACHE STRING "C++ compiler options (debug builds)" FORCE)
    SET(CMAKE_C_FLAGS_RELEASE "/MP /MD" CACHE STRING "C compiler options (release builds)" FORCE)
    SET(CMAKE_C_FLAGS_DEBUG   "/MP /MDd" CACHE STRING "C compiler options (debug builds)" FORCE)
ELSEIF(${UNIX})
    SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
ENDIF()

# Compressonator
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Compressonator/Compressonator/CMP_CompressonatorLib)
SET(COMPRESSONATOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Compressonator/Compressonator/CMP_CompressonatorLib PARENT_SCOPE)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
# "TESTCODE" in this context means SIMDified DXTC compression code.
# It is disabled by default in the Compressonator project, so I will do the same here.
TARGET_COMPILE_DEFINITIONS(Compressonator PRIVATE DISABLE_TESTCODE)
ENDIF()

