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
        epigimp_warnings
)

# OpenSSL: link with imported targets when available, else fallback to variables
if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(epigimp_io PUBLIC OpenSSL::SSL OpenSSL::Crypto)
elseif(OPENSSL_FOUND)
    target_include_directories(epigimp_io PUBLIC ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(epigimp_io PUBLIC ${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "epigimp_io: OpenSSL introuvable — requis pour lier epigimp_io")
endif()

# libzip: try system CMake config first, fallback to FetchContent
find_package(libzip CONFIG QUIET)

if(NOT libzip_FOUND)
    message(STATUS "libzip introuvable — utilisation de FetchContent en dernier recours")
    include(FetchContent)

    FetchContent_Declare(
        libzip
        GIT_REPOSITORY https://github.com/nih-at/libzip.git
        GIT_TAG v1.11.4
        GIT_SHALLOW TRUE
    )

    # libzip options (important)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    set(ENABLE_COMMONCRYPTO OFF CACHE BOOL "" FORCE)
    set(ENABLE_GNUTLS OFF CACHE BOOL "" FORCE)
    set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
    set(ENABLE_OPENSSL ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(libzip)
else()
    message(STATUS "libzip trouvé ✔")
endif()

# Link libzip with proper target detection
# Priority: pkg-config (Linux) > vcpkg (Windows) > system CMake config > FetchContent
if(LIBZIP_FOUND AND LIBZIP_LIBRARIES)
    # pkg-config found it (most reliable on Linux)
    target_include_directories(epigimp_io PUBLIC ${LIBZIP_INCLUDE_DIRS})
    target_link_libraries(epigimp_io PUBLIC ${LIBZIP_LIBRARIES})
    message(STATUS "epigimp_io: Linking with libzip via pkg-config: ${LIBZIP_LIBRARIES}")
elseif(TARGET unofficial::libzip::libzip)
    # vcpkg provides this
    target_link_libraries(epigimp_io PUBLIC unofficial::libzip::libzip)
    message(STATUS "epigimp_io: Linking with unofficial::libzip::libzip (vcpkg)")
elseif(TARGET libzip::zip)
    target_link_libraries(epigimp_io PUBLIC libzip::zip)
    message(STATUS "epigimp_io: Linking with libzip::zip")
elseif(TARGET zip)
    target_link_libraries(epigimp_io PUBLIC zip)
    message(STATUS "epigimp_io: Linking with zip target")
else()
    message(FATAL_ERROR "epigimp_io: Could not find any libzip target to link against")
endif()