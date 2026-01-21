# CI / CD — Documentation (GitHub Actions)

Ce document explique toute la CI/CD du repo EpiGimp2.0 : objectifs, workflows, étapes, caches, artefacts et comment reproduire les checks en local.

> Emplacement : `.github/workflows/`  
> Workflows typiques : `build.yml`, `lint.yml`, `coverage.yml`, `release.yml`

---

## 1) Vue d’ensemble

La CI/CD est découpée en 4 workflows principaux :

1. **Build**  
   Compile l’application (et les tests) sur les environnements cibles.
2. **Lint / Code Quality**  
   Vérifie format, style, analyse statique (clang-format, clang-tidy, cppcheck), et garde-fous (CRLF).
3. **Coverage**  
   Exécute les tests et publie la couverture (ex: Codecov).
4. **Release**  
   Construit et publie des artefacts “release” (AppImage / zip Windows) lors d’un tag `v*`.

Chaque workflow est indépendant mais cohérent :
- Le workflow **Lint** installe Qt, configure CMake, build (pour générer `moc/uic` + `compile_commands.json`), puis exécute les outils d’analyse.
- Le workflow **Coverage** compile en mode coverage et lance les tests.
- Le workflow **Release** compile en mode Release, package et upload.

---

## 2) Triggers

### Lint (exemple)
Souvent déclenché sur :
- `push` sur toutes branches
- `pull_request` vers `main`/`dev`
- `workflow_dispatch` (manuel)

Exemple (présent dans le repo) :
```yaml
on:
  push:
    branches: ["**"]
  pull_request:
    branches: [ main, dev ]
  workflow_dispatch:
```
## 3) Workflow Lint / Code Quality

Objectif : bloquer ce qui doit l’être (format / erreurs d’analyse), et fournir du feedback actionnable.

**3.1 Checkout**
```yaml
- uses: actions/checkout@v4
  with:
      fetch-depth: 0
      submodules: recursive
```

fetch-depth: 0 : utile pour git diff et les calculs “changed files”

submodules: recursive : nécessaire si le projet a des submodules

**3.2 “Scope”** 

Analyser seulement les fichiers modifiés

Le job calcule une liste de fichiers :
- format_files.bin : fichiers à passer à clang-format
- cpp_files.bin : .cpp à passer à clang-tidy / cppcheck

**3.2 Installation des outils**

```bash
sudo apt-get update
sudo apt-get install -y clang-format clang-tidy cppcheck cmake ninja-build
```

## 4) Coverage

Ce workflow calcule la **couverture de tests** et l’envoie à **Codecov**.  
Il s’exécute uniquement :
- sur **Pull Request** vers `main` ou `dev`
- en **manuel** (`workflow_dispatch`)

### Étapes

1. **Checkout**
    - récupère le repo et les submodules

2. **Installation des dépendances**
    - outils de build : `cmake`, `ninja-build`
    - outil couverture : `gcovr`
    - dépendances runtime nécessaires à Qt (XCB/GL), pour exécuter les tests en CI headless

3. **Configuration CMake (coverage)**
    - build séparé dans `build-cov/`
    - `-DENABLE_COVERAGE=ON` active les flags de couverture
    - `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` exporte aussi la compile database (utile pour debug/outils)

4. **Build**
    - compilation via Ninja

5. **Tests**
    - exécution via `ctest` avec logs en cas d’échec (`--output-on-failure`)

6. **Génération du rapport**
    - `gcovr` génère `coverage.xml` (format Cobertura/XML)
    - exclusions pour ne pas compter :
        - répertoires de build (`build-cov/`, `build/`, `cmake-build-*`)
        - dépendances téléchargées (`_deps/`)
        - tests (`tests/`) — volontaire pour se concentrer sur le code produit

7. **Upload artifact**
    - `coverage.xml` est uploadé en artefact GitHub (conservé 7 jours)

8. **Upload Codecov**
    - envoi de `coverage.xml` à Codecov via `codecov/codecov-action@v5`
    - la CI échoue si l’upload échoue (`fail_ci_if_error: true`)

### Fichiers produits
- `coverage.xml` : rapport de couverture (artefact + envoyé à Codecov)


### Codecov configuration (`codecov.yml`)

La couverture est mesurée sur les Pull Requests via `gcovr` et envoyée à Codecov.
La configuration est définie dans `codecov.yml` (dossiers ignorés, seuils *project* et *patch*).

- `project` : suit l’évolution globale de la couverture par rapport à la branche cible (target `auto`).
  Tolérance : -0.30% pour éviter le bruit.
- `patch` : couverture minimale exigée sur le code ajouté/modifié dans la PR (target 60%).
- Dossiers ignorés : build/, build-cov/, _deps/, tests/, packaging/, .github/, etc.
- `require_ci_to_pass: true` : Codecov ne prend en compte la mesure que si la CI est verte.
