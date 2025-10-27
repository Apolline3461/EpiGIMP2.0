# ============================================================
# 🖼️ EpiGimp2.0 — CMake Configuration
# ============================================================
# Auteur : Apolline Fontaine & Léandre Godet
# Version : 1.0
# Cible : C++17 / Qt6
# Compatibilité : Linux / Windows
# ============================================================

cmake_minimum_required(VERSION 3.20)

project(EpiGimp2.0
    VERSION 1.0
    DESCRIPTION "Projet Epitech : clone simplifié de GIMP en C++/Qt6"
    LANGUAGES CXX
)

# ============================================================
# Configuration du standard C++
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Configuration Qt6
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Recherche des packages Qt6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Svg
)

# ============================================================
# Configuration des répertoires
# ============================================================
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# Récupération des fichiers sources
file(GLOB_RECURSE SRC_FILES ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE HDR_FILES ${INCLUDE_DIR}/*.hpp ${INCLUDE_DIR}/*.h)

# ============================================================
# Création de l'exécutable
# ============================================================
add_executable(EpiGimp2.0
    ${SRC_FILES}
    ${HDR_FILES}
)

# Configuration des répertoires d'inclusion
target_include_directories(EpiGimp2.0 PRIVATE
    ${INCLUDE_DIR}
    ${SRC_DIR}
)

# Liaison avec les bibliothèques Qt6
target_link_libraries(EpiGimp2.0 PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

# ============================================================
# Configuration spécifique Windows
# ============================================================
if(WIN32)
    set_target_properties(EpiGimp2.0 PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()

