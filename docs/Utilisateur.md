# Documentation utilisateur

Ce document présente l'essentiel pour utiliser EpiGimp2.0 : démarrage, création et ouverture de projets, gestion des fichiers et des calques, outils principaux et conseils pratiques.

## Lancer le logiciel

- Sur Linux (depuis release AppImage) : rendre exécutable puis lancer :

```bash
chmod +x epigimp-linux-x86_64.AppImage
./epigimp-linux-x86_64.AppImage
```

- Si vous avez buildé depuis les sources, exécutez `./build/bin/epigimp` depuis la racine du projet.

## Interface principale (vue rapide)

- Barre de menus : Fichier, Vue.
- Barre d'outils : accès rapide aux outils (pot de peinture, sélection).
- Panneau Calques : liste des calques, opacité, verrouillage, boutons supprimer.
- Canvas : zone centrale pour dessiner.
- Palette de couleurs et historique (annuler/rétablir).

## Créer un nouveau projet

- Menu : `Fichier` → `Nouveau projet` (raccourci : Ctrl+N).
- Étapes :
  1.  Saisir largeur, hauteur et résolution (optionnel).
  2.  Choisir couleur de fond (transparent par défaut) et confirmer.
  3.  Le système crée le canvas et un calque de base.

Conseils : choisissez une taille adaptée à votre écran/mémoire (4K supporté, mais gourmand en RAM).

## Ouvrir / Importer une image

- Menu : `Fichier` → `Ouvrir` (Ctrl+O).
- Formats supportés : PNG, JPEG, et format projet `.epg` (voir section dédiée).
- Comportement : les images PNG/JPEG sont importées comme calque unique ; les `.epg` sont ouverts comme projet complet.

## Ouvrir / Enregistrer un projet (.epg)

- Format `.epg` : archive ZIP contenant `project.json`, `preview.png` et `layers/NNNN.png`.
- Enregistrer : `Fichier` → `Enregistrer` (Ctrl+S) ou `Enregistrer sous` pour choisir l'emplacement.
- Sauvegarde atomique : l'application écrit dans un fichier temporaire puis renomme pour éviter la corruption.

## Exporter l'image finale

- Menu : `Fichier` → `Enregistrer sous` (Ctrl+Shift+S).
- Options : choisir PNG (avec transparence) ou JPEG (qualité réglable).

## Gestion des calques

- Ajouter un calque : bouton `ajouter un calque` dans le menu fichier.
- Supprimer : sélectionner calque → clique sur poubelle. Confirmation si calque non vide.
- Réorganiser : glisser/déposer dans la liste des calques.
- Propriétés : nom, opacité, verrouillage, visibilité.
- Fusionner vers le bas : combine le calque avec celui inférieur.

## Historique

- Annuler / Rétablir : Ctrl+Z / Ctrl+Y. Historique par défaut limité à 20 actions (configurable).

## Raccourcis utiles

- Nouveau : Ctrl+N
- Ouvrir : Ctrl+O
- Enregistrer : Ctrl+S
- Exporter : Ctrl+Shift+E
- Annuler / Rétablir : Ctrl+Z / Ctrl+Y
- Pot de peinture : Maj+B


## Dépannage rapide

- Format non supporté : Vérifier l'extension et le fichier source.
- Espace disque insuffisant : libérer de l'espace ou choisir un autre emplacement.
- Calque verrouillé : déverrouiller dans le panneau Calques pour modifier.


## Ressources

- README du projet : [README.md](../README.md)
- Format `.epg` : [docs/epgformat.md](epgformat.md)
