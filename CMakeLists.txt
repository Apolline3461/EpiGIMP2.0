# ============================================================
# 🖼️ EpiGimp2.0 — CMake Configuration (BOOSTED EDITION)
# ============================================================
# Auteur : Apolline Fontaine & Léandre Godet
# Version : 1.1
# C++17 / Qt6 / Cross-platform / vcpkg compatible
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(EpiGimp2.0
    VERSION 1.1
    DESCRIPTION "Projet Epitech : clone simplifié de GIMP en C++/Qt6"
    LANGUAGES CXX
)

# ============================================================
# Configuration C++ (avec flags MSVC)
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# ============================================================
# Qt6
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)

# ============================================================
# vcpkg auto-support
# (vcpkg integrates automatically via toolchain, no more needed)
# ============================================================

# ============================================================
# libzip (pkg-config or vcpkg)
# ============================================================
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

# ============================================================
# OpenSSL
# ============================================================
find_package(OpenSSL REQUIRED)

# ============================================================
# nlohmann/json — AUTO INSTALL if missing
# ============================================================
include(FetchContent)

message(STATUS "Recherche de nlohmann/json…")

find_package(nlohmann_json QUIET)

if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json introuvable — téléchargement automatique")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
else()
    message(STATUS "nlohmann_json trouvé ✔")
endif()

# ============================================================
# Répertoires source
# ============================================================
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE HDR_FILES ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.h)

# ============================================================
# Cible principale
# ============================================================

add_executable(${PROJECT_NAME}
    tests/main.cpp
    src/EpgFormat.cpp
    src/layer.cpp
    ${HDR_FILES}
)

# Includes
target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    ${SRC_DIR}
    ${LIBZIP_INCLUDE_DIRS}
)

# Link
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    ${LIBZIP_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Linux/macOS: répertoire libs pkgconfig
target_link_directories(${PROJECT_NAME} PRIVATE
    ${LIBZIP_LIBRARY_DIRS}
)

# ============================================================
# Windows : masque la console
# ============================================================
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE YES
    )
endif()

# ============================================================
# Lint
# ============================================================
include(${CMAKE_SOURCE_DIR}/lint.cmake)
