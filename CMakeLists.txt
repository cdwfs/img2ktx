cmake_minimum_required(VERSION 2.8)

project(img2ktx)

add_subdirectory(third_party)

IF(${MSVC})
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)
    SET(CMAKE_CXX_FLAGS "/MP /GS- /GR- /W4 /WX /wd4996 /EHsc /D _HAS_EXCEPTIONS=0" CACHE STRING "C++ compiler options (all builds)" FORCE)
    SET(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob1 /Oi /fp:fast /D NDEBUG" CACHE STRING "C++ compiler options (release builds)" FORCE)
    SET(CMAKE_CXX_FLAGS_DEBUG   "/MTd /Od /Ob0 /Zi /D _DEBUG" CACHE STRING "C++ compiler options (debug builds)" FORCE)
    ADD_DEFINITIONS(-D_UNICODE -DUNICODE -D_CRT_SECURE_NO_WARNINGS -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
ELSEIF(${UNIX})
    SET(CMAKE_CXX_FLAGS "-Wall -Werror -march=native -msse4.1 -D_POSIX_C_SOURCE=199309L -DM_PI=3.141592653 -std=c++11 -Wno-unused-function" CACHE STRING "C++ compiler options (all builds)" FORCE)
    SET(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG" CACHE STRING "C++ compiler options (release builds)" FORCE)
    SET(CMAKE_CXX_FLAGS_DEBUG   "-D_DEBUG -g" CACHE STRING "C++ compiler options (debug builds)" FORCE)
    SET(CMAKE_EXE_LINKER_FLAGS  "-pthread" CACHE STRING "C++ linker options (all builds)" FORCE)
ENDIF()

ADD_EXECUTABLE(img2ktx ${CMAKE_CURRENT_SOURCE_DIR}/img2ktx.cpp)

IF(${UNIX})
    SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
    TARGET_LINK_LIBRARIES(img2ktx PRIVATE m)
ENDIF()

TARGET_LINK_LIBRARIES(img2ktx PRIVATE Compressonator)