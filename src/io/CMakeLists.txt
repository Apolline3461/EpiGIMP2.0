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

target_link_libraries(epigimp_io
        PUBLIC
        epigimp_core

        # TODO: Remove Qt dependency from epigimp_io.
        # epgformat.cpp must be refactored to use pure core types (no QImage/QFile),
        # Qt-specific code should live in the UI/app layer only.
        Qt6::Core
        Qt6::Gui
)