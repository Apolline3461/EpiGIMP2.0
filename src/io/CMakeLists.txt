file(GLOB IO_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/io/*.h
        ${EPIGIMP_ROOT_INCLUDE_DIR}/io/*.hpp
)
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(stb)

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()


file(GLOB IO_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(epigimp_io STATIC
        ${IO_HEADERS}
        ${IO_SOURCES}
)

target_include_directories(epigimp_io
        PUBLIC
        ${EPIGIMP_ROOT_INCLUDE_DIR}
        ${stb_SOURCE_DIR}
)

target_link_libraries(epigimp_io
        PUBLIC
        epigimp_core
        # removed ${stb_SOURCE_DIR} here: stb is header-only and should not be linked as a directory
)