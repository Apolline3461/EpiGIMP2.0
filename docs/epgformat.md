# Documentation technique du format `.epg`

**Version du format :** 1.0
**Date :** 2025-11-11
**Auteur :** Léandre Godet
**Statut :** Documentation interne

---

## Table des matières

1. [Intro](#1-intro)
2. [Format .epg](#2-Format-epg)
3. [Structure du project.json](#3-structure-du-projectjson)
4. [Description détaillée des sections](#4-description-détaillée-des-sections)
5. [Licence](#5-licence)
6. [Annexes](#annexes)

---

## 1. Intro

Le format **.epg** (_EPIGIMP2.0_) est un format de fichier **non destructif et modulaire** destiné à la sauvegarde de projets graphiques complexes.

Il préserve l'état complet du projet (calques, transformations, opacités, modes de fusion, paramètres colorimétriques, métadonnées…) afin de permettre une **réouverture à l'identique**.

Ce format se veut **minimaliste, contrôlable et extensible**, comparable au `.xcf` de GIMP ou au `.psd` d'Adobe Photoshop, mais spécifiquement adapté à l'architecture interne d'EpiGimp 2.0.

---

## 2. Format .epg

Un fichier `.epg` est une **archive compressée (ZIP)** contenant :

```
project.epg
├── project.json          # Structure principale du projet
├── preview.png           # [Optionnel] Aperçu rapide du rendu
└── layers/
    ├── 0001.png
    ├── 0002.png
    ├── 0003_mask.png     # [Optionnel] Masque de calque
    └── ...
```

### 2.1. Conventions de nommage

- **Identifiants de calques** : format `NNNN` (4 chiffres, ex. `0001`, `0042`)
- **Fichiers de calques** : `NNNN.png` ou `NNNN.tiff`
- **Masques de calques** : `NNNN_mask.png`
- **Prévisualisation** : `preview.png` (recommandé 256×256px max)

### 2.2. Compression

- **Méthode** : ZIP
- **Extension** : `.epg`
- Tous les chemins sont relatifs à la racine de l’archive

---

## 3. Structure du `project.json`

Le cœur du format `.epg` repose sur un fichier JSON UTF-8 structuré comme suit :

```json
{
  "epg_version": 1,
  "manifest": {
    "entries": [
        { "path": "project.json", "sha256": "a3f23c3b..." },
        { "path": "layers/0001.png", "sha256": "0ff3c2..." }
    ],
    "file_count": 5,
    "generated_utc": "2025-11-11T15:00:00Z"
  },
  "canvas": {
    "name": "EpiGimp2.0",
    "width": 1920,
    "height": 1080,
    "dpi": 72,
    "color_space": "sRGB",
    "background": { "r": 255, "g": 255, "b": 255, "a": 0 }
  },

  "layers": [
    {
      "id": "0001",
      "name": "Arrière-plan",
      "type": "raster",
      "visible": true,
      "locked": false,
      "opacity": 1.0,
      "blend_mode": "normal",
      "transform": {
        "tx": 0,
        "ty": 0,
        "scaleX": 1.0,
        "scaleY": 1.0,
        "rotation": 0.0,
        "skewX": 0.0,
        "skewY": 0.0
      },
      "bounds": {
        "x": 0,
        "y": 0,
        "width": 1920,
        "height": 1080
      },
      "path": "layers/0001.png",
      "masks": [
        {
          "type": "alpha",
          "path": "layers/0001_mask.png",
          "mode": "multiply",
          "opacity": 1.0
        }
      ],
      "filters": []
    },
    {
      "id": "0002",
      "name": "Texte principal",
      "type": "text",
      "visible": true,
      "locked": false,
      "opacity": 0.85,
      "blend_mode": "multiply",
      "transform": {
        "tx": 120,
        "ty": 450,
        "scaleX": 1.0,
        "scaleY": 1.0,
        "rotation": -5.0,
        "skewX": 0.0,
        "skewY": 0.0
      },
      "bounds": {
        "x": 120,
        "y": 450,
        "width": 640,
        "height": 80
      },
      "path": "layers/0002.png",
      "masks": [],
      "filters": [
        {
          "type": "gaussian_blur",
          "params": { "radius": 2.5 }
        }
      ],
      "text_data": {
        "content": "Bonjour le monde",
        "font_family": "Arial",
        "font_size": 48,
        "font_weight": "bold",
        "color": { "r": 0, "g": 0, "b": 0, "a": 255 },
        "alignment": "left"
      }
    }
  ],

  "layer_groups": [
    {
      "id": "group_001",
      "name": "Groupe UI",
      "visible": true,
      "locked": false,
      "opacity": 1.0,
      "blend_mode": "normal",
      "layer_ids": ["0003", "0004", "0005"]
    }
  ],

  "io": {
    "pixel_format_storage": "RGBA8_unorm_straight",
    "pixel_format_runtime": "ARGB32_premultiplied",
    "color_depth": 8,
    "compression": "png"
  },

  "metadata": {
    "created_utc": "2025-11-11T14:30:00Z",
    "modified_utc": "2025-11-11T15:45:00Z",
    "author": "Jean Dupont",
    "description": "Bannière publicitaire pour campagne Q4",
    "tags": ["marketing", "web", "bannière"],
    "license": "proprietary"
  },

  "guides": [
    { "orientation": "vertical", "position": 960 },
    { "orientation": "horizontal", "position": 540 }
  ],

  "selection": {
    "active": false,
    "path": null
  }
}
```

---

## 4. Description détaillée des sections

### 4.1. `epg_version`

- **Type** : `integer`
- **Description** : version du format `.epg`
- **Valeur actuelle** : `1`
- **But** : gestion de la rétrocompatibilité et détection de versions incompatibles

### 4.2. `canvas`

Configuration de la zone de travail.

| Champ         | Type      | Obligatoire | Description           | Valeurs/Limites                     |
| ------------- | --------- | ----------- | --------------------- | ----------------------------------- |
| `name`        | `string`  | ✅          | Nom de l'application  | 1 à 32 char                         |
| `width`       | `integer` | ✅          | Largeur en pixels     | 1 à 65535                           |
| `height`      | `integer` | ✅          | Hauteur en pixels     | 1 à 65535                           |
| `dpi`         | `integer` | ✅          | Résolution            | 72, 96, 150, 300...                 |
| `color_space` | `string`  | ✅          | Espace colorimétrique | `sRGB`, `Adobe RGB`, `ProPhoto RGB` |
| `background`  | `object`  | ✅          | Couleur RGBA de fond  | r,g,b ∈ [0,255], a ∈ [0,255]        |

**Exemple** :

```json
"background": { "r": 255, "g": 255, "b": 255, "a": 0 }
```

### 4.3. `layers`

Liste ordonnée des calques (du fond vers le premier plan).

#### 4.3.1. Propriétés communes

| Champ        | Type            | Obligatoire | Description           | Valeurs                                 |
| ------------ | --------------- | ----------- | --------------------- | --------------------------------------- |
| `id`         | `string`        | ✅          | Identifiant unique    | Format `NNNN`                           |
| `name`       | `string`        | ✅          | Nom d'affichage       | UTF-8, max 255 car.                     |
| `type`       | `string`        | ✅          | Type de calque        | `raster`, `text`, `shape`, `adjustment` |
| `visible`    | `boolean`       | ✅          | Visibilité            | `true`/`false`                          |
| `locked`     | `boolean`       | ✅          | Verrouillage          | `true`/`false`                          |
| `opacity`    | `float`         | ✅          | Opacité               | [0.0, 1.0]                              |
| `blend_mode` | `string`        | ✅          | Mode de fusion        | Voir §4.4.3                             |
| `transform`  | `object`        | ✅          | Transformation 2D     | Voir §4.4.4                             |
| `bounds`     | `object`        | ✅          | Rectangle englobant   | x, y, width, height                     |
| `path`       | `string`        | ✅          | Chemin vers l'image   | Relatif à la racine                     |
| `mask`       | `string`/`null` | ❌          | Chemin vers le masque | Relatif, ou `null`                      |
| `filters`    | `array`         | ❌          | Liste des effets      | Voir §4.4.5                             |

#### 4.3.2. Propriétés spécifiques aux calques texte

| Champ                   | Type      | Description                                       |
| ----------------------- | --------- | ------------------------------------------------- |
| `text_data.content`     | `string`  | Contenu textuel                                   |
| `text_data.font_family` | `string`  | Police de caractères                              |
| `text_data.font_size`   | `integer` | Taille en points                                  |
| `text_data.font_weight` | `string`  | Épaisseur (`normal`, `bold`, `light`...)          |
| `text_data.color`       | `object`  | Couleur RGBA                                      |
| `text_data.alignment`   | `string`  | Alignement (`left`, `center`, `right`, `justify`) |

#### 4.3.3. Modes de fusion supportés

| Mode          | Description            |
| ------------- | ---------------------- |
| `normal`      | Superposition standard |
| `multiply`    | Multiplication         |
| `screen`      | Superposition écran    |
| `overlay`     | Incrustation           |
| `darken`      | Obscurcir              |
| `lighten`     | Éclaircir              |
| `color_dodge` | Densité couleur -      |
| `color_burn`  | Densité couleur +      |
| `hard_light`  | Lumière crue           |
| `soft_light`  | Lumière tamisée        |
| `difference`  | Différence             |
| `exclusion`   | Exclusion              |

#### 4.3.4. Objet `transform`

```json
"transform": {
  "tx": 0.0,        // Translation X en pixels
  "ty": 0.0,        // Translation Y en pixels
  "scaleX": 1.0,    // Échelle X (1.0 = 100%)
  "scaleY": 1.0,    // Échelle Y
  "rotation": 0.0,  // Rotation en degrés [-360, 360]
  "skewX": 0.0,     // Inclinaison X en degrés
  "skewY": 0.0      // Inclinaison Y en degrés
}
```

**Ordre d'application** : échelle → inclinaison → rotation → translation

#### 4.3.5. Filtres et effets

```json
"filters": [
  {
    "type": "gaussian_blur",
    "params": { "radius": 5.0 }
  },
  {
    "type": "brightness_contrast",
    "params": { "brightness": 10, "contrast": 15 }
  },
  {
    "type": "hue_saturation",
    "params": { "hue": 30, "saturation": 20, "lightness": 0 }
  }
]
```

### 4.4. `layer_groups`

Groupes de calques pour l'organisation hiérarchique.

| Champ        | Type      | Description                      |
| ------------ | --------- | -------------------------------- |
| `id`         | `string`  | Identifiant unique du groupe     |
| `name`       | `string`  | Nom du groupe                    |
| `visible`    | `boolean` | Visibilité du groupe             |
| `locked`     | `boolean` | Verrouillage du groupe           |
| `opacity`    | `float`   | Opacité globale                  |
| `blend_mode` | `string`  | Mode de fusion du groupe         |
| `layer_ids`  | `array`   | Liste des IDs de calques membres |

### 4.5. `io`

Configuration des formats de pixels.

| Champ                  | Type      | Description           | Valeurs                                         |
| ---------------------- | --------- | --------------------- | ----------------------------------------------- |
| `pixel_format_storage` | `string`  | Format de stockage    | `RGBA8_unorm_straight`, `RGBA16_unorm_straight` |
| `pixel_format_runtime` | `string`  | Format d'exécution    | `ARGB32_premultiplied`, `RGBA32_premultiplied`  |
| `color_depth`          | `integer` | Profondeur de couleur | 8, 16, 32                                       |
| `compression`          | `string`  | Format de compression | `png`, `tiff`                                   |

### 4.6. `metadata`

Métadonnées du projet.

| Champ          | Type     | Obligatoire | Description                      |
| -------------- | -------- | ----------- | -------------------------------- |
| `created_utc`  | `string` | ✅          | Date de création (ISO 8601)      |
| `modified_utc` | `string` | ✅          | Dernière modification (ISO 8601) |
| `author`       | `string` | ❌          | Nom de l'auteur                  |
| `description`  | `string` | ❌          | Description du projet            |
| `tags`         | `array`  | ❌          | Mots-clés                        |
| `license`      | `string` | ❌          | Type de licence                  |

### 4.7. `guides`

Repères de composition.

```json
"guides": [
  { "orientation": "vertical", "position": 960 },
  { "orientation": "horizontal", "position": 540 }
]
```

### 4.8. `selection`

État de la sélection active.

```json
"selection": {
  "active": true,
  "path": "selection.path"  // Fichier SVG de chemin
}
```

---

## 5. Licence

Cette spécification est publiée sous **Creative Commons BY-SA 4.0**.  
Les implémentations peuvent utiliser toute licence compatible.

---

## Annexes

### Annexe A : Cas d'usage et exemples

#### A.1. Projet simple (1 calque)

```json
{
  "epg_version": 1,
  "canvas": { "width": 800, "height": 600, "dpi": 72 },
  "layers": [
    {
      "id": "0001",
      "name": "Photo",
      "type": "raster",
      "path": "layers/0001.png",
      "opacity": 1.0,
      "blend_mode": "normal"
    }
  ]
}
```

#### A.2. Projet avec effet de texte

```json
{
  "layers": [
    {
      "id": "0001",
      "name": "Titre avec ombre",
      "type": "text",
      "opacity": 1.0,
      "filters": [
        {
          "type": "drop_shadow",
          "params": {
            "offset_x": 3,
            "offset_y": 3,
            "blur": 5,
            "color": { "r": 0, "g": 0, "b": 0, "a": 128 }
          }
        }
      ],
      "text_data": {
        "content": "Bienvenue",
        "font_family": "Helvetica",
        "font_size": 72
      }
    }
  ]
}
```

---
