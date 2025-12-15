# ============================================================
# EpiGimp2.0 — CMake Configuration
# ============================================================
# Auteur : Apolline Fontaine & Léandre Godet
# Version : 1.0
# Cible : C++20 / Qt6
# Compatibilité : Linux / Windows
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(EpiGimp2.0
    VERSION 1.0
    DESCRIPTION "Projet Epitech : clone simplifié de GIMP en C++/Qt6"
    LANGUAGES CXX
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ============================================================
# Configuration du standard C++
# ============================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Configuration Qt6
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
)

# ============================================================
# Dépendances externes (FetchContent)
# ============================================================

include(FetchContent)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f1c79c02822848a9bed4315b12c8c8f3761e1296
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(stb)

# ============================================================
# Configuration par couche
# ============================================================
set(EPIGIMP_ROOT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src/core)
add_subdirectory(src/io)
add_subdirectory(src/app)
add_subdirectory(src/render)
add_subdirectory(src/ui) # Ici qu'on crée l'exe

# ============================================================
# Tests
# ============================================================
option(BUILD_TESTING "Build unit tests" ON)

if(BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================
# Cibles de lint
# ============================================================
include(${CMAKE_SOURCE_DIR}/lint.cmake)