file(GLOB RENDER_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/render/*.h
        ${EPIGIMP_ROOT_INCLUDE_DIR}/render/*.hpp
)

file(GLOB RENDER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(epigimp_render STATIC
        ${RENDER_HEADERS}
        ${RENDER_SOURCES}
)

target_include_directories(epigimp_render
        PUBLIC
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(epigimp_render
        PUBLIC
        epigimp_core
        Qt6::Core
        Qt6::Gui
)