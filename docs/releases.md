# Release & Versioning

Ce projet publie automatiquement des releases via GitHub Actions lors du push d’un tag `v*` (ex: `v1.2.0`).

## Convention de version (SemVer)

On utilise la sémantique SemVer : `MAJOR.MINOR.PATCH`

- **MAJOR** : changement cassant (breaking change)
- **MINOR** : nouvelle fonctionnalité rétrocompatible
- **PATCH** : correction de bug / amélioration mineure rétrocompatible

Exemples :
- `v1.0.0` : première release stable
- `v1.1.0` : ajout de features sans casser l’existant
- `v1.1.1` : fix

## Pré-requis

- Être à jour avec `main` (ou la branche de release si vous en utilisez une)
- Avoir une CI verte sur la branche cible
- Avoir des changements “prêts release” (docs, changelog si présent)

## Étapes pour créer une release (tag)

### 1) Se placer sur la branche cible et mettre à jour

```bash
git checkout main
git pull --rebase
```

### 2) Choisir la nouvelle version

Décider du tag _(ex: v1.2.0)_.

> Astuce : si vous avez un CHANGELOG.md, mettez-le à jour avant de tagger.

### 3) Créer et pousser le tag

```bash
git tag -a v1.2.0 -m "v1.2.0"
git push origin v1.2.0
```

**Résultat attendu**

- Le workflow de release se déclenche automatiquement 
- Les artefacts (AppImage, zip Windows, etc.) sont générés 
- Une GitHub Release est publiée (selon la config du workflow)

**Vérifications après release**

- Vérifier la page GitHub Releases (assets présents)
- Télécharger et lancer :
  - Linux : AppImage exécutable 
  - Windows : zip portable
- Vérifier que l’app démarre et ouvre/exporte une image

## Corriger une release ratée

Re-tag (si vous devez refaire une release)

> ⚠️ Éviter de réutiliser le même tag si une release est déjà publique.
> Préférer un patch : ex v1.2.1.
