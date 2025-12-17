file(GLOB IO_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/io/*.h
        ${EPIGIMP_ROOT_INCLUDE_DIR}/io/*.hpp
)
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
)

target_include_directories(epigimp_io
        SYSTEM PRIVATE
        ${stb_SOURCE_DIR}
)

target_link_libraries(epigimp_io
        PUBLIC
        epigimp_core
)