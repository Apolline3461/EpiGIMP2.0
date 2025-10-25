# 🖌️ EpiGimp2.0

**EpiGimp2.0** est un éditeur d’images libre et multiplateforme inspiré de GIMP. 
Développé en **C++20 / Qt 6**, il permet la création, la retouche et la gestion de calques d’images au format **PNG/JPEG** ainsi qu’un format de projet propriétaire **.epg**.

---

## 📋 Sommaire
- [Présentation](#-présentation)
- [Fonctionnalités](#-fonctionnalités)
- [Format `.epg`](#-format-epg)
- [Installation](#-installation)
- [Licence](#-licence)

---

## Présentation

> Projet réalisé dans le cadre d’Epitech
> Par **Apolline Fontaine** et **Léandre Godet**

**Objectif :** livrer un **MVP réaliste** d’éditeur d’images, fluide et robuste, en 10 semaines de développement.

---

## Fonctionnalités

### Must have
- **Fichiers**
 - Nouveau / Ouvrir (PNG, JPEG)
 - Enregistrer projet `.epg`
 - Export PNG/JPEG
- **Calques**
 - Créer / Supprimer / Dupliquer / Réordonner
 - Visibilité / Opacité / Verrouillage
- **Outils**
 - Pinceau (taille, dureté, couleur)
 - Gomme (transparence)
 - Sélection rectangulaire (add/subtract)
 - Pot de peinture (tolérance)
- **Historique**
 - Annuler / Rétablir multi-niveaux (≥ 20 actions)

### Should have
- Pipette (prélever couleur du rendu composite)
- Texte (calque texte)
- Zoom / Pan / Checkerboard de transparence
- Raccourcis clavier (type GIMP)

### Could have
- Sélection libre (lasso)
- Export WebP (qualité réglable)
- Outil courbes

---

## Format `.epg`

Format propriétaire et textuel, encodé en UTF-8, inspiré de `.xcf`.

### Contenu typique

a détailler

## Installation

### Prérequis

CMake ≥ 3.25
Qt 6.7.x
C++20

### Build

```bash

git clone https://github.com/Apolline3461/EpiGIMP2.0.git
cd EpiGIMP2.0

mkdir build && cd build

cmake ..
make
./EpiGimp
```

## Raccourcis principaux

| Action | Raccourci |
| --- | --- |
| Nouveau projet | Ctrl + N |
| Ouvrir | Ctrl + O |
| Enregistrer | Ctrl + S |
| Exporter | Ctrl + Maj + E |
| Annuler / Rétablir | Ctrl + Z / Ctrl + Y |
| Pinceau | P |
| Gomme | Maj + E |
| Pot de peinture | Maj + B |
| Pipette | O |
| Texte | T |
| Zoom +/- | Ctrl + + / Ctrl + - |
| Fermer | Ctrl + W |

## Licence

EpiGimp2.0 est distribué sous licence MIT.
Vous êtes libres de l’utiliser, modifier et redistribuer le projet à des fins pédagogiques ou personnelles.
