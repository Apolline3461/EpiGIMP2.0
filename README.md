# EpiGimp2.0

**EpiGimp2.0** est un éditeur d’images libre et multiplateforme inspiré de GIMP.
Développé en **C++20 / Qt 6**, il permet la création, la retouche et la gestion de calques d’images au format **PNG/JPEG** ainsi qu’un format de projet propriétaire **.epg**.

---

## Sommaire

- [Présentation](#-présentation)
- [Fonctionnalités actuelles](#fonctionnalités-actuelles)
- [Format `.epg`](#format-epg)
- [Installation](#installation)
    - [Télécharger une Release](#télécharger-une-release-recommandé)
    - [Build depuis les sources](#build-depuis-les-sources)
- [Prise en main](#prise-en-main)
- [Licence](#-licence)

---

## Présentation

> Projet réalisé dans le cadre d’Epitech
> Par **Apolline Fontaine** et **Léandre Godet**

**Objectif :** livrer un **MVP réaliste** d’éditeur d’images, fluide et robuste, avec un socle technique propre (CI, tests, architecture adaptée).

---

## Fonctionnalités actuelles

### Gestion de fichiers

- Nouveau projet : création d’un canvas vierge (dimensions choisies).
- Ouvrir : import d’images PNG / JPEG.
- Enregistrer / Ouvrir : format projet .epg.
- Exporter : export en PNG / JPEG.

### Calques

- Ajout de calques (vides ou à partir d'une image).
- Suppression et réorganisation des calques (glisser / déposer dans le panneau Calques).
- Gestion de l'opacité, du verrouillage et du nom pour chaque calque.
- Fusion vers le bas.

### Sélection rectangulaire

- Sélection pour limiter l'action des outils au sein de la sélection.
- Effacer la sélection via le bouton "Effacer la sélection" ou la touche Échap.

### Pot de peinture

- Outil pot de peinture qui remplit une zone avec une seule couleur.
- Palette de couleurs commune aux outils.

### Historique

- Annuler / Rétablir : icônes + raccourcis (Ctrl+Z / Ctrl+Y).
- Limité à 20 actions d'annulation par défaut.

---

> Le reste des fonctionnalités arrive progressivement.
> Ce README se limite volontairement aux fonctionnalités déjà implémentées.
> La liste complète des fonctionnalités visées est détaillée dans le [cahier des charges](./docs/EPIGIMP2.0%20cdc.pdf) du repo.

---

## Format `.epg`

Le `.epg` est le format de projet d’EpiGimp2.0 : il permet de sauvegarder un projet non destructif afin de le réouvrir à l’identique.

Techniquement, un fichier `.epg` est une **archive ZIP** contenant :

- `project.json` : description du projet (canvas, calques, métadonnées…)
- `preview.png` : aperçu du rendu (256×256)
- `layers/NNNN.png` : images des calques (identifiants sur 4 chiffres : `0001`, `0042`, …)

Plus d’informations : voir la **[documentation technique du format `.epg`](./docs/epgformat.md)**.

---

## Installation

### Télécharger une Release (recommandé)

Les versions prêtes à l’emploi sont disponibles dans **[GitHub Releases](https://github.com/Apolline3461/EpiGIMP2.0/releases/)** :

- **Linux** : `epigimp-linux-x86_64.AppImage`
- **Windows** : `epigimp-windows-x86_64.zip` (portable)

> Les releases sont générées automatiquement lors d’un push de tag `v*` (ex: `v1.1.0`).

#### Linux (AppImage)

1. Télécharge `epigimp-linux-x86_64.AppImage`
2. Rends le fichier exécutable :
   ````bash
   chmod +x epigimp-linux-x86_64.AppImage
   ./epigimp-linux-x86_64.AppImage```
   ````

#### Windows (Zip portable)

1. Télécharge `epigimp-windows-x86_64.zip`
2. Décompresse le zip
3. Lance epigimp.exe

### Build depuis les sources

#### Prérequis

- CMake ≥ 3.20 (minimum du projet)
- Compilateur C++ compatible C++20
- Qt 6.x (Core, Gui, Widgets, Svg)
- (Linux) Ninja recommandé

#### Etapes à suivre

```bash
git clone --recurse-submodules https://github.com/Apolline3461/EpiGIMP2.0.git
cd EpiGIMP2.0

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

./build/bin/epigimp
```

### Tests & CI

- Lancer un build de développement et la suite de tests unitaires localement :

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

- Le dépôt contient une configuration GitHub Actions qui build et lance les tests automatiquement sur les pull request de dev et main.

---

## Prise en main

Une version plus détailler d'une prise en main : [Utilisateur](docs/Utilisateur.md)

### Raccourcis principaux

| Action             | Raccourci           |
| ------------------ | ------------------- |
| Nouveau projet     | Ctrl + N            |
| Ouvrir             | Ctrl + O            |
| Enregistrer        | Ctrl + S            |
| Exporter           | Ctrl + Maj + E      |
| Annuler / Rétablir | Ctrl + Z / Ctrl + Y |
| Pinceau            | P                   |
| Gomme              | Maj + E             |
| Pot de peinture    | Maj + B             |
| Pipette            | O                   |
| Texte              | T                   |
| Zoom +/-           | Ctrl + + / Ctrl + - |
| Fermer             | Ctrl + W            |

---

## Licence

EpiGimp2.0 est distribué sous licence MIT.
Vous êtes libres de l’utiliser, modifier et redistribuer le projet à des fins pédagogiques ou personnelles.

Information contribution : **[Contribution](./CONTRIBUTING.md)**.

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
[![🖼️ Build EpiGimp2.0](https://github.com/Apolline3461/EpiGIMP2.0/actions/workflows/github-actions-build.yaml/badge.svg?branch=main)](https://github.com/Apolline3461/EpiGIMP2.0/actions/workflows/github-actions-build.yaml)

[![Coverage (Codecov)](https://github.com/Apolline3461/EpiGIMP2.0/actions/workflows/coverage.yaml/badge.svg?branch=main)](https://github.com/Apolline3461/EpiGIMP2.0/actions/workflows/coverage.yaml)
[![codecov](https://codecov.io/gh/Apolline3461/EpiGIMP2.0/branch/main/graph/badge.svg)](https://codecov.io/gh/Apolline3461/EpiGIMP2.0)
