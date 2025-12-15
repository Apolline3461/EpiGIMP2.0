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
        ${stb_SOURCE_DIR}
)

target_link_libraries(epigimp_io
        PUBLIC
        epigimp_core
        nlohmann_json::nlohmann_json
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Link libzip with proper target detection
# vcpkg provides unofficial::libzip::libzip
# System installations may provide libzip::zip or zip
if(TARGET unofficial::libzip::libzip)
    target_link_libraries(epigimp_io PUBLIC unofficial::libzip::libzip)
    message(STATUS "epigimp_io: Linking with unofficial::libzip::libzip (vcpkg)")
elseif(TARGET libzip::zip)
    target_link_libraries(epigimp_io PUBLIC libzip::zip)
    message(STATUS "epigimp_io: Linking with libzip::zip")
elseif(TARGET zip)
    target_link_libraries(epigimp_io PUBLIC zip)
    message(STATUS "epigimp_io: Linking with zip target")
elseif(LIBZIP_FOUND AND LIBZIP_LIBRARIES)
    # pkg-config found it
    target_include_directories(epigimp_io PUBLIC ${LIBZIP_INCLUDE_DIRS})
    target_link_libraries(epigimp_io PUBLIC ${LIBZIP_LIBRARIES})
    message(STATUS "epigimp_io: Linking with libzip via pkg-config: ${LIBZIP_LIBRARIES}")
else()
    message(FATAL_ERROR "epigimp_io: Could not find any libzip target to link against")
endif()