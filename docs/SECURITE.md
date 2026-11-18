# Sécurité NAND et écritures — audit UZDoom NX2

Exigences : **aucune écriture susceptible d'endommager la NAND**, et **toutes
les écritures dans `sdmc:/switch/uzdoom-nx2`**.

## 1. Modèle de menace

kaiHEN donne à chaque homebrew une session FS complète (racine `fsp-srv`
clonée par HEN) : un programme peut ouvrir les partitions BIS (SYSTEM, USER,
SAFE, PRODINFO…), les sauvegardes système, les réglages `set:sys`, etc. Une
écriture fautive là peut rendre la console inutilisable. La carte SD, elle,
ne peut pas briquer la console, mais l'exigence est de n'y écrire que dans
notre dossier.

Chemins d'écriture possibles pour un programme :

| Chemin | Comment UZDoom NX2 le ferme |
|---|---|
| API libc (fopen, open, mkdir, rename, remove…) vers la SD | pare-feu devoptab (`nx2_fsguard.c`) + politique (`nx2_pathpolicy.c`) |
| Autre périphérique devoptab (save:, bis:, user:…) | aucun n'est monté ; le pare-feu démonte tout périphérique FS inconnu au démarrage ; la politique refuse tout préfixe autre que `sdmc:` |
| API FS libnx directe (`fsOpenBis*`, `fsOpen_*`, `fsdevMountSaveData`…) | absente du binaire (contrôle des symboles, build refusé sinon) |
| `fsdevGetDeviceFileSystem` / `fsFs*` direct (contourne la libc) | absent du binaire / interdit dans les sources du portage |
| Services système modifiant la NAND (`set:sys`, `ns`, `ncm`, `es`, `btm`, album, `nifm`…) | absents du binaire (contrôle des symboles) |
| API kaiHEN (racine FS, loader, plugins) | absente du binaire |
| IPC brut (`serviceDispatch`, `smGetService`) dans le code du portage | interdit par le contrôle des sources |
| Cache disque du pilote Mesa | désactivé sur Horizon dans OunceVK ; l'ancien nettoyage RDoom de `sdmc:/.mesa` a été supprimé |

## 2. Couche 1 — contrôle du binaire (`tests/check_nand_safety.sh`)

Exécuté par `scripts/build.sh` sur l'ELF final non strippé (lien statique,
`--gc-sections` : seul le code atteignable est présent). Le `.nro` est
**supprimé** si le contrôle échoue et `scripts/package.sh` refuse d'empaqueter
sans contrôle réussi. Il vérifie :

- aucun symbole correspondant à ~80 motifs interdits (FS NAND/sauvegardes/
  stockages, montages non-SD, réglages système, contenus, tickets, appairage,
  album, réseau, contrôle parental, horloges, bus matériels, SMC, debug
  d'autres processus, kaiHEN) ;
- le pare-feu est lié (`NX2_FsGuardInstall`, `NX2_PathCheck`, `G_open`,
  `G_unlink`, `G_rename`, `G_mkdir`, `G_rmdir`, `G_chdir`) et `userAppInit`
  appelle `NX2_FsGuardInstall` (vérifié par désassemblage) ;
- les sources du portage (`engine/.../nx/`, `launcher/src/`) n'utilisent ni
  IPC brut ni FS sous la libc, ni chemin SD absolu hors du bac à sable (sauf
  ligne marquée « nx2: read-only path » : dossier proposé en lecture).

Inventaire manuel de l'ELF de la 0.1.0 : les seules fonctions FS d'écriture
présentes sont `fsFsCreateFile/DeleteFile/CreateDirectory/DeleteDirectory/
RenameFile/RenameDirectory`, `fsFileWrite/SetSize`, appelées uniquement par
`fsdev` (derrière le pare-feu) sur l'unique système de fichiers ouvert,
`fsOpenSdCardFileSystem`. `setsys` n'est utilisé qu'en lecture
(`setsysGetFirmwareVersion`). `appletSetCpuBoostMode` /
`apmSetPerformanceConfiguration` sont référencés par `applet.c` de libnx pour
remettre les valeurs par défaut (contrôlé par `tests/check_no_overclock.sh`).

## 3. Couche 2 — pare-feu devoptab (`nx2_fsguard.c`)

Toutes les fonctions de fichiers de newlib passent par la table
`devoptab_list`. Dans `userAppInit` (appelé par libounce juste après le
montage de la SD, **avant tout constructeur C++ et toute ouverture de
fichier**), l'entrée `sdmc` est remplacée par une copie dont les opérations
suivantes consultent la politique avant d'appeler fsdev :

| Opération newlib | Contrôle |
|---|---|
| `open_r` avec `O_WRONLY`/`O_RDWR`/`O_CREAT`/`O_TRUNC`/`O_APPEND` | écriture fichier |
| `unlink_r`, `chmod_r`, `utimes_r` | écriture fichier |
| `rename_r` (source **et** destination), `link_r`, `symlink_r` | écriture fichier |
| `mkdir_r` | création de dossier (le dossier racine lui-même est permis) |
| `rmdir_r` | suppression de dossier |
| `chdir_r` | uniquement dans le bac à sable (les chemins relatifs y partent toujours) |
| `write_r`, `ftruncate_r`, `fchmod_r`, `fsync_r` | sur un descripteur que seul `G_open` a pu ouvrir en écriture |

Refus : `errno = EACCES`, l'appel n'atteint jamais fsdev, l'événement est
noté et écrit dans `logs/rdoom.log` (catégorie `fsguard`). Si l'installation
échoue (pas de SD, dossier impossible à créer), un écran d'erreur s'affiche
et **rien ne démarre**. Si un composant réenregistrait `sdmc`, le pare-feu
est remis en place (`NX2_FsGuardActive`, vérifié par le thread d'écriture).
`/switch` n'est jamais créé ; le dossier `switch/uzdoom-nx2` lui-même ne peut
être ni écrit, ni supprimé, ni renommé.

## 4. Couche 3 — politique de chemins (`nx2_pathpolicy.c`)

Résout le chemin comme fsdev (préfixe `sdmc:` facultatif, relatif au dossier
courant suivi par le pare-feu), normalise `.`/`..`, puis exige que les deux
premiers composants soient `switch` et `uzdoom-nx2` (insensible à la casse,
comme FAT/exFAT). Refusés d'office : tout autre préfixe (`bis:`, `save:`,
`SDMC:`…), plusieurs `:`, `\`, caractères de contrôle et jokers, noms faits
de points/espaces ou finissant par `.`/espace, `..` au-dessus de la racine,
chemins > 0x300. 54 tests (`tests/pathpolicy_tests.c`, ASan+UBSan) dans
`scripts/test.sh`.

## 5. Inventaire des écritures du programme

Toutes relatives au dossier courant (= `sdmc:/switch/uzdoom-nx2`) ou absolues
sous `NX2_SANDBOX_ROOT`.

| Origine | Fichiers |
|---|---|
| Démarrage (`rdoom_nx.cpp`) | `logs/` (rotation `logs/anterior*`), `boot.log`, `rdoom.log`, `engine.log`, `engine-stderr.log`, `engine-session.txt`, `gpu-*.txt` |
| Plantage (`i_nxsystem.cpp`, `nx_glthread.cpp`) | `logs/crash.log`, `hang.log`, `logs/phase.log` |
| Mesures (`nx_perf.cpp`, `rdoom_gpubench.cpp`) | `logs/perf.log`, `logs/gpu-bench.txt` |
| Launcher (`launcher/src/core`) | `profiles/*.json` (+ `.bak`, écriture atomique), `config/launcher.json`, `config/engine/<profil>.ini`, `backup/`, `export/`, `cache/` (miniatures, `loadinfo.txt`), `logs/last-launch.txt` |
| Moteur UZDoom | `config/engine/<profil>.ini`, `saves/<profil>/`, `screenshots/`, `demo/`, `cache/uzdoom/` (pipelines Vulkan, nœuds GL) |
| Écran de chargement | `loadtimes.txt` (historique des durées) |
| Console UZDoom (`logfile`, `dumpjit`…) | relatif au dossier ; un chemin hors du dossier est refusé par le pare-feu |

Retiré par rapport à RDoom : effacement de `sdmc:/.mesa/mesa_shader_cache_sf`
(démarrage et bouton « Vider le cache »).

## 6. Ce qui reste hors du contrôle du programme

- Les captures d'écran/vidéos faites avec le bouton Capture : gérées par le
  système (comme pour tout jeu), pas par le homebrew.
- kaiHEN, hbloader et hbmenu eux-mêmes.
