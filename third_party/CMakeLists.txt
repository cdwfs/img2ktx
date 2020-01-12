# Suppress all warnings from external projects.
if(${MSVC})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS /W0)
elseif(${UNIX})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
endif()

# stb
target_include_directories(img2ktx PRIVATE ${CMAKE_CURRENT_LIST_DIR}/stb)
target_sources(img2ktx PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/stb/stb_image.h
  ${CMAKE_CURRENT_LIST_DIR}/stb/stb_image_resize.h
)
