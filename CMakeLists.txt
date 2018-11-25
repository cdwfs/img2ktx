cmake_minimum_required(VERSION 2.8)

project(img2ktx)

add_subdirectory(third_party)

ADD_EXECUTABLE(img2ktx ${CMAKE_CURRENT_SOURCE_DIR}/img2ktx.cpp)

IF(${MSVC})
    TARGET_COMPILE_OPTIONS(img2ktx PRIVATE -W4 -MT -EHsc -wd4996)
ELSEIF(${UNIX})
    SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -w)
    TARGET_LINK_LIBRARIES(img2ktx PRIVATE m)
ENDIF()

FIND_LIBRARY(IMG2KTX_ISPC_TEXCOMP_LIB
    ispc_texcomp
    PATHS ${CMAKE_CURRENT_SOURCE_DIR} # Finds the pre-build Windows binary as a last resort
    DOC "the ispc_texcomp library to link against"
    )
TARGET_LINK_LIBRARIES(img2ktx PRIVATE ${IMG2KTX_ISPC_TEXCOMP_LIB})