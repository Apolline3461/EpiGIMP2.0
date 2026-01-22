# Contribution

Ce document décrit comment installer l’environnement, builder/tester le projet, respecter le style.

---
## 1) Pré-requis

### Outils obligatoires
- **CMake** (≥ 3.20)
- **Ninja** (recommandé)
- **Compilateur C++20** (clang++ ou g++)
- **Qt 6.x** (Core, Gui, Widgets, Svg)
- **Git** (+ submodules)

### Dépendances projet
Le repo utilise `vcpkg.json` pour gérer certaines libs.  
Assure-toi de cloner avec les submodules :

```bash
git clone --recurse-submodules https://github.com/Apolline3461/EpiGIMP2.0.git
cd EpiGIMP2.0
```
Si tu as déjà cloné sans submodules :

```bash
git submodule update --init --recursive
```

(Optionnel) Outils qualité (recommandés)
- clang-format 
- clang-tidy 
- cppcheck 
- pre-commit

## 2) Build et tests

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/epigimp

./build/bin/test_${name_of_the_layer}
```

## 3) Branches et Pull Request

### Branches
**main** : stable / release-ready
**dev** : intégration continue
**feature branches** : feature/<topic>, fix/<topic>, docs/<topic>

### Règles PR

PR obligatoire pour merger dans dev / main

Une PR doit :
- compiler 
- passer les tests 
- respecter le format (clang-format)
- ne pas introduire de warnings CI bloquants

#### Bonnes pratiques

1 PR = 1 sujet (éviter les PR “fourre-tout”)

Commits clairs

Décrire dans la PR :
- le contexte
- le changement
- comment tester

## 4) Pre-commit (recommandé)
Objectif : éviter les échecs dû au clang format lors des CI.

### Installer pre-commit

> **Ubuntu / Debian**
> ```bash
> python3 -m pip install --user pre-commit
> ```

> **Arch**
> ```bash
> sudo pacman -S python-pipx
> pipx ensurepath
> pipx install pre-commit
> ```

### Activé le hook dans le repo

```bash
pre-commit install
pre-commit run --all-files
```

## 5) Tag et Version

Les releases sont déclenchées par push d’un tag `v*` (ex: `v1.2.0`).
Plus d’informations : voir la **[documentation release](./docs/releases.md)**.
