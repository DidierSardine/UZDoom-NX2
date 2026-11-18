# UZDoom NX2

Portage non officiel d'**UZDoom 5.0.3** pour **Nintendo Switch 2**, en homebrew
`.nro` lancé par **kaiHEN**. Rendu **Vulkan** via **OunceVK** (NVK de Mesa
pour Switch 2), sur **libounce** (libnx pour Switch 2). Un seul fichier,
`uzdoom-nx2.nro`, avec un launcher intégré (profils, bibliothèques d'IWAD et
de mods, ordre de chargement) en français.

Compatible **.wad**, **.pk3**, **.ipk3**, **.pk7**, **.zip**, **.deh/.bex**
(tout ce que charge UZDoom).

> État : **compile et passe tous les contrôles automatiques ; pas encore testé
> sur une vraie Switch 2.** Voir « Ce qui reste à vérifier sur console ».

## Sécurité : zéro écriture NAND

kaiHEN n'impose aucune restriction aux homebrews : un programme peut écrire
dans la NAND et rendre la console inutilisable. UZDoom NX2 se protège en trois
couches indépendantes :

1. **Rien dans le binaire ne peut ouvrir la NAND.** À chaque build,
   `tests/check_nand_safety.sh` inspecte l'ELF final (lié statiquement, code
   mort éliminé) et refuse le binaire s'il contient la moindre fonction qui
   ouvre une partition BIS, une sauvegarde, un stockage système, ou qui
   modifie réglages système, contenus installés, tickets, appairage Bluetooth,
   album, fréquences ou matériel (≈ 80 motifs), ainsi que toute l'API kaiHEN.
   Si le contrôle échoue, le `.nro` est supprimé.
2. **Pare-feu d'écriture sur la carte SD** (`nx2_fsguard.c`). Installé dans
   `userAppInit`, avant tout constructeur C++ et toute ouverture de fichier :
   l'entrée `sdmc` de newlib est remplacée par une copie dont chaque opération
   d'écriture (ouverture en écriture/création/troncature, suppression,
   renommage, mkdir, rmdir, chmod, utimes, liens) est refusée (`EACCES`) hors
   de `sdmc:/switch/uzdoom-nx2`. Cela couvre tout le code : moteur, launcher,
   ZMusic, SDL, pilote Vulkan. Les autres périphériques de fichiers sont
   démontés. Si le pare-feu ne s'installe pas, le programme refuse de démarrer.
3. **Politique de chemins stricte** (`nx2_pathpolicy.c`) : canonisation,
   `..`, préfixe de périphérique, noms ambigus (`...`, points/espaces finaux),
   caractères interdits. 54 tests unitaires sur PC.

Le code de RDoom qui effaçait `sdmc:/.mesa` (hors du dossier) a été retiré ;
OunceVK n'a pas de cache disque sur Horizon.

## Installation

Voir [docs/INSTALL-fr.txt](docs/INSTALL-fr.txt). En bref : copier `switch/` à
la racine de la SD, mettre les IWAD dans `switch/uzdoom-nx2/iwads/` et les
mods dans `switch/uzdoom-nx2/mods/`, lancer depuis hbmenu (jeu lancé avec R).

## Compiler

```sh
git clone https://github.com/ouncebrew/ounce-devkit
docker build -t ounce-devkit ounce-devkit
scripts/build.sh
```

Produit `dist/uzdoom-nx2.nro`, `dist/UZDoom-NX2-<ver>.zip` et
`dist/UZDoom-NX2-<ver>-Freedoom.zip` (avec les IWAD libres Freedoom).

## Origine et différences avec le portage Switch 1

Basé sur le moteur et le launcher de **RDoom NX** de Renan (Æon)
(github.com/aeon1911/RDoom-NX, GPLv3), portage Vulkan d'UZDoom pour Switch 1.
Changements pour Switch 2 :

- libounce + OunceVK au lieu de libnx + SDK Mesa ; CPU Cortex-A78C ;
- fenêtre VI2 (pas de file binder) : surface `VK_NN_vi_surface` 1920×1080,
  60 i/s à 60 et 120 Hz, VRR autorisé ;
- trampolines `vkGet*ProcAddr` pour les commandes d'extension non exportées
  par OunceVK (debug utils, surface caps 2, ray tracing) ;
- suppression des contournements Switch 1 / Ryujinx (pont TPIDR_EL0,
  compatibilité nvhost-as, wraps binder/Mesa GL) ;
- pare-feu SD et contrôles NAND ; tout sous `switch/uzdoom-nx2` ;
- launcher et écran de chargement en français, nouvelle identité visuelle.

## Licence

GPLv3 (comme UZDoom). Mentions des tiers dans `THIRD_PARTY_NOTICES.md` et
`switch/uzdoom-nx2/licenses/`.
