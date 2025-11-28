file(GLOB APP_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/app/*.h
        ${EPIGIMP_ROOT_INCLUDE_DIR}/app/*.hpp
)

file(GLOB APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

add_library(epigimp_app STATIC
        ${APP_HEADERS}
        ${APP_SOURCES}
)

target_include_directories(epigimp_app
        PUBLIC
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)

target_link_libraries(epigimp_app
        PUBLIC
        epigimp_core
        epigimp_io
)