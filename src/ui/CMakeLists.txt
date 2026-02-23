# src/ui/CMakeLists.txt

file(GLOB UI_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/ui/*.hpp
)

file(GLOB UI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

list(REMOVE_ITEM UI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

set(UI_RESOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc
)

# ----------------------------
# Lib UI
# ----------------------------
add_library(epigimp_ui STATIC
        ${UI_SOURCES}
        ${UI_HEADERS}
)

target_include_directories(epigimp_ui
        PUBLIC
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(epigimp_ui
        PUBLIC
        epigimp_app
        epigimp_render
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        PRIVATE
        epigimp_warnings
)

# ----------------------------
# Exécutable final
# ----------------------------
add_executable(epigimp
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        ${UI_RESOURCES}
)

target_link_libraries(epigimp
        PRIVATE
        epigimp_ui
        epigimp_warnings
)

if(WIN32)
    set_target_properties(epigimp PROPERTIES WIN32_EXECUTABLE ON)
endif()
