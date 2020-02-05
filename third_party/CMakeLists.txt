cmake_minimum_required(VERSION 3.10)

# Suppress all warnings from external projects.
if(${MSVC})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS /W0)
  SET(CMAKE_CXX_FLAGS_RELEASE "/MT" CACHE TYPE STRING FORCE)
  SET(CMAKE_CXX_FLAGS_DEBUG   "/MTd" CACHE TYPE STRING FORCE)
elseif(${UNIX})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
endif()

# stb
target_include_directories(img2ktx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/stb)
target_sources(img2ktx PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/stb/stb_image.h
  ${CMAKE_CURRENT_LIST_DIR}/stb/stb_image_resize.h
)

# Compressonator
set(COMPRESSONATOR_DIR ${CMAKE_CURRENT_LIST_DIR}/Compressonator/Compressonator/CMP_CompressonatorLib)
add_subdirectory(${COMPRESSONATOR_DIR})
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  # "TESTCODE" in this context means SIMDified DXTC compression code.
  # It is disabled by default in the Compressonator project, so I will do the same here.
  target_compile_definitions(Compressonator PRIVATE DISABLE_TESTCODE)
endif()
# This line should be here instead of the root-level CMakeLists.txt, but would require CMake 3.13 (see CMP0079 for details)
# target_link_libraries(img2ktx PRIVATE Compressonator)
target_include_directories(img2ktx PRIVATE ${COMPRESSONATOR_DIR})
