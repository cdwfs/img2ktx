cmake_minimum_required(VERSION 3.10)

project(img2ktx)

# requires CMake 3.15, so instead we include /MT and /MTd in CXX flags()
# set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)

add_executable(img2ktx "")
target_sources(img2ktx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/img2ktx.cpp)

if(${MSVC})
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  target_compile_options(img2ktx PRIVATE -W4 -EHsc -wd4996)
  # TODO: replace with CMAKE_MSVC_RUNTIME_LIBRARY in CMake 3.15
  set(CMAKE_CXX_FLAGS_RELEASE "/MT" CACHE TYPE STRING FORCE)
  set(CMAKE_CXX_FLAGS_DEBUG   "/MTd" CACHE TYPE STRING FORCE)
elseif(${UNIX})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
  target_link_libraries(img2ktx PRIVATE m pthread)
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
# This line should be in third_party/CMakeLists.txt, but would require CMake 3.13 (see CMP0079 for details)
target_link_libraries(img2ktx PRIVATE Compressonator)

