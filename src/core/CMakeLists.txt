
file(GLOB CORE_HEADERS
        ${EPIGIMP_ROOT_INCLUDE_DIR}/core/*.h
        ${EPIGIMP_ROOT_INCLUDE_DIR}/core/*.hpp
        ${EPIGIMP_ROOT_INCLUDE_DIR}/core/tools/*.h
        ${EPIGIMP_ROOT_INCLUDE_DIR}/core/tools/*.hpp
)

file(GLOB CORE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/*.cpp
)

add_library(epigimp_core STATIC
        ${CORE_HEADERS}
        ${CORE_SOURCES}
)

target_include_directories(epigimp_core
        PUBLIC
        ${EPIGIMP_ROOT_INCLUDE_DIR}
)