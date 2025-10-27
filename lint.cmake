# ============================================================
# lint.cmake — vérification et auto-formatage
# ============================================================

message(STATUS "🔍 Exécution du lint (clang-tidy + clang-format)")

file(GLOB_RECURSE LINT_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/include/*.h
)

# 1️⃣ Vérification du format (sans modifier)
add_custom_target(format-check
    COMMAND clang-format --dry-run --Werror ${LINT_FILES}
    COMMENT "🔎 Vérification du style de code (clang-format)"
)

# 2️⃣ Auto-formatage
add_custom_target(format
    COMMAND clang-format -i ${LINT_FILES}
    COMMENT "✨ Application automatique du style de code (clang-format)"
)

# 3️⃣ Analyse statique clang-tidy
add_custom_target(tidy
    COMMAND clang-tidy
        --quiet
        -p ${CMAKE_BINARY_DIR}
        --extra-arg-before=-Qunused-arguments
        -extra-arg=-I${CMAKE_SOURCE_DIR}/include
        ${LINT_FILES}
    COMMENT "🧠 Analyse statique du code avec clang-tidy"
)

# 4️⃣ Cible globale lint
add_custom_target(lint
    DEPENDS format-check tidy
    COMMENT "✅ Vérification complète du code (clang-tidy + clang-format)"
)
