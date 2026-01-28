# src/ui/CMakeLists.txt

set(UI_SOURCES
        main.cpp
        window.cpp
        image.cpp
        resources.qrc
)

set(UI_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/ui/window.hpp
        ${EPIGIMP_ROOT_INCLUDE_DIR}/ui/image.hpp
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