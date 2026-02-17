# src/ui/CMakeLists.txt

file(GLOB UI_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/ui/*.hpp
)

file(GLOB UI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        resources.qrc
)

add_executable(epigimp
        ${UI_SOURCES}
        ${UI_HEADERS}
)

target_include_directories(epigimp
        PRIVATE
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(epigimp
        PRIVATE
        epigimp_app
        epigimp_render
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        epigimp_warnings
)

if(WIN32)
    set_target_properties(epigimp PROPERTIES
            WIN32_EXECUTABLE ON
    )
endif()