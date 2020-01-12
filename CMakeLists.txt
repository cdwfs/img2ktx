cmake_minimum_required(VERSION 3.15)

project(img2ktx)

set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>) # requires CMake 3.15

add_executable(img2ktx "")
target_sources(img2ktx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/img2ktx.cpp)

if(${MSVC})
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  target_compile_options(img2ktx PRIVATE -W4 -EHsc -wd4996)
elseif(${UNIX})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
  target_link_libraries(img2ktx PRIVATE m)
endif()

find_library(IMG2KTX_ISPC_TEXCOMP_LIB
  ispc_texcomp
  PATHS ${CMAKE_CURRENT_SOURCE_DIR} # Finds the pre-build Windows binary as a last resort
  DOC "the ispc_texcomp library to link against"
)
target_link_libraries(img2ktx PRIVATE ${IMG2KTX_ISPC_TEXCOMP_LIB})
