# ============================================================
# EpiGimp2.0 — CMake Configuration
# ============================================================
# Auteur : Apolline Fontaine & Léandre Godet
# Version : 1.1
# Cible : C++20 / Qt6
# Compatibilité : Linux / Windows
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(EpiGimp2.0
    VERSION 1.1
    DESCRIPTION "Projet Epitech : clone simplifié de GIMP en C++/Qt6"
    LANGUAGES CXX
)

# ------------------------------------------------------------
# Global CMake behavior
# ------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# ------------------------------------------------------------
# C++ Configuration
# ------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(
        /W3
        /permissive-
        /Zc:preprocessor
        /EHsc
        /utf-8
    )
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ------------------------------------------------------------
# Qt6
# ------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
)

# ------------------------------------------------------------
# FetchContent dependencies
# ------------------------------------------------------------
include(FetchContent)

# ---- nlohmann/json ----
find_package(nlohmann_json QUIET CONFIG)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json introuvable — téléchargement automatique")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
else()
    message(STATUS "nlohmann_json trouvé ✔")
endif()

# ---- stb ----
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f1c79c02822848a9bed4315b12c8c8f3761e1296
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# ------------------------------------------------------------
# ZLIB (explicit — REQUIRED)
# ------------------------------------------------------------
find_package(ZLIB REQUIRED)

# ------------------------------------------------------------
# OpenSSL
# ------------------------------------------------------------
find_package(OpenSSL REQUIRED)

# ------------------------------------------------------------
# libzip
# ------------------------------------------------------------
find_package(libzip CONFIG QUIET)

if(NOT libzip_FOUND)
    message(STATUS "libzip introuvable via packages — FetchContent en dernier recours")

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

# ------------------------------------------------------------
# Project structure
# ------------------------------------------------------------
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src/core)
add_subdirectory(src/io)
add_subdirectory(src/app)
add_subdirectory(src/render)
add_subdirectory(src/ui)

# Tests
option(BUILD_TESTING "Build unit tests" ON)
if(BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

# Lint
include(${CMAKE_SOURCE_DIR}/lint.cmake)
