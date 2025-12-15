# ============================================================
# lint.cmake — vérification et auto-formatage
# ============================================================

message(STATUS "🔍 Exécution du lint (clang-tidy + clang-format)")

file(GLOB_RECURSE LINT_CPP_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE LINT_HEADER_FILES
    ${CMAKE_SOURCE_DIR}/include/*.hpp
    ${CMAKE_SOURCE_DIR}/include/*.h
)

set(LINT_FORMAT_FILES ${LINT_CPP_FILES} ${LINT_HEADER_FILES})

add_custom_target(format-check
    COMMAND clang-format --dry-run --Werror--style=file ${LINT_FORMAT_FILES}
    COMMENT "Vérification du style de code (clang-format)"
)

add_custom_target(format
    COMMAND clang-format -i --style=file ${LINT_FORMAT_FILES}
    COMMENT "Application automatique du style de code (clang-format)"
)

add_custom_target(tidy
    COMMAND clang-tidy
        --quiet
        -p ${CMAKE_BINARY_DIR}
        -header-filter=.*/(src|include)/.*
        --extra-arg=-std=c++20
        ${LINT_CPP_FILES}
    COMMENT "Analyse statique du code avec clang-tidy"
)

add_custom_target(lint
    DEPENDS format-check tidy
    COMMENT "✅ Vérification complète du code (clang-tidy + clang-format)"
)
