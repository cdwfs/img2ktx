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

# Update build_version.h whenever current commit changes
# c/o https://github.com/rpavlik/cmake-modules/blob/master/GetGitRevisionDescription.cmake
# c/o https://stackoverflow.com/a/4318642
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/)
include(GetGitRevisionDescription)
git_describe(GIT_DESCRIBE_RESULT --tags --always)
configure_file(${CMAKE_CURRENT_LIST_DIR}/build_version.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp @ONLY)
target_sources(img2ktx PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}/build_version.cpp
  ${CMAKE_CURRENT_LIST_DIR}/build_version.h
)

add_subdirectory(third_party)
