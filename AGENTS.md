# AGENTS.md — UZDoom NX2 (à lire avant de toucher au projet)

Portage de **UZDoom 5.0.3** sur **Nintendo Switch 2**, homebrew `.nro` lancé par
**kaiHEN** (CFW non restreint), rendu **Vulkan** (OunceVK = NVK de Mesa) sur
**libounce** (libnx pour Switch 2). Un seul exécutable : `uzdoom-nx2.nro`, qui
contient le launcher (profils, IWAD, mods) et le moteur.

Base : le fork moteur de RDoom-NX (aeon1911/RDoom-NX-engine, branche `rdoom-nx`,
UZDoom 5.0.3 + 66 commits de portage Switch 1) et son launcher. Nos commits sont
sur la branche `uzdoom-nx2` de `engine/` (dépôt git imbriqué, ignoré par le
dépôt parent) ; la série est exportée dans `patches/`.

## Règle n°1 : ZÉRO écriture NAND, tout dans sdmc:/switch/uzdoom-nx2

kaiHEN donne aux homebrews un accès FS complet (NAND comprise). Trois couches,
à ne jamais affaiblir :

1. **Symboles** — `tests/check_nand_safety.sh` refuse le binaire si l'ELF final
   contient une API libnx/libounce/libkaihen qui ouvre la NAND, des sauvegardes,
   des stockages système, ou modifie réglages/contenus/tickets/appairage/
   horloges/matériel. Il vérifie aussi que le pare-feu est lié, et que les
   sources du portage n'utilisent ni IPC brut ni l'API FS sous la libc.
2. **Pare-feu devoptab** — `engine/src/common/platform/posix/nx/nx2_fsguard.c`
   remplace l'entrée `sdmc` de newlib dans `userAppInit` (avant tout
   constructeur). Toute création/écriture/troncature/suppression/renommage/
   mkdir/rmdir/chmod/utimes hors de `sdmc:/switch/uzdoom-nx2` échoue (EACCES).
   Les autres périphériques FS sont démontés. Si l'installation échoue, le
   programme affiche une erreur et ne démarre pas.
3. **Politique de chemins** — `nx2_pathpolicy.c` (C pur) : canonisation,
   `..`, préfixe de périphérique, caractères/noms douteux refusés. Testée sur
   PC par `tests/pathpolicy_tests.c` (`scripts/test.sh`).

Interdits : `fsOpenBis*`, `fsOpen_*`, `fsdevMount*` autre que la SD,
`fsdevGetDeviceFileSystem`, `setsysSet*`, `ns*`, `ncm*`, `kaihen*`, IPC brut
(`serviceDispatch`, `smGetService`) dans le code du portage, chemins absolus
hors du bac à sable. `/switch` lui-même n'est jamais créé.

## Autres règles techniques

- Vulkan uniquement (OunceVK) ; un échec Vulkan est fatal avec message.
- Aucun overclock (`tests/check_no_overclock.sh`).
- Ticrate 35 Hz inchangé.
- Ne jamais distribuer d'IWAD commerciale ni de mod ; ne pas committer
  `*.wad`, `*.pk3`, `build/`, `dist/`.
- Les tests sur console sont ceux de l'utilisateur : ne jamais écrire « testé
  sur Switch 2 » sans ses logs.

## Build

```
docker build -t ounce-devkit <checkout de ouncebrew/ounce-devkit>
scripts/build.sh          # image + outils hôte + .nro + contrôles NAND + paquet + tests
scripts/test.sh           # tests hôte (politique de chemins, launcher)
```

## Carte du code propre au Switch 2

| Fichier | Rôle |
|---|---|
| `engine/src/common/platform/posix/nx/nx2_fsguard.c` | pare-feu SD (couche 2) |
| `engine/src/common/platform/posix/nx/nx2_pathpolicy.c` | politique de chemins (couche 3) |
| `engine/src/common/platform/posix/nx/i_nxsystem.cpp` | entrée, pile 16 Mo, crash.log, erreur fatale si pare-feu absent |
| `engine/src/common/platform/posix/nx/rdoom_nx.cpp` | layout SD, logs, launcher, thread d'écriture |
| `engine/src/common/platform/posix/sdl/sdlglvideo.cpp` | surface `VK_NN_vi_surface` 1920×1080 sur la fenêtre VI2 |
| `engine/libraries/ZVulkan/src/volk/volk.c` | pas de loader sur Horizon : commandes liées statiquement |
| `launcher/` | launcher (profils, bibliothèque IWAD/mods) |
| `tests/check_nand_safety.sh` | couche 1 |
