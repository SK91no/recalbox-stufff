# Release notes

This file will be processed and will be distributed with Recalbox Releases.
The text, located between the first `## Version XXX` and the second one, will appear
as release notes for end user on a Recalbox upgrade.

## Version 10.1

__English__:
- Add libretro mednafen STV core for RPi5 and PC
- Remove mame 2003, 2001, 2015 and advance mame from Raspberry Pi 5. 
  - The latest mame is optimized enough for all game.
- Automatically set bootloader options `POWER_OFF_ON_HALT=1` and `NET_INSTALL_AT_POWER_ON=0` on Raspberry Pi 5
- Do not display framebuffer image when launching games
- Add Recalbox JVS support
- Change boot splash screen and add darkmode option
- Major performance boost on Model 3 on Raspberry Pi 5
- Compatibility with Initial D series and F355 games on flycast
- Dynamic controls remap for JVS race cab
- Force single screen for OutRunners on fbneo
- Add bouncing screensaver (DVD type)
- Bump libretro-atari800 to 668d34eb
- Add missing moonlight-embedded on PC
- Remove duplicated keys in recalbox.conf
- Saving after resuming a game from a save state stays on that slot, instead of drifting to a phantom slot that could never be loaded back
- Fix missing logos on games whose only wheel is ScreenScraper's high-def one (Initial D, Beach Spikers)
- X68000 sound no longer saturates on Recalbox RGB Dual 2, Recalbox RGB JAMMA and Recalbox JVS
- Scrape SuFami Turbo and Dragon 32/64 games as their own system instead of the Super Nintendo and the TRS-80 Color Computer
- Two identical USB pads can now be configured apart, each one keeping the mapping of the port it is plugged into
- Fixes and enhances serach view from main screen


__Français__ :
- Ajout du core STV libretro mednafen pour RPi5 et PC
- Suppression de mame 2003, 2001, 2015 et de advance mame sur Raspberry Pi 5. 
  - Les dernières versions de Mame sont assez optimisées pour passer tous les jeux.
- Change automatiquement les options du bootloader `POWER_OFF_ON_HALT=1` et `NET_INSTALL_AT_POWER_ON=0` sur Raspberry Pi 5
- N'affiche plus l'image du framebuffer au lancement des jeux
- Ajout du support de Recalbox JVS
- Changement du splash screen de démarrage et ajout d'une option darkmode
- Gain de performances sur Model 3 sur Raspberry Pi 5
- Compatibilité avec la serie Initial D et F355 sur flycast
- Remap dynamic des controls pour les race cab JVS
- Force le single screen pour OutRunners sur FBNeo
- Ajout d'un écran de veille de logo rebondissant (type DVD)
- Mise à jour de libretro-atari800 à 668d34eb
- Ajout de moonlight-embedded manquant sur PC
- Suppression des clés en doublons dans recalbox.conf
- Sauvegarder après avoir repris une partie depuis une save state reste sur ce slot, au lieu de dériver vers un slot fantôme impossible à recharger
- Correction des logos manquants sur les jeux dont le seul wheel est celui haute définition de ScreenScraper (Initial D, Beach Spikers)
- Le son du X68000 ne sature plus sur Recalbox RGB Dual 2, Recalbox RGB JAMMA et Recalbox JVS
- Les jeux SuFami Turbo et Dragon 32/64 sont scrapés sur leur propre système au lieu de la Super Nintendo et du TRS-80 Color Computer
- Deux manettes USB identiques peuvent désormais être configurées séparément, chacune gardant le mapping du port sur lequel elle est branchée
- Corrige et améliore la vue recherche de l'écran principal

## Version 10.0.7

__English__:
- RPi 5 2GB
  - Graphics memory optimisations
  - Update RPi5 bootloader for better compatibility with RPi5 2GB
- Fix RGB JAMMA 2 OLED screen

__Français__ :
- RPi 5 2GO
  - Optimisations de la mémoire graphique
  - Mise à jour du bootloader pour une meilleure compatibilité avec le RPi5 2GO
- Correction de l'écran OLED RGB JAMMA 2

## Version 10.0.6

__English__:
- Improved support for the RPi5 2GB and optimization of graphics memory.
- Check if boot has enough free space to proceed to the upgrade, before upgrading.
- Fix read/write speed on SHARE partitions (exFat)
- Fix GPICase HDMI output & default button mapping.
- Fix default theme causing loop boot for Tate 4:3 HD
- Fix oled screen on RGB JAMMA
- Fix reversed buttons 6 on JAMMA 2
- Add support of Fightstick R10 Pro

__Français__ :
- Meilleur support du RPi5 2Go et optimisation de la mémoire graphique.
- Affiche une popup pour verifier si le device à assez d'espace libre au lancement d'une upgrade
- Corrige les performances de lecture ecriture sur le SHARE (exFat)
- Corrige la sortie GPICase HDMI et de la configuration par défaut des boutons.
- Corrige un problème avec le thème par défaut provoquant un redémarrage en boucle sur le Tate 4:3 HD
- Corrige l'affichage sur l'écran oled du RGB JAMMA
- Corrige les boutons 6 inversés sur JAMMA 2

## Version 10.0.5

__Français__ :
- Corrige les sous repertoires manquants dans les listes de jeux
- Corrige la partition share sur les Odroid GO Advance et Super

__English__ :
- Fixes missing subdirectories in the game lists
- Fixes the share partition on the Odroid GO Advance and Super

## Version 10.0.4

__Français__ :
- Corrige un crash potentiel dans le calcul du hachage des chaînes de caractères
- Retarde l'événement de calcul du hash des roms pour accélérer le démarrage du frontend
- Correction de la dépendance au build du webmanager
- Correction du jeu préinstallé thomson (fd -> zip)
- Correction des inputs recommandés pour le zx spectrum
- Adaptation du driver pour RGB JAMMA 2
- Ajout d'une aide pour le folding/unfolding des jeux 1G1R dans la vue detaillée
- Correction du problème lié aux manettes USB doubles où les deux périphériques étaient déclarés sur un seul descripteur USB
  - Cela pourrait résoudre les problèmes rencontrés avec : Pandorabox, certains modèles Xinmo, les adaptateurs doubles Retrobit, les adaptateurs Shanwan, les adaptateurs DragonRise, etc.
- Ajout de l'aide pour plier/déplier les jeux en mode 1G1R avec HK+gauche/droite

__English__:
- Fix potential crash in string hash calculation
- Delay rom hash calculation event more to speed up frontend startup
- Fix webmanager dependencies
- Fix thomson preinstalled game
- Fix recommended inputs for zx spectrum
- Adapt driver for RGB JAMMA 2
- Add help item for folding/unfolding 1G1R games in detailedview
- Fix DUAL USB joysticks declaring both devices on one descriptor
  - May fix : pandorabox, some xinmo, Retrobit dual adapters, Shanwan twins, DragonRise twins etc...
- Add help for Fole/Unfold 1G1R games with HK+Left/right

## Version 10.0.2

__Français__ :
- Ajout de l'aide pour plier/déplier les jeux en mode 1G1R avec HK+gauche/droite
- Corrige un crash lors de l'utilisation de HK+gauche/droite sur les en-têtes de tri dans les listes de jeux
- Désactivation du menu des scripts utilisateur lorsqu'aucun script n'est disponible
- Renomme les systèmes (nom complet uniquement) :
  - Atari 800 => Atarti 8 bits (conforme à la compatibilité XL/XE)
  - Neo-Geo => Neo-Geo AES (conforme au nouveau logo et à la nouvelle politique)
  - Amiga 600 => Amiga ECS/OCS (conforme à la compatibilité non-aga)
  - Amiga 1200 => Amiga AGA (conforme à la compatibilité aga)
- Correction des jeux qui ne s'affichent pas toujours dans le système "derniers jeux joués" (lastplayed)
- Correction du menu de résolution indisponible en HDMI avec un adaptateur CRT connecté  
- Corrige la performance des share dynamiques formatés en exFat
- Corrige la vitesse de démarrage (le chargement des systèmes n'était plus multi-threadé)
- Corrige le softpatching non fonctionnel
- Correction du lancement des jeux model 3 sur JAMMA + HDMI
- Correction de l'écran OLED JAMMA lors de l'utilisation de l'HDMI
- Correction des lenteurs au chargement des systemes
- Le système NeoGeo a été renommé NeoGeo AES (console de salon) et ne fait plus partie des systèmes arcade.
  - Pour vos jeux NeoGeo MVS (arcade), utilisez les roms fbneo ou mame, et activez le système virtuel NeoGeo
- Corrige un problème dans la mise çà jour du système LastPlayed quand on revient d'un jeu.
- Corrige le message d'erreur qui apparait furtivement quand on démarre une MAJ de Recalbox.
- Nettoyage du dossier d'upgrade si le téléchargement a échoué, ce qui évite une tentative d'installation inutile au reboot
- Corrige l'extraction des métadata des noms de fichiers sur les devices en read-only (elles n'étaient pas extraites jusque là)
- Déporte le calcul des hash après le chargement des fichiers (ne ralentit pas le chargement des systèmes quand les hash ne sont pas encore calculés)
- Theme: neogeo logo for crt - cps2 assets - system desc
- Correction du versionning des themes
- Correction des modes d'affichage de dosbox et scummvm sur crt

__English__:
- Add help for Fole/Unfold 1G1R games with HK+Left/right
- Fix crash when using HK+left/right on sort headers in gamelists
- Disable userscript menu when no script is vailable at all
- Rename systems (fullname only):
  - Atari 800 => Atarti 8bits (consistent with XL/XE compatibility)
  - Neo-Geo => Neo-Geo AES (consistent with new logo and new policy)
  - Amiga 600 => Amiga ECS/OCS (consistent with non-aga compatibility)
  - Amiga 1200 => Amiga AGA (consistent with aga compatibility)
- Fix games not always shown in last-played system
- Fix resolution menu not available in HDMI with a CRT adapter connected  
- Fix perf of dynamic shares in exFat
- Fix startup performances (system loading was no longer multi-threaded)
- Fix non-working softpatching
- Fix Model 3 games on JAMMA + HDMI
- Fix JAMMA OLED screen when using HDMI
- Fixed slow loading of systems
- The NeoGeo system has been renamed NeoGeo AES (home console) and is no longer part of the arcade systems.
 - For your NeoGeo MVS (arcade) games, use fbneo or mame ROMs and activate the NeoGeo virtual system.
- Fix a problem in updating the LastPlayed system when returning from a game.
- Fix error message that appears briefly when starting a Recalbox update.
- Cleaned up the upgrade folder if the download failed, which prevents unnecessary installation attempts on reboot
- Correct the extraction of metadata from file names on read-only devices (they were not extracted until now)
- Defer the calculation of hashes until after the files have been loaded (does not slow down the loading of systems when the hashes have not yet been calculated)
- Theme: neogeo logo for crt - cps2 assets - system desc
- Fix theme versionning
- Fix display modes for scummvm and dosbox on crt


## Version 10.0.1

__Français__ :
- Corrige la mise à jour des variables de theme contextuelles quand on revient d'un jeu (sur notre theme, c'est la date de dernière partie qui se met à jour)
- Corrige le mise à jour de la gamelist après detection de nouveaux jeux ajoutés automatiquement et rescan du système
- Corrige la mise à jour de la gamelist et des metadata quand on sort du menu contextuel des jeux
- Corrige le re-scraping d'un même système
- Corrige la non-reutilisation des données déjà scrapées et présentes sur le disque
- Corrige la gestion du retry pour les download de media et modifie la façon sont certaines erreurs sont remontées  (plus de Fatal error qui n'en sont pas)
- Corrige la fenêtre des BIOS en HDMI
- CRT: flycast: fix le mode 240p downscalé (phase 1/2)
- Corrige le colorset gris + Clip "Full video" pour CRT sur recalbox-next-2025
- Corrige l'affichage "inconnu"  sur les dossier dans la liste de jeu sur l'ancien theme
- Fix de la sauvegarde dans les fichiers .ini quand le backup n'existe pas
- Fix du changement de resolution du frontend pour le CRT
- Fix d'une race condition pouvant amener à un plantage ou a des fuites mémoire quand on enerve trop le manager de textures
- Fix du chargement d'un png inexistant, malformé ou vide
- Fix du son des vidéos de boot
- Fix rotation des pinball en mode Tate droit
- Retour à 100% de la sensibilité dans fbneo avec un spinner
- Fix ajout du menu crt avec les DACS
- Fix des modes 240p 60Hz pour les jeux TATE sur écrans YOKO (phase 1/2)
- Fix de la gestion du case sur steamdeck
- Mise à jour de lr-holani vers 0.9.9

__English__:
- Fix the update of contextual theme variables when returning from a game (in our theme, the last game date is updated)
- Fix the update of the gamelist after detecting new games added automatically and rescan of the system
- Fix the update of the gamelist and metadata when exiting the games context menu
- Fix scraping the same system multiple times
- Fix the non-reuse of data already scraped on the disk
- Fix retry management for media downloads and changes the way some errors are reported (no more fatal errors that aren't actually fatal)
- Fix the BIOS window in HDMI
- CRT: flycast: fix downscaling when force 240p mode
- Fix grey colorset option + fullvideo clip on CRT on recalbox-next-2025
- Fix "inconnu" on folder  in gamelist on old theme
- Fix saving to .ini files when no backup exists
- Fix frontend resolution selection for CRT
- Fix a race condition that could lead to crashes or memory leaks when overloading the texture manager
- Fix loading of non-existent, malformed, or empty png files
- Fix no sound on boot videos
- Fix rotation for virtual pinball when Tate right is selected
- Rollback to 100% of analog sensibility for spinner in fbneo
- Fix crt menu with crt DACS added
- Fix 240p 60Hz mode for TATE games on YOKO screens
- Fix case management on steamdeck
- Bump lr-holani to version 0.9.9


## Version 10

### News

#### Hardware
- Add RPi5 2GB support
- Add RPi500 support
- Add Steam Deck LCD & OLED Support
- Add RGB DUAL 2 Support
- Add RGB JAMMA 2 Support
- Add Recalbox Card Reader Support

#### Cases
- Add 64Pi Case support
- Add Argon 40 ArgonOne v3 for RPi5 support
- Add DreamCase support

#### Controllers
- Add Taito Trackball and Spinner support
- Add Baolian Spinner support
- Add DIY Spinner (Arduino) support

#### Emulators
- Add Model 3 (supermodel) in Pi5
  - Add model3 games to the lightgun game list
- Add XBOX emulator (xemu)
- Add Gamecube libretro dolphin for rpi5
- Add Gamecuve Dolphin standalone for rpi5
- Add Hyper Neo-Geo 64 virtual system
- Add Tamagotchi (tamalibretro core)
- Add new Atari core, HatariB set as new default emulator for Atari ST(E)/Mega ST(E)/TT/Falcon
- Add AppleWin libretro core in Apple II system (up to date w/ standalone version)
- Add libretro-b2 for BBC Micro system
- Change lr-hatari to lr-hatariB
- Add libretro-dice for machines without any type of CPU
- Add Visual Pinball for rpi5 and X86
- Add Sega ST-V virtual system
- Add Geargrafx core
- Add Holani core (Atari Lynx)
- Add official lexaloffle pico8 core
- Add libretro-ps2
- Add gearsystem for CRT

#### Games
- Add VVVVVV port
- Add new port Corsix-TH (port of Theme Hospital)
- Add new port Julius (port of Caesar III)
- Add new port Openlara (port of Tomb Raider)
- Add Mohkey in GBC pre-installed games
- Add new port The Powder Toy
- Add new port vitaQuake3 (port of Quake 3)
- Add new port vitaVoyager (port of Star Trek Voyager Elite Force)
- Add Mabool game on VG5000
- Add support of WiiWares games

#### System
- Add Theme Manager ! Install, uninstall & update themes from our online theme repository
- New original system logos in the Frontend !
- New menus design, better, faster, stronger !
- Add extension check when launching games to prevent running emulators on incompatible file extensions
- Add option to enable/disable adding/removing favourites with Y button
- Add meaningful headers when using sorts other than alphabetical order
- Reintroduced Favourites on top of gamelist as an option
- Add new menu to hide systems individually
- Add user script menu to run scripts from the interface
- Add detection of copied/modifier/removed roms in rom folder
  - Rescan modified rom folders to detected added/removed games
  - Option to scrape newly added roms (PATREON Only)
- Add support customize iso from Raspberry Pi Imager (add sshkey, wifi, hostname and password)
- Add NetPlay support for cores libretro-gpsp and libretro-dosbox-pure
- Add new search by aliases or licences
- Add gamelist optional decoration on the right side: regions flags, genre, players, support number & support types
- Add new One Game One Rom (1G1R)
  - Choose your priority regions
  - Also work on multidisc/multi-disk/multi-tape roms
- Add Plex plugin for Kodi
- Add automatic bootloader upgrade at startup for RPi4 and RPi5
- Add Kodi Crunchyroll plugin
- Add Orange PVR plugin
- Remove Linapple (obsolete Apple II emulator)
- Add "reset emulator settings" option that makes all emulators returning to their factory defaults


### Improvements
- Add "Quick jump option" to jump in large gamelists (replace "Jump to letter")
- Add option to disable ultra-fast scrolling in gamelists
- Add gamelist decoration on the right side: regions flags, genre, players, support number & support types
- Add script for support of Retroflag GPI Case 2 (at boot only)
  - Unset in-case pad when HDMI is connected
  - Audio switching between in-case and HDMI
  - Switch to recalbox-next theme when docked
- Add sort by playing time & sort by regions
- Remove overscan option in frontend
- Add automatic multitap support on snes
- User data in gamelists are now saved separately, allowing to:
  - Use external scraper w/o loosing your favourites, emulators configurations, etc.
  - Add pre-installed game in favourite list or even hide them individually
- Move L2/R2 buttons to L1/R1 on Dreamcast if controller doesn't have L2/R2
- RGB Dual/Jamma
  - New CRT mode management with dynamic resolutions
  - Support for 24kHz on Jamma
  - Enable retroarch notifications
  - Add priority to HDMI support on Recalbox RGB DUAL and JAMMA
- Added options into configgen for Amiga systems
- Improved removable/internal disk support (SATA/NVME, USB, MMC).
  - Better network share naming
  - All external storages are now shared on the network (was only first USB key before)
- Force analog mode on Naomi, Naomi GD and Naomi 2
- Add rumble option in frontend
- Move Amiga bios automatically into its directory
- Simplified/improved Virtual Keyboard ergonomy & search window
- Move retroarch hotkey to F12 & qui key to Escape (use F12+Escape to quit)
- Libretro-mame
- Add new core : libretro-mame0278
  - Support Hiscore
  - Access to service mode for rgb jamma
- Improve bluetooth, automatic reconnection of sound and game controllers on reboot
- Add medium preset for overclocking for rpi5 (2600 Mhz)
- Set flycastnext core as default core for dreamcast, naomi, atomiswave on RPi5
- Added emulator options in frontend for systems:
  - PSP (PPSSPP)
  - Gamecube/Wii (Dolphin)
  - Xbox (Xemu)
  - Neo-Geo (Libretro-Geolith)
  - Atari ST (Libretro-HatariB)
  - ZX Spectrum (Libretro-Fuse)
  - Nintendo DS (Libretro-MelonDS)
  - x68000 (Libretro-Px68k)
  - SCUMMVM (SCUMMVM)
  - Model 3 (Supermodel)
- Lots of improvements in Gamelist updates:
  - No longer requires frontend reboot
  - Initial gamelist loading automatically removes deleted games
  - The frontend detects file changes in rom folders and prompts for updating gamelists
  - The frontend new detects external changes on gamelist.xml and prompts for reload
  - Updating gamelists detects added games and prompt for immediate scraping of new games in background
- Better support for newer rg353m (no more downclock needed)
- Libretro Scummvm
  - Enable subtitle
  - Possibility to choose language inside .scummvm file
- Gamelist & Arcade gamelist unification
- Show artist name in playlist popup for patron mixtapes
- Added auto-detection of Gamecube/Wii game to get correct inputs
- Added Xbox eeprom auto-configuration from frontend options
- Add support of softpatching for geargrafx & gearsystem
- Auto selection right shader for handheld when you use shadersets
- Auto configuration multitap for psx and saturn
- WIFI and WIFI connection are now separate.
- WIFI hotspot scan works when WIFI is enabled in network menu
- Update readme (and localized readme) in rom folders when recalbox is updated
- Automatic creation of missing directories/files on share/external:
  - Missing roms directories
  - Missing readme files (.txt)
  - Pad-2-Keyboard global files (.p2k.cfg)
- Switch to new texture manager:
  - Smoother interface, especially for slow media or NAS
  - Fixes issues with image-rich themes
- Lots of improvements in themes:
  - New variables (global, contextual and user)
  - Free options: define your own options
  - New simplified writing style (use attributes instead of node)
  - Localizable text/image
  - Default logos available for all themes
  - ... and much more. Check the wiki for additional information
- Debounce buttons & hats in frontend to avoid double clicks with zero-delay controllers
- Make multiline texts scrollable using right joystick in system list (information) and gamelist (game synopsis)
- Add menu entry (in System menu) to display current version's release notes
- Show Raspberry Pi RAM in System menu
- Disable FBNEO rewind by default to avoid performance issues. Can be reenabled in advanced emulator settings.

### Fixes
- Fix hidden and/or empty being shown after some user operations
- Fix user selected ratio per game
- Fix Supermodel config and configgen files (Thanks AxelR !)
- Fix bios scanner slowing down startup when running on large files
- Fix enable RetroAchievements for new systems
- Fix moonlight embedded script
- Fix arcade joysticks gpio on RPi4
- Fix webmanager on Raspberry Pi 3
- Fix rg353m v2 support
- Fix demo mode by adding default image when there is no image from scrap to display
- Fix default emulator to libretro-puae for all Amiga systems in RPi5 and PC
- Fix zip loading with libretro-quasi88
- Fix bios paths for libretro-mgba
- fix force 480i for 240p scummvm games when 480i is selected in resolution popup
- Fix disable service menu in fbneo for others players in netplay
- Fix go to game from search and gameclip
- Fix TATE filtering in TATE system
- Fix non-working overlays in demo mode
- Fix freezes and crashes in search windows
- Fix netplay by recreating the retroarch.corenames files on each upgrade
- Fix crash when using unrecognized roms only in arcade systems (naomi2 issues in patron channel)
- Fix softpatching work with uppercase extension letters
- Fix OSD fonts in low resolutions
- Fix UI of single game scraping window
- Fix UI of scraping window in low resolutions
- Fix display of A/B buttons in help when inversion is enabled
- Fix some crashes in sound management
- Fix fatal error in scraper (ignore the faulty game & continue)
- Fix resolutions in demo mode (Makes overlay working)
- Fix P2K files & P2K Hint window
- Fix multiple issues in TATE filtering
- Fix controllers support with mupen64plus
- Fix user overlays prevents games from working on 4/3 screen (for handled cases with dock)
- Fix highscore for lr-mame and add access to dip switch for RGB JAMMA
- Fix MSU support for libretro-genesisplusgx

### Bumps
- Bump retroarch to 1.21.0
- Bump xone (xbox joystick driver)
- Bump nvidia drivers
- Bump libretro-mame2000
- Bump libretro-snes9x2002
- Bump mupen64plus
- Bump ppsspp
- Bump vice
- Bump sdlpop
- Bump scummvm
- Bump dolphin-emu
- Bump rb5000
- Bump libretro-yabasanshiro
- Bump libretro-vitaquake2
- Bump libretro-snes9x
- Bump libretro-picodrive
- Bump libretro-mupen64plux-nx
- Bump libretro-fceumm
- Bump libretro-ecwolf
- Bump libretro-bluemsx
- Bump libretro-dosbox-pure to 1.0-preview2
- Bump libretro-ppsspp
- Bump Kodi plugins
- Bump libretro-vice
- Bump libretro-px68k
- Bump lr-vice
- Bump lr-mame to 0.274
- Bump lr-parallel-n64
- Bump lr-pcsx-rearmed
- Bump lr-sameboy (move to official repo)
- Bump hypseus-singe
- Bump Solarus to version 2.0
- Bump moonlight embedded
- Bump supermodel
- Bump Mesa3D to 25.1.0 for rpi4 and rpi5
- Bump libretro-dolphin
- Bump RetroArch cheats
- Bump libretro-quasi88
- Bump libretro-mednafen-saturn
- Bump libretro-gearsystem
- Bump libretro-genesisplusgx
- Bump libretro-genesisplusgxwide
- Bump libretro-desmume
- Bump libretro-melonds

__Français__ :

### News

#### Matériel
- Ajout de la prise en charge du RPi5 2 Go
- Ajout de la prise en charge du RPi500
- Ajout de la prise en charge des Steam Deck LCD et OLED
- Ajout de la prise en charge du RGB DUAL 2
- Ajout de la prise en charge du RGB JAMMA 2
- Ajout de la prise en charge du Recalbox Card Reader

#### Boîtiers
- Ajout de la prise en charge du boîtier 64Pi
- Ajout de la prise en charge d'Argon 40 ArgonOne v3 pour RPi5
- Ajout de la prise en charge de DreamCase

#### Contrôleurs
- Ajout de la prise en charge de Taito Trackball et Spinner
- Ajout de la prise en charge de Baolian Spinner
- Ajout de la prise en charge de DIY Spinner (Arduino)

#### Émulateurs
- Ajout du Model 3 (supermodel) pour Pi5
  - Ajout des jeux model3 à la liste des jeux lightgun
- Ajout de l'émulateur XBOX (xemu)
- Ajout de Gamecube libretro dolphin pour rpi5
- Ajout de Gamecuve Dolphin standalone pour rpi5
- Ajout du système virtuel Hyper Neo-Geo 64
- Ajout de Tamagotchi (noyau tamalibretro)
- Ajout d'un nouveau noyau Atari, HatariB défini comme nouvel émulateur par défaut pour Atari ST(E)/Mega ST(E)/TT/Falcon
- Ajout du noyau AppleWin libretro dans le système Apple II (à jour avec la version autonome)
- Ajout de libretro-b2 pour le système BBC Micro
- Remplacement de lr-hatari par lr-hatariB
- Ajout de libretro-dice pour les machines sans aucun type de CPU
- Ajout de Visual Pinball pour rpi5 et X86
- Ajout du système virtuel Sega ST-V
- Ajout du noyau Geargrafx
- Ajout du noyau Holani (Atari Lynx)
- Ajout du noyau officiel lexaloffle pico8
- Ajout de libretro-ps2
- Ajout d'un nouveau noyau : libretro-mame0278
  - Prise en charge de Hiscore
  - Accès au mode service pour rgb jamma
- Ajout du support du core gearsystem pour CRT

#### Jeux
- Ajout du port VVVVVV
- Ajout du nouveau port Corsix-TH (port de Theme Hospital)
- Ajout du nouveau port Julius (port de Caesar III)
- Ajout du nouveau port Openlara (port de Tomb Raider)
- Ajout de Mohkey dans les jeux préinstallés GBC
- Ajout du nouveau port The Powder Toy
- Ajout du nouveau port vitaQuake3 (portage de Quake 3)
- Ajout du nouveau portage vitaVoyager (portage de Star Trek Voyager Elite Force)
- Ajout du jeu Mabool sur VG5000
- Ajout de la prise en charge des jeux WiiWares

#### Système
- Ajout du gestionnaire de thèmes ! Installez, désinstallez et mettez à jour les thèmes à partir de notre repo de thèmes en ligne
- Nouveaux logos pour les système dans l'interface utilisateur !
- Nouveau design des menus, plus clair, plus rapide, plus puissant !
- Ajout d'une vérification des extensions lors du lancement des jeux afin d'empêcher lancement de jeux avec des extensions incompatibles
- Ajout d'une option permettant d'activer/désactiver l'ajout/la suppression de favoris à l'aide du bouton Y
- Ajout d'en-têtes lors de l'utilisation de tris
- Réintroduction des favoris en haut de la liste des jeux en tant qu'option
- Ajout d'un nouveau menu pour masquer les systèmes individuellement
- Ajout d'un menu de scripts utilisateur pour exécuter des scripts à partir de l'interface
- Ajout de la détection des roms copiées/modifiées/supprimées dans le dossier rom
  - Réanalyse des dossiers rom modifiés pour détecter les jeux ajoutés/supprimés
  - Option pour récupérer les roms nouvellement ajoutées (PATREON uniquement)
- Ajout de la prise en charge de la personnalisation des iso à partir de Raspberry Pi Imager (ajout de sshkey, wifi, nom d'hôte et mot de passe)
- Ajout de la prise en charge NetPlay pour les cœurs libretro-gpsp et libretro-dosbox-pure
- Ajout d'une nouvelle recherche par alias ou licences
- Ajout de decorations en option dans la liste de jeux sur le côté droit : drapeaux des régions, genre, joueurs, nombre et types de prise en charge
- Ajout de la fonctionnalité One Game One Rom (1G1R)
  - Choisissez vos régions prioritaires
  - Fonctionne également avec les ROM multidisc/multi-disquette/multi-cassette
- Ajout du plugin Plex pour Kodi
- Ajout de la mise à jour automatique du bootloader au démarrage pour RPi4 et RPi5
- Ajout du plugin Kodi Crunchyroll
- Ajout du plugin Orange PVR
- Suppression de Linapple (émulateur Apple II obsolète)
- Ajouter l'option « réinitialiser les paramètres des émulateurs » qui permet à tous les émulateurs de revenir à leurs paramètres d'usine par défaut


### Améliorations
- Ajout de l'option « Saut rapide » pour naviguer dans les longues listes de jeux (remplace « Aller à la lettre »)
- Ajout d'une option pour désactiver le défilement ultra-rapide dans les listes de jeux
- Ajout d'un script pour la prise en charge du boîtier Retroflag GPI Case 2 (au démarrage uniquement)
  - Désactivation du pad intégré au boîtier lorsque le HDMI est connecté
  - Commutation audio entre le boîtier intégré et le HDMI
  - Passage au thème recalbox-next lorsque le boîtier est connecté
- Ajout du tri par nombre de parties et par région
- Suppression de l'option overscan dans l'interface
- Ajout de la prise en charge automatique du multitap sur snes
- Les données utilisateur dans les listes de jeux sont désormais enregistrées séparément, ce qui permet :
  - D'utiliser un scraper externe sans perdre vos favoris, vos configurations d'émulateurs, etc.
  - D'ajouter des jeux préinstallés à la liste des favoris ou même de les masquer individuellement
- Déplacement des boutons L2/R2 vers L1/R1 sur Dreamcast si la manette ne dispose pas de L2/R2
- RGB Dual/Jamma
  - Nouvelle gestion du mode CRT avec résolutions dynamiques
  - Prise en charge du 24 kHz sur JAMMA
  - Activation des notifications RetroArch
  - Ajout d'une priorité HDMI sur Recalbox RGB DUAL et JAMMA
- Ajout d'options dans configgen pour les systèmes Amiga
- Amélioration de la prise en charge des disques amovibles/internes (SATA/NVME, USB, MMC).
  - Meilleur nommage des partages réseau
  - Tous les stockages externes sont désormais partagés sur le réseau (auparavant, seule la première clé USB l'était)
- Forcer le mode analogique sur Naomi, Naomi GD et Naomi 2
- Ajouter d'une option Vibration des manettes
- Deplacement automatique dule bios Amiga dans son répertoire
- Ergonomie simplifiée/améliorée du clavier virtuel et de la fenêtre de recherche
- Déplacement le raccourci clavier retroarch vers F12 et la touche qui vers Escape (utiliser F12+Escape pour quitter)
- Amélioration du Bluetooth, reconnexion automatique du son et des manettes de jeu au redémarrage
- Ajout d'un préréglage moyen pour l'overclocking du rpi5 (2600 Mhz)
- Définition du cœur flycastnext comme cœur par défaut pour dreamcast, naomi, atomiswave sur RPi5
- Ajout d'options d'émulateur dans l'interface pour les systèmes suivants :
  - PSP (PPSSPP)
  - Gamecube/Wii (Dolphin)
  - Xbox (Xemu)
  - Neo-Geo (Libretro-Geolith)
  - Atari ST (Libretro-HatariB)
  - ZX Spectrum (Libretro-Fuse)
  - Nintendo DS (Libretro-MelonDS)
  - x68000 (Libretro-Px68k)
  - SCUMMVM (SCUMMVM)
  - Model 3 (Supermodel)
- Nombreuses améliorations dans les mises à jour de la liste de jeux :
  - Ne nécessite plus le redémarrage de l'interface
  - Le chargement initial supprime maintenant les jeux supprimés sans avoir à forcer une actualisation de la liste
  - L'interface détecte les modifications de fichiers dans les dossiers rom et invite à mettre à jour les listes de jeux
  - L'interface détecte désormais les modifications externes sur gamelist.xml et invite à mettre à jour les listes de jeux
  - La mise à jour des listes de jeux détecte les jeux ajoutés et invite à scraper immédiatement les nouveaux jeux en arrière-plan (exclue patron)
- Meilleure prise en charge de la rg353m (plus besoin de réduire la fréquence d'horloge).
- Libretro Scummvm
  - Option d'ctivation des sous-titres
  - Possibilité de choisir la langue dans le fichier .scummvm
- Affichage du nom de l'artiste dans la fenêtre contextuelle de la liste de lecture pour les mixtapes des patrons
- Ajout de la détection automatique des jeux Gamecube/Wii pour une meilleure config des manettes
- Ajout de la configuration automatique de l'eeprom Xbox à partir des options du frontend
- Ajout de la prise en charge du softpatching pour geargrafx & gearsystem
- Sélection automatique du bon shader pour les consoles portables lorsque vous utilisez des shadersets
- Configuration automatique du multitap pour psx et saturn
- L'activation du WIFI et la connexion WIFI sont désormais séparés
- Le scan des hotspots WIFI fonctionne lorsque le WIFI est activé dans le menu réseau
- Mise à jour des fichiers readme (et des fichiers readme localisés) dans les dossiers rom lorsque Recalbox est mis à jour
- Création automatique des répertoires/fichiers manquants sur le share :
  - Répertoires roms manquants
  - Fichiers readme manquants (.txt)
  - Fichiers Pad-2-Keyboard (.p2k.cfg)
- Passage au nouveau gestionnaire de textures :
  - Interface plus fluide, en particulier pour les supports lents ou les NAS
  - Correction des problèmes avec les thèmes riches en images
- Nombreuses améliorations dans les thèmes :
  - Nouvelles variables (globales, contextuelles et utilisateur)
  - Options libres : définissez vos propres options
  - Nouveau style d'écriture simplifié (utilisation d'attributs au lieu de nœuds)
  - Texte/image localisable
  - Logos par défaut disponibles pour tous les thèmes
  - ... et bien plus encore. Consultez le wiki pour plus d'informations
- Debounce des manettes dans l'interface pour éviter les doubles clics avec les contrôleurs à délai zéro
- Défilement des textes multilignes à l'aide du joystick droit
- Ajout d'une entrée dans le menu Système pour afficher les Release Notes de la version actuelle
- Affichage de la RAM totale dans le menu Système sur Raspberry Pi
- Désactivation par défaut de la fonction de rembobinage FBNEO pour éviter les problèmes de performances. Peut être réactivée dans les paramètres avancés de l'émulateur.

### Corrections
- Correction de l'affichage de messages cachés et/ou vides après certaines opérations utilisateur
- Correction du ratio sélectionné par l'utilisateur pour chaque jeu
- Correction des fichiers de configuration Supermodel et configgen (merci AxelR !)
- Correction du scanner bios qui ralentissait le démarrage lors de l'exécution de fichiers volumineux
- Correction de l'activation de RetroAchievements pour les nouveaux systèmes
- Correction du script intégré moonlight
- Correction des joysticks d'arcade gpio sur RPi4
- Correction du gestionnaire web sur Raspberry Pi 3
- Correction de la prise en charge de rg353m v2
- Correction du mode démo en ajoutant une image par défaut lorsqu'il n'y a pas d'image à afficher
- Correction de l'émulateur par défaut vers libretro-puae pour tous les systèmes Amiga sur RPi5 et PC
- Correction du chargement zip avec libretro-quasi88
- Correction des chemins d'accès au BIOS pour libretro-mgba
- Correction de la force 480i pour les jeux scummvm 240p lorsque 480i est sélectionné dans la fenêtre contextuelle de résolution
- Correction de la désactivation du menu de service dans fbneo pour les autres joueurs en netplay
- Correction de l'accès au jeu à partir de la recherche et du gameclip
- Correction du filtrage TATE dans le système TATE
- Correction des overlays qui ne fonctionnent pas en mode démo
- Correction des blocages et des plantages dans les fenêtres de recherche
- Correction du netplay en recréant les fichiers retroarch.corenames à chaque mise à jour
- Correction du plantage lors de l'utilisation de roms non reconnues uniquement dans les systèmes d'arcade (problèmes naomi2 dans le canal patron)
- Correction du fonctionnement du softpatching avec les lettres majuscules des extensions
- Correction des polices OSD en basse résolution
- Correction de l'interface utilisateur de la fenêtre de scraping d'un seul jeu
- Correction de l'interface utilisateur de la fenêtre de scraping en basse résolution
- Correction de l'affichage des boutons A/B dans l'aide lorsque l'inversion est activée
- Correction de certains plantages dans la gestion du son
- Correction des erreurs fatales dans le scraper (ignore le jeu défectueux et continue)
- Correction des résolutions en mode démo (rend l'overlay fonctionnel)
- Correction des fichiers P2K et de la fenêtre d'aide P2K
- Correction de plusieurs problèmes dans le filtrage TATE
- Correction de la prise en charge des contrôleurs avec mupen64plus
- Correction des overlays utilisateur qui empêchent les jeux de fonctionner sur un écran 4/3 (pour les consoles portables avec dock)
- Correction du meilleur score pour lr-mame et ajout d'un accès au commutateur DIP pour RGB JAMMA
- Correction de la prise en charge MSU pour libretro-genesisplusgx

### Mises à jour
- Bump retroarch to 1.21.0
- Bump xone (xbox joystick driver)
- Bump nvidia drivers
- Bump libretro-mame2000
- Bump libretro-snes9x2002
- Bump mupen64plus
- Bump ppsspp
- Bump vice
- Bump sdlpop
- Bump scummvm
- Bump dolphin-emu
- Bump rb5000
- Bump libretro-yabasanshiro
- Bump libretro-vitaquake2
- Bump libretro-snes9x
- Bump libretro-picodrive
- Bump libretro-mupen64plux-nx
- Bump libretro-fceumm
- Bump libretro-ecwolf
- Bump libretro-bluemsx
- Bump libretro-dosbox-pure to 1.0-preview2
- Bump libretro-ppsspp
- Bump Kodi plugins
- Bump libretro-vice
- Bump libretro-px68k
- Bump lr-vice
- Bump lr-mame to 0.274
- Bump lr-parallel-n64
- Bump lr-pcsx-rearmed
- Bump lr-sameboy (move to official repo)
- Bump hypseus-singe
- Bump Solarus to version 2.0
- Bump moonlight embedded
- Bump supermodel
- Bump Mesa3D to 25.1.0 for rpi4 and rpi5
- Bump libretro-dolphin
- Bump RetroArch cheats
- Bump libretro-quasi88
- Bump libretro-mednafen-saturn
- Bump libretro-gearsystem
- Bump libretro-genesisplusgx
- Bump libretro-genesisplusgxwide
- Bump libretro-desmume
- Bump libretro-melonds


## Version 9.2.3-Pulstar

### Fixes
- Fix data files, and game recognition in systems using Flycast & Flycast-Next cores
- Fix scraper not saving all gamelists when scraping virtual systems
- Fix multiple update pop when exiting screensaver
- Fix immediate update check after relaunching the interface
- Fix crash/freeze with auto-scraper conflicting with background scraping
- Fix blocked reboot/shutdown when scraping in background (save gamelists also)


## Version 9.2.2-Pulstar

### Fixes
- Fix jumping to netplay game when you're in a virtual system (Favorites, ...)
- Avoid 50Hz mode for pal games on JAMMA
- Fix Midway resolutions on CRT
- Add support for horizontal integer scale on rgb dual
- Fix rtype and mk display on 240p@120Hz
- Fix GX4000 emulation on libretro-cap32
- Fix C64 emulation on vice_x128
- Fix forcing OpenGLES for RPI5 and RPI4 for Libretro-ScummVM
- Fix preinstalled game with ScummVM and Libretro-ScummVM
- Fix OpenGL error with Libretro-ScummVM on PC
- Fix kodi crash when playing HEVC/h264 video on rpi4
- Fix kodi not playing HEVC video on rpi5 (still no 4k though)
- Fix preinstalled games:
  - Fix all ScuimmVM games
  - improve "The Tinnies" Pad-2-keyboard
  - improve "Sword of Ianna" Pad-2-keyboard
- Fix Flycast performance & regression issues
  - New flycast core available on Naomi2, Naomi/GD, Atomiswave & Dreamcast
  - Old flycast core is the default core for all systems above but Naomi2
- Fix Daphne emulation on Hypseus
- Fix EmulationStation crash when changing audio output device (on some PC)
- Fix rpizero2 and rpi3 shifted screen bug


## Version 9.2.1-Pulstar

### Fixes
- Fix GX4000 & CPC+ graphic glitches (libretro-caprice32)
- Fix bios checking when launching games
- Update system translations
- Fix launch Indiana Jones and the Fate of Atlantis with ScummVM
- Add core libretro geolith for crt
- Fix launch demo gladmort on neogeo
- Fix saturn and psx zoomed games on CRT
- Fix game information space on 240p theme in tate
- Fix gameclip view on 240p theme
- Fix jamma fan not stopping when shutdown from menu
- Fix 240p@120Hz mode for CRT
- Fix forcing OpenGLES for RPI4 for Libretro-ScummVM


## Version 9.2-Pulstar

### Hardware
- Add support for Raspberry Pi 5 (64 bits)
- Add support for Recalbox RGB JAMMA
- Add support for Anbernic RG351v (Thanks BelgaWill !)
- Add support for Anbernic RG351p and RG351m (Thanks lxmaster1 & yomax69fly !)
- Add support for Realtek RTW-88 series of wifi controllers
- Add support for N64 and Megadrive/Genesis controllers from Nintendo Switch Online
- Add support for eeprom update on RPi4 and RPi 5
- Add auto mapping for retroflag snes usb and hutopi controllers:

### News
- Add Arcade View
  - Tree view of Parents/Clones
  - Separated bios/unknown roms
  - Use HotKey+up/down to fold/unfold all
  - Use Hotkey+left/right to fold/unfold current selection
  - Use official names from DAT files
- Add Arcade manufacturers virtual systems (Atari Classics, CPS1/2/3, Banpresto, IREM, ...)
- Add brand new Web Manager (accessible on http://recalbox.local)
- Add Arcade settings in the main menu
- Add Input OSD (icon selectable among most common pads)
- Add mouse controls:
  - use vertical wheel to move up/down
  - use horizontal wheel to move left/right
  - left/middle/right buttons act as b/start/a
- Add new system Sega Pico
- Add Naomi 2 system on rpi5 and x86_64
- Add new time played information
- Add "Boot on game" feature to enable recalbox booting on a selected game
- Add undervoltage and temperature popups in frontend for rpi familly
- Add new Saturn core on RPi4 and RPi5: beetle saturn
- Add support for HD mode on Pi5, PC (Dreamcast, Naomi, Atomiswave, Playstation, Saturn)
- Add support for Widescreen mode on Pi4, Pi5, PC (Dreamcast, Naomi, Atomiswave, Snes, Megadrive)
- Add latest libretro mame core (0.258) for RPi4 and RPi5
- Add RPi5 overclock option
- Add skip boot video option
- Add demo Inspector Waffles Early Days (Broke Studio) on gameboy color
- Perf: enable auto frame delay in retroarch
- Perf: add auto blitter for fbneo cave games
- CRT: add superrez and auto scanlines for 240p games on 31kHz display
- Add new core libretro-dirksimple
- Add new port SDLPop (port of Prince of Persia)
- Add libretro-dolphin (MagicSeb)
- Add possibility to adjust music volume in EmulationStation
- Add en_GB translation (SecularSteve)
- Add new core geolith for neogeo
- Add GladMort Demo on neogeo
- Add libretro-scummvm with support of multi resolution for CRT
- Add dynamic themes: change theme or theme options without rebooting
- Add new core libretro fake08 for pico8

### Improvements
- Show a battery indicator in EmulationStation for Piboy DMG and XRS
- Controllers:
  - Unplugged/plugged-in pads keep their positions
  - Bluetooth auto-pairing is started at each boot now
  - Fix pad ordering
  - Fix analog stick with inverted axis behaviour
  - Fix not mappables pads
  - Fix some pandorabox controllers for 2 players
  - Fix some Xbox 360 pad issues
- Add new softpatching LAUNCH LAST option
- Disable unnecessary services when use the Recalbox RGB Jamma
- Allow starting an update stright from the update popup
- Add missing favorites icons for some systems (Thanks Bounitos and Malixx !)
- Add m3u support on Dolphin with auto change disc

### Fixes
- Fix argonone case power management
- Fix gpu overclock when overclocking on RPi4
- Fix multi-wifi configuration
- Fix demo Astebros for megadrive
- Fix odroidxu4 OGST video not playing
- Removed LinApple from PC
- Fix headphone audio output on Anbernic rg353v
- Fix vulkan support on rpi5
- Fix daphne games scrap

### Bumps
- Bump RetroArch to version 1.15.0
- Bump libretro-mame to 0.258
- Bump odroidxu4 kernel to 6.1.49
- Bump libretro fbneo to 6b6ba3
- Bump libretro-mame2003+
- Bump advancemame
- Bump libretro-atari800
- Bump libretro-a5200
- Bump libretro-wasm4
- Bump libretro-gambatte
- Bump libretro-sameboy
- Bump libretro-mgba
- Bump libretro-meteor
- Bump libretro-tgbdual
- bump libretro-nestopia
- Bump libretro-mesen
- Bump libretro-mednafen_lynx
- Bump libretro-pokemini
- Bump libretro-opera
- Bump libretro-o2em
- Bump libretro-freeintv
- bump libretro-freechaf
- Bump libretro-fmsx
- Bump libretro-neocd
- Bump libretro-mu
- Bump libretro-mednafen_pcfx
- Bump libretro-kronos
- Bump libretro-mednafen_saturn
- Bump libretro-yabasanshiro
- Bump libretro-yabause
- Bump libretro-mame2000
- Bump libretro-desmume
- Bump libretro-melonds
- Bump libretro-mame2015
- Bump libretro-mame2003
- Bump libretro-xrick
- Bump libretro-xmil
- Bump libretro-virtualjaguar
- Bump libretro-vecx
- Bump libretro-uzem
- Bump libretro-tic80
- Bump libretro-supafaust
- Bump libretro-snes9x2010
- Bump libretro-snes9x2005
- Bump libretro-sameduck
- Bump libretro-reminiscence
- Bump libretro-race
- Bump libretro-quicknes
- Bump libretro-quasi88
- Bump libretro-prboom
- Bump libretro-ppsspp
- Bump libretro-potator
- Bump libretro-nxengine
- Bump libretro-np2kai
- Bump libretro-mrboom
- Bump libretro-minivmac
- Bump libretro-mednafen_wswan
- Bump libretro-mednafen_vb
- Bump libretro-mednafen_ngp
- Bump libretro-lutro
- Bump libretro-lowresnx
- bump libretro-gw
- Bump libretro-gearsystem
- Bump libretro-gearcoleco
- Bump libretro-fuse
- Bump libretro-fceunext
- Bump easyrpg
- Bump libretro-fceumm
- Bump libretro-theodore
- Bump libretro-crocods
- Bump frotz
- Bump libretro-cap32
- Bump libretro-cheats to 1.15.0
- Bump libretro-snes9x
- Bump libretro-mednafen-psx_hw
- Bump libretro-mednafen-supergrafx
- Bump libretro-prosystem
- Bump libretro-mojozork
- Bump libretro-handy
- Bump libretro-px68k
- Bump libretro-uae
- Bump libretro-genesisplusgx
- Bump libretro-mednafen_psx
- Bump libretro-mednafen_pce_fast
- Bump libretro-genesisplusgx_wide
- Bump lr-mupen64plus_nx
- Bump libretro-bk
- Bump libretro-bluemsx
  - Compatible with SG-1000 games
- Bump libretro-stella
- Bump libretro-bsnes
- Bump libretro-swanstation
- Bump libretro-pcsx_rearmed
- Bump libretro-gpsp
- Bump libretro-81
- Bump hypseus-singe
- Bump libretro-parallel_n64
- Bump libretro-emuscv
- Bump Anbernic rg353x kernel
- Bump libretro-picodrive
- Bump hatari


## Version 9.1-Pulstar

### Hardware
- Add support for Anbernic rg353m, rg353p, rg353v
- Add support for PiBoy XRS

### News
- Add TATE mode support for HDMI, CRT and handhelds
- Add Game Downloader for free games of WASM4, Vectrex, UzeBox & TIC80 systems
- Add start a game from savestate menu
- Add new system Arduboy (libretro arduous)
- Add new system Wasm4 (libretro wasm4)
- Add support for Super Game Boy games
- Add CRT Curved shaders support
- Add new core for Philips CDI (libretro samecdi, for PC only)
- Add 240p test suite support for all platforms
- Add libretro-mojozork as Z-Machine core
- Add libretro-supafaust core for Snes
- Add support for break key for pad2keyboard

### Bumps
- Bump FBneo to aad581
- Bump Hypseus-Singe to v2.10.4
- Bump Netflix and Youtube plugins in Kodi

### Improvements
- Add wizard to disable smooth game when enabling shaders
- Add recommended info for scrap video selection in menu
- Improved upgrade method, faster to upgrade and no need to have free space in share anymore

### Fixes
- Fix crash when using run in background after the end of a scrap
- New image for RPIZero (first one) and GPiCase2W
- Add missing netplay support for sameboy and mgba
- Fix naomi and atomiswave resolution in 240p for CRT
- Fix Mortal Kombat resolution on CRT
- Add missing joystick support on GPi Cases


## Version 9.0.2-Pulstar

- Fix black screen on older PC with Intel graphic card

## Version 9.0.1-Pulstar

### Fixes

- Fix black screen on older PC with Intel graphic card
- Add Ukrainian, Persian, Deutch (CH), and Slovak languages
- Fix resolution selection


## Version 9-Pulstar

### News

- Add RetroFlag GPi2W case support with only Raspberry Pi Zero2 board
- Add Raspberry Pi Touch Display case support for Raspberry Pi 3 and 4
- Add support for 4k video in Kodi for Raspberry Pi 4
- New bluetooth system
  - Audio devices can be paired in sound settings
  - Joysticks and joypads will be automatically paired on first boot
  - New background pairing, letting new devices pair for 5 minutes
- Add 'Z-Machine' emulator with new core frotz
- Add 'Philips VG5000' emulator (RB5000, a new Recalbox's emulator based on DCVG5K)
- Add 'Macintosh' emulator with new core libretro-MinivMac (RPi3/RPi4/XU4/OGS/PC)
- Add 'C64' Vice standalone emulator 3.6.1
- Add game Pong Recalbox Edition by Studio 40A on Gameboy
- Add demo Astebros (Neofid studio) on Megadrive
- Add 240p theme as default for GPiCase 1
- Add Playstation core libretro-pcsx-rearmed for OGA/OGS
- Add new category 'Demoscene' in tic80 games downloader.
- Add support for nVidia Prime Offloading (for computers with both iGPU and nVidia GPU)
- Recalbox RGB Dual:
  - Add multiple resolution calibration screen
  - Add HDMI priority option in recalbox rgb dual to switch between Scart and HDMI
  - Add neogeocd, picodrive and pcsx rearmed, cores for rrgbd
  - Add snes9x2002, 2005, 2010 and bsnes support for rrgbd
  - Add region selection for satellaview, sufami, amiga 600, amiga 1200
  - Add fceumm core support for nes and fds
  - Fix rtype resolution for 31kHz
  - Add new arcade game support for 15kHz.
  - Add mupen64plus glide64mk2 support
  - Add multiple PAL/NTSC modes
  - Add 240p@120Hz frontend resolution for 31kHz screens
  - Add kodi resolution menu selection and dynamic resolution for CRT
  - Add new Kodi theme for 240p/480i resolutions
- Add H/W video decoding for Raspberry Pi 4, Pi 3 & Pi zero 2

### Improvements
- Add the possibility to sort games by release date.
- Enhance shader support by setting multi-threaded renderer when shader enabled
- Improve metadata storage, using far less memory for huge game collections
- Bump libretro pcsx-rearmed
- Bump libretro gong
- Bump libretro fuse & add new supported file extensions
- Bump libretro bluemsx
- Bump libretro neocd
- Bump libretro race
- Bump libretro uae4arm
- Bump bebeem
- Bump retro8
- Update of the theme for the X68000 system
- Add support for BCM20702, BCM20703, BCM43142 bluetooth chipsets
- Bump libretro gPSP
- Bump libretro genesisplusgxwide
- Bump libretro Beetle Lynx
- Bump libretro Opera
- Bump libretro fbneo
- Bump libretro uae
- Bump libretro dosbox-pure
- Improve AppleIIGS configurator & add eXoAppleIIGS config support
- Bump libretro-mesen

### Fixes
- Fix resolution selection in frontend for Raspberry Pi 4/400/3/Zero2 (KMS)
- Fix crash libretro pcsx-rearmed for x64
- Add .wia file extension for Gamecube and Wii + .gcz for Wii.
- Fix pad 8bitdo Pro 2 mode D (Android)
- Fix bad overlay showing when save on exit is enabled on retroarch
- Fix support for Ultimarc IPAC 2
- Fix internal scraper saving path before actual media
- Fix resolution selection in frontend for Raspberry Pi 4/400/3/Zero2 (KMS)
- Controllers recognized as Xbox in Dolphin (GC/Wii) have their hotkeys working
- Fix pause menu in the game for libretro pcsx_rearmed

## Version 8.1.1-Electron

## Fixes

- Fix older nVidia adapters support (PC)
- Fix framerate auto selection for video mode on retroarch (RPi 3/4/400/Zero2)
- Duplicate favorite only option to show entry in gamelists menu
- Fix audio menu crash
- Fix timeout for patron authentication
- Fix bad free space report in system menu
- Enhance scraper performance


## Version 8.1-Electron

### News

- Add Raspberry Pi 4 64 bit support
- Add Recalbox RGB Dual support
- Add support for external rom folders on USB devices (up to 8 volumes/partitions)
  - Hotplug: EmulationStation automatically reconize USB device containing roms!
  - Initialization: Ask the user when USB device with no rom folders at all
  - Roms folders are reconized in the root of the usb device or in `roms` or `recalbox/roms`
  - Games are merged into your game list seamlessly
- Add support for additional network rom folders (up to 4)
  - As for USB devices, games are merged into your game list seamlessly
- Add case support for retroflag, argon and nuxii cases
- Add Pi400 power button support (Fn + F10 to turn off)
- Add remote playlists (Patrons)
- Add seamless scraping (Patrons)
- Add GAME FILTER menu to filter games in lists based on several criteria
- Add slangs shaders support on retroarch
- Add system Philips CD-I (libretro core : cdi2015)
- Add BSNES core (performance build) (PC and RPi4/400)
- Add BSNES HD for wide screen Super Nintendo games (PC and RPi4/400)
- Add libretro PSP emulator (libretro ppsspp)
- Add delete game options
- Add the game Zdey The Game on Nintendo Nes. Thank you Art'cade
- Add core libretro vitaquake2 for x64, RPi4, Odroid Xu4 and Odroid Go Advance/Super
- New system sorting options in UI menu: sort your systems by type, manufacturer, release dates and more !
- Add Iris driver for Intel iGPUs on x64 version
- New option in UI menu : hide PORTS collection
- Add recalbox.conf option to force resolution of splash videos on x64
- New Random game picker in demo mode (less cycling)
- Display game regions flag in gamelist
- Add Twitch as Kodi plugin
- Add resolution menu in Advanced Settings > Resolutions
- Add scraper progress bar
- Add display by filename in gamelist option
- Add go to game from search results and add search games from gamelist menu
- Add retroachievements for systems PSP, Amstrad CPC, Dreamcast, Atomiswave, Naomi and Naomigd
- Add Amiga 500 Mini controller configuration
- Add softpatching manager in game settings
- Add system MegaDuck (libretro core : sameduck) Thanks Duimon for the drawing of the console on the theme.
- Add new Atari 5200 core (libretro core: a5200)

### Improvements

- Improve region processing in the internal scraper
- Alert the user if too much systems or games are loaded regarding available memory
- Bump EmuSCV (Super Cassette Vision)
  - Fix zipped rom handling
  - Handle multi-roms (.0 .1 ...) in a single zip file
  - Correction of the CRC32 entry
  - Add the new LupinIII md5 so that it is recognised and mapped correctly in memory
- Bump libretro Caprice32 (Amstrad):
  - Update to Caprice 4.5.2
  - Lots of fixes & improvement with latest Retroarch versions
- Bump Retroarch to v1.10.0
- Bump libretro Gearcoloeco (fix graphics glitches, fix PAL refresh rates, add bios alert)
- Bump libretro Picodrive - lots of small fixes & improvements
- Bump libretro NX Engine (Cave Story) - Lots of small bug fixes!
- Bump Amiberry to v1.4.6 (fix some graphic glitches)
- Bump libretro Quasi88 (Nec PC-8800)
- Bump libretro MAME 2003 Plus
- Bump libretro FBNeo
- Bump SimCoupe to v1.2.8 (fix some tape issues & update spectrum library)
- Bump libretro FCEUMM (NES) - Lots of bug fixes & increase compatibility with new mappers
- Bump Beetle Saturn (add netplay support)
- Bump libretro mame2015
- Bump libretro mame 0.239
- Improve vectorial image quality (particularly on low resolutions) - Thanks to @rymcclel
- Bump libretro fuse to v1.6.0 (ZX Spectrum core)
- Add support for many bluetooth controllers on Odroid GO Advanced and Go Super (notably Realtek one)
- Bump Hatari: updated cpu emulation and lots of small fixes
- Disable Retroarch's keyboard shortcut when running computer systems:
  - Full keyboard is available for the emulated computer
  - Use F12 as ESC replacement to quit
- Bump Hypseus Singe (Daphne) - Add support for Singe 2 games
- Add support for additional WIFI USB adapters on Odroid GO Super (Mediatek, Realtek, …)
- Bump Realtek 8821-CU driver
- Bump libretro Pscx-Rearmed
- Bump Libretro Sameboy
- Bump Parallel N64 to add Raspberry Pi 4 64 bits support

### Fixes

- Fix kodi controller configuration
- Fix swanstation memory card compatibility option for saving on network
- Fix lots of issues in the internal scraper
- Add mc10 and coco3 model to trs80 color computer xroar emulator
- Add CRT check menu footer
- Fix es_input for 8bitdo SN30Pro+ and Xbox One S Controller Model 1708
- Fix libretro mame2015 for OGA/OGS
- Fix to not reload recalbox.conf file in manager on save
- Deleting the netplay system configuration in recalbox.conf
- Fix some ZeroCD wifi adapters not working like Odroid Wifi Module 5B adapter
- Fix kodi running at startup
- Fix savestate in libretro fuse (ZX Spectrum core)
- Fix Vectrex overlays load for preinstalled games
- Fix wifi not connecting when inserting the wifi controller afterwards
- Fix extension bin for the watara supervision system (libretro potator)
- Fix wifi password that could not contain & character
- Fix extension .bin for Nintendo 64DD
- Fix reversal of buttons A and B on SF 30 PRO controller
- Add buttons x and y for keyboard
- Fix black screen/no signal on Pi3 & Pi4
- Fix Neo-Geo CD file extensions
- Fix extension .bin for Gameboy and Gameboy Color
- Fix netplay for mame
- Fix PiBoy DMG losts its joystick controller and volume control after first boot
- Fix repo rockchip-mali and librga

## Version 8.0.2-Electron

- Fix 4k resolution issues on RPi4
- We are sorry but we must rollback the "fix" of the black screen as it caused other issues.
- If you still have an issue with black screen after this upgrade, please follow the instructions on https://fix.recalbox.com
## NEXT

### Improvements
- Add amiga support for Raspberry Pi Zero and one through new uae4all core
- Add native support for GPi Case 2 / CM4


## Version 8.0.1-Electron

- Fix controllers not being detected after controller sleep mode or unplug/replug
- Fix black screens with some HDMI screens on Raspberry Pi3 and Pi4
- Fix TV Remote controller (CEC) not working with Kodi on Raspberry Pi3 and Pi4
- Fix Raspberry Pi Zero 2 performances issues
- Add Mupen64plus GLide64MK2 on RPI3
- Add a better text when you are stuck in the favorites list in any system.
- Fix controllers in Duckstation (access menu with HK+B)
- Fix frontend not displaying two letter extentions games (.md/.st)
- Fix "Configuration" tab save feature in manager.
- Fix PC88 & Spectravideo scraping
- Fix rotated Digital Pad in PCSX2 (PS2 emulator)
- Fix tearing effect in frontend on Odroid GO Advance and Odroid GO Super
- Fix crash on Mupen64plus GLideN64 on RPI3
- Fix automatic audio switch to PlayStation DS4 joypad jack
- Fix Mupen64plus and PPSSPP on PiBoy DMG
- Fix Duckstation's menu: Navigate with your controler
- Fix WPA3 wifi connection
- Fix shader crt-pi for Raspberry Pi boards
- Fix Pin 5/6 and RetroFlag safe shutdown scripts to use with cases
- Add GSplus emulator to run Apple 2
- PiBoy DMG pad is now detected in Kodi
- Fix annoying error popup when kodi starts
- Fix 'This title is not available to watch instantly' error with Netflix in Kodi
- Fix no sound issue on some PCs

## Version 8.0-Electron

### News
- Add Demons of Asteborg Demo on Megadrive
- Add support for new Raspberry Pi Zero 2
- Add Hatari standalone emulator (Atari ST/STE/TT/Falcon)
- Add new ColecoVision libretro core: Gearcoleco
- Add BBC Micro (Beebem emulator)
- Add TI-99/4A system, emulated using the ti99sim standalone emulator
- Add Dragon 32/64 system (XRoar standalone emulator)
- Add TRS-80 Color Computer (XRoar standalone emulator)
- Add Duckstation standalone emulator (PSX)
- Add ps2 system with pcsx2 emulator
- Add ps2 system with pcsx2 emulator & libretro-pcsx2 core
- Add CRT TV and monitors support
- Add genesisplusgxwide, the genesis/megadrive widescreen core!
- Add libretro fbneo core for channel f system
- Add libretro uae4arm core, an Amiga core optimized for ARM boards!
- Add stella2014 core on RPI1/Zero and RPI2 as it is more performant than stella
- Add recalbox overlays for all systems!
- Add LowRes NX virtual console on all boards
- Add CHD support for Mega CD Games with Picodrive core
- Add Saturn on Raspberry pi4 (libretro core: Yabasanshiro)
- Add gamelink overload for gb and gbc
- Add recalbox-wpaf tool for controlling various RPI fan controllers
- Add libretro Gong core on all boards
- Add Xbox One wireless dongles support (xow userspace driver daemon from Medusalix)
- Add PiBoy DMG support

### Improvements
- Improve ergonomy in gamelists when "quick system select" is enabled
- Add new Netplay's MITM servers: Montreal & SaoPaulo
- Bump libretro MGBA (Gameboy Advance core)
- Bump Libretro PX68K (Sharp x68000)
- Bump Oricutron to latest version
- Bump SimCoupé to v1.2.5
- Bump Libretro Beetle PSX
- Bump Libretro MAME2003 Plus
- Bump Libretro-MAME2003 to include latest fixes
- Bump Libretro VICE to include new options and latest fixes
- Bump Amiberry to v4.1.5. Add CDTV support
- Bump Retroarch to v1.9.3
- Bump Retroarch to v1.9.8
- Bump Hypseus to version 2.6.17 (DirtBagXon) to support Singe
- Bump Fbneo v1.0.0.02
- Bump Sharp X68000 core (px68k)
- Bump Flycast
- Bump FreeIntv core (fix audio glitches)
- Improve Netplay window
  - More responsive & faster to get client latencies
  - Manual refresh with START
  - Seamless auto-hashing when Netplay is on
- Bump Kronos libretro core
- Bump Mame 0.235
- Add new Pad-to-Keyboard capabilities:
  - Mouse emulation using joystick or dpad.
  - Keystroke sequences (like typing a word)
  - Keystroke combination (like CTRL+C)
- Bump ScummVM to 2.4.0git
- Bump pcsx_rearmed on Raspberry PI 0 and 1
  - Add CHD support

### Fixes
- Fix controllers behavior in WII & GC systems (dolphin)
- Fix message "failed to set last used disc" for core libretro px68k
- Fix laggy video playback on Odroid XU4 OGST
- Fix message "failed to extract content from compressed" for core libretro xrick
- Fix retroarch 1.9.8 to correct issue on wrong lightgun shooting area
- Fix loads of bugs on pcsx_rearmed on Raspberry PI 0 and 1
- Fix static WIFI IP configuration not working
- Fix non-working shaders (removed)


## Version 7.2.2-Reloaded

### Fixes
- Fix mupen64plus resolution (standalone)
- Make mupen64plus working on Odroid Advance Go2/Go Super (standalone)
- Make mupen64plus + RICE working on RPi4
- Fix ratio issues in Dolphin & Supermodel3
- Switch PUAE to A600 instead of A500PLUS for amiga600 system
- PSP (ppsspp) saves are now in (share/saves/PSP)
- Fix controler #2 configuration in swanstation
- Fix sound issue on boot video
- Audio is working on Sound Open Firmware audio adapters
- Added configuration for SN30-GP controller
- Fix OGST display not working on Odroid XU4
- Fix no boot black screen on some PCs
- Fix realtek wifi driver for rtl8812au and rtl8821au
- Fix missing rtl8821cu wifi drivers for Odroid XU4, RPI2, RPI3 and PCs
- Fix some bluetooth adapters not working when detected as HID device (like some Apple or Dell devices)
- Add Mupen64plus Next libretro core on PC platforms
- Bump Mupen64plus standalone emulator
- Fix Libretro Parallel N64 video plugin selection to Rice (better speed on selected H/W)
- Make Libretro Parallel N64 works on Odroid XU4
- Make Mupen64plus Rice works on Odroid XU4, Odroid GO Advanced and Odroid GO Super
- Fix black screen on Moonlight on RPI4
- Fix the time conflict between Windows and Recalbox in case of dual-boot
- Make Mupen64plus GLideN64 works on Raspberry PI3
- Fix MAME menu is displayed if you add credit with the wiimote
- Make Mupen64plus Glide64MK2 works on various boards
- Fix image display on dreamcast when using overlays
- Fix ratio selection for gamecube, wii and supermodel
- Bump supermodel to r862 and enable build optimisation


## Version 7.2.1-Reloaded

### Fixes
- Bring back fan speed setting on Odroid XU4
- Fix loop reboot with non official themes
- Lightgun system is shown by default if any game is detected
- Kodi video playback is back on Rpi 4
- Reicast has been removed from Rpi, PC and XU4 as this emulator does not work anymore with our new kernel and softwares versions. You can now use Flycast as the main emulator for the dreamcast.
- On PSP, with PPSSPP emulator, videos now run full speed
- Raspberry Pi 1,2,3 boot video now run full speed
- Fix arcade view systems missing model3, atomiswave, naomi, naomigd, daphné
- Duckstation has been renamed Swanstation
- Fix Swanstation issues when loading games
- Fix cavestory x86 and x86_64
- Fix House of the dead bios validation
- Add missings game extensions detection for SCV and WII
- Default language is now used when scraping
- Fix gameclip initialization on network shares
- Fix freezes when running a game while playing gameclip
- Using IPv4 in priority when connecting WIFI
- Factory reset now reset case configuration
- Fix rating componant running out of screen
- Fix sound menu & output auto-selection
- Fix Odroid Go Advance V1 gamepad detection
- Fix Super Cassette Vision scraping
- Fix rtl88x2bu Realtek wifi driver
- Fix "Forget all bluetooth devices"
- Fix pixel-perfect settings being incorrectly set in menu
- Fix update popup appearing over Gameclip Screensaver
- Fix hiding a game wipe all its metadata
- Fix theme display after switching theme or editing theme options
- Fix some files referenced in .cue/.gdi/.m3u not being ignored by the scraper
- Force volume of all outputs to 100%
- Fix crash when running a game from the search window
- Fix GBA gpsp emulator


## Version 7.2-Reloaded

### News
- Bump libretro-mame
- Bump Datfiles for Mame, Atomiswave, Naomi, NaomiGd, Model3 on Romset 0.230
- Bump of the port family
- Bump Dolphin Emulator to Version 5.0-13178
- Bump Buildroot to version 2020.02
- Bump Kernel to 5.8 (except on Odroid Go Advance)
- Add WebManager v3
- New multi-source rom management:
  - Pre-installed games are no longer copied into SHARE partition (except ports)
  - New option in game menu to show/hide pre-installed games
- New "GameClip" screensaver playing games' videos in many configurable ways
- Show feature slides during install
- Add Super Cassette Vision core
- Add Libretro NeoCD core (w/ CHD support)
- Add Nintendo Switch joycon support (both solo or combined mode)
- Add new libretro core for DOS games (dosbox-pure available on rpi3/4, x86/x64 & Xu4.Goa)
- Add Pulseaudio support
- Add PICO-8 core (libretro-retro8)
- Add Pocket Challenge v2 system (pcv2)
- Lightgun feature : more than 200 games already ready to play! (using dolphin bar/wiimote)
- Add Minecraft's port Craft on pc
- Add support for Odroid Go Super
- Add Watara SuperVision System on all boards
- Add Elektronika BK series on all boards
- Add VideoPac+ System
- Separate VIC-20 system from C64
- Add sound mode option (Music/Video sound)

### Improvements
- Bump supermodel to version 858
- Add frequency screen option for X86, X86_64 by Chriskt78
- Bump Amiberry to version 3.3
- Bump Stella (A2600) to fix lots of games (Pitfall II among many others)
- Set default overlay to vc4-kms-v3d on RPI4 (better 3D performance)
- Bump and Patch Beetle-Saturn for best support of european Virtua Cop games
- Patch Retroarch for X11 multi-mouse
- Flycast crossairs updated
- Improve menu ergonomy
- Improve internal scraper
- Bump libretro-uae adding CHD support
- Bump ScummVM and merge with ResidualVM
- Bump BlueMSX
- Bump rockchip-libmali (for odroidgo2 and 3)
- Bump Hypseus to version 1.3
- Bump Duckstation, PSX games now run full speed (for odroidgo2 and 3)
- Bump Fbneo
- Bump Libretro Mupen64plus Next
- Bump Odyssey²: Add support for european & french models
- Bump Np2kai NP21/W rev.79
- Bump nVidia video drivers, add support for RTX 20x0 and RTX 30x0
- Bump PPSSPP, PSP games now run far more quickly (for odroidgo2 and 3)

### Fixes
- Fix bypass configgen option for model3
- Fix reactivation of the deleted mesa 3d demos package
- Fix Dolphin emulator
- Fix Odroid Go Advance controller (v1)
- Check recalbox image integrity before upgrading
- Fix lost support of open and WPA-PSK-SHA256 wifi networks
- Fix 8Bitdo NES30 Pro rev 1 mapping (thanks @fredbcode)
- Fix name for palmos rom (thanks @RaZer0k)
- Fix 4k resolution handling on rpi4
- Fix shaders on odroidgo2
- Fix RetroArch shortcuts on odroidgo2
- Fix Libretro Parallel N64 core on odroidgo2 & Pi4
- Fix fan not stopping on shutdown on nespi4case
- Fix libretro-mame, now full speed

## Version 7.1.1-Reloaded

### Fixes
- Fix abnormally long boots
- Fix Sega Model 3 missing sound option
- Fix theme issues when switching themes or theme options
- Fix save required when exiting metadata edition w/o changing anything
- Fix popup settings not saved
- Fix quick system change setting not saved
- Fix Supermodel build and move Nvram path
- Fix Demo mode informùation screen
- Fix P2K hanging on game exit


## Version 7.1-Reloaded

### News
- Add model3 arcade system
- Add WPS support for quick WIFI configuration
- Recalbox is distributed on Noobs (again) and Raspberry Pi Imager
- Add Raspberry Pi400 support
- Add Odroid Go Advance board support
  - Run all emulators up to Naomi/Dreamcast (smoothly) or even Saturn/Atomiswave (not all games)
  - Full power-saving support (slow down or speed up CPU/GPU/Ram regarding the running emulator)
  - Suspend/Soft Power-off using POWER button
  - Automatic audio switch between speakers/headpĥones
  - Volume & Brightness buttons
  - First-boot wizard
- Add 8 Broke Studio's game demos on NES!
- Add 4 MAME games from the '80

### Improvements
- Improved NESPi4 case detection
- Libretro-atari800 with .car extension support
- Check free space before downloading updates
- Bump Picodrive
- Bump FBNeo to v1.0.0.0
- Add FBNeo as core for
  - Colecovision
  - Gamegear
  - Famicom Disk System
  - Master-System
  - Megadrive
  - Msx 1
  - NES
  - NeoGeo Pocket
  - PC-Engine
  - SG-1000
  - SuperGraphx
  - ZX-Spectrum
- Add overlay global/per-system settings support in share/overlays
- Recalbox is back in Window's Network
- Update BIOS database
- Improved Flashback support: just copy your disk or CD game into roms/ports/flashback
- Bump Kodi to 18.9

### Fixes
- Fix AdvanceMAME
- Fix freezes on megadrive/picodrive
- Fix broken WIFI on some Intel chips
- Bring back all firmwares
- Fix boot from SSH/HDD on rpi4
- Fix arcade hash calculations
- Fix game extension filtering
- Fix screenshots on x86/x64
- Fix multiple issues in ScummVM/ResidualVM: lag/crash on Pi4, joystick buttons, ...
- Fix upgrade procedure that could fail in some circumstances
- Fix battery icon on PC
- Display p2k hints properly, including folders's p2k files
- Fix Vic20 games not running
- Fix inconsistent MD5 on multi-file zipped bios (neogeo.zip, ...)
- Fix gamelist bein overwritten when edited from outside Emulationstation
- Fix scrapper settings not always being saved
- Fix VirtualKeyboard on resolution higher than 720p
- Fix UAE configuration for CDTV/CD32
- Fix UAE kickstart copy for WHDL games
- Fix PSP mapping configuration
- Fix Mupen64 resolution on x86/x64


## Version 7.0.1

### News
- Add Sigil in PORTS (https://www.romerogames.ie/si6il)
- Add Kubo 3 on NES (dale_coop & seiji)
- Add raspi-gpio to manage GPIO in scripts
- Add new boot videos (GBA & Amiga)
- Add missing bios report file (share/bios/missing_bios_file.txt)

### Improvements
- SNES default emulators pi0/1: pisnes, pi2/3: snes9x2010 & snes9x elsewhere
- Bump FBNeo - Emulation improvements + fix neogeo-CD audio artifects
- Bump linux-firmware to 2020.09.18
- Bump kodi to 18.8
- Make pc98 playable with pads/joysticks

### Fixes
- Fix Missing dat files mame, naomi(Gd), atomiswave
- Fix Netflix plugin authentication (bump to v1.10.0)
- Fix RPI4 blackscreen issues while booting
- Fix doom.wad required for WADs games (prboom core)
- Fix Mupen64 L1/R1 mapping
- Fix DosBOX not returning back to emulationstation
- Fix pads in reversed order by default
- Fix recalbox.conf modifications getting lost
- Fix missing media being not updated using internal scrapper
- Fix outdated bios list in webmanager
- Fix zfast and crt-pi shaders' path
- Fix "Last Played" metadata & display in gamelist
- Fix missing psx bios md5
- Fix hdmi-cec not working on rpi4
- Fix PS4 pad pairing issue
- Fix duplicated entries in "Last Played"
- Fix Ports & aAcade virtual system not searchable
- Fix empty card names in audio output device list
- Fix no sound through jack on RPi1/2/3
- Fix information popup positions
- Fix Oricutron bios path
- Fix false update message
- Fix X-Arcade driver. Now supports v2 panels and other keyboard encoders
- Fix ShaderSets
- Fix mouse not working on rpi4
- Fix multi-file zipped roms hash calculations (arcade)
- Add more hard-patched xbox pads
- Fix idroid pad for kodi
- Fix connected/disconnected status in Network menu
- Fix i18n plural forms (Chineese language are now working)
- Fix video player behavior while reading invalid files
- Fix NESPi4 case shutdown script
- Fix VirtualKeyboard red mess on Pi4
- Fix crash while quitting gamelist menu in virtual systems
- Fix main menu still accessible while it should not be.
- Fix crash in p2k hints with some p2k definitions
- Fix GPI case shutdown script
- Fix screensaver triggered while downloading updates
- Fix rpi4 random freeze


## Version 7.0

### News
- New compatible board: Raspberry Pi4
- Recalbox system is now a firmware, more robust than ever:
  - Automatic recovery after 3 boots failure (reset to factory settings)
  - Brand new update system, easier and faster
- Share partition is created in exfat on fresh install. Access your share partition from your Windows PC!
- Add BIOS Window in EmulationStation to list missing/incorrect Bios:
- Add new BIOS XML list aware of mandatory/optionnal bios and multiple working Bios signatures.
- Add sorting options: Publisher and system-name for all virtual systems **
- Add 3 new virtual systems: All-games, Last-Played and Multiplayers **
- Add virtual systems per genre (RPG, Shoot'em up, Pinballs, ...) **/*
- Add Region highligting in gamelist (highlight games from your favorite region) **
- Add Adult game filtering in EmulationStation **
- Add laptop switch to external screen (x86/x64)
- Add EmulationStation event-driven user scripts
- Add Search feature (with a brand new Arcade-style virtual keyboard!)
- Add Pad-To-Keyboard driver. Play keyboard computer games with your pad!
- Add License menu
- Add support for nVidia proprietary drivers version 390 and 440 (x86/x64)
- Add Arcade virtual system in EmulationStation
- Add automatic detection & management of RetroFlag's NESPi4 case
- Add animation while creating/populating SHARE partition
- Add BIOS check *before* running a game.
- Add Screenshot from the Webmanager for Xu4 and x86/x64.
- Add new Netplay-able systems: Atari2600, PCEngine CD, PC-FX, Family Disc System, TIC-80, Sega CD & Mr.Boom
- Add new systems:
  - Add Nintendo 64DD platform
  - Add OpenBOR (Beat'em up engine)
  - Add Solarus (RPG engine)
  - Add EasyRPG (RPG engine, using libretro-easyrpg, compatible w/ RPG Maker 2000 & 2003)
  - Add naomigd system for your NAOMI GD-ROM games
  - Add Amiga CD32 on x86/x64 (w/ libretro-puae core)
  - Add Naomi on Pi3 (w/ libretro-flycast)
- Add new emulators/cores:
  - Add libretro's pcsx_rearmed on pc too
  - Add libretro's mupen64plus-nx, an improved n64/64dd emulator (rpi only for now)
  - Add libetro's Mesen, accurate NES & FDS emulator (rpi4, xu4, x86/x64 only)
  - Add libetro's Mesen-S, accurate SNES, Satellaview, GB/GBC & Super GameBoy emulator (rpi4, xu4, x86/x64 only)
  - Add libretro mame for recent mame set
  - Add libretro-race, an ngp & ngpc emulator focused on performance
  - Add Libretro-flycast on on rpi3, xu4 and x86
- Add PORTS system:
  - CaveStory moves to ports and is now included, ready to play
  - Add Mr. Boom (8-player Bomberman clone), ready to play
  - Prboom (Former DOOM system) moves to ports
  - Add Quake 1 game engine, ready to play
  - Add 2048 game, ready to play
  - Add Dinothawr game, ready to play
  - Add XRick (Rick Dangerous clone) game, ready to play
  - Add Flashback game. Requires full game files to be added (read the .txt)
  - Add Wolfenstein 3D game, ready to play
  - Add Out Run game. Required arcade rom to be played (read the .txt)
- Add support for popular music formats in EmulationStation
  - MP3 files
  - High quality FLAC files
  - OGG OPUS
  - Amiga Modules (and all derivatives traker formats)
  - Wave (Raw audio format)
  - MIDI file (soundfont file required)
    More information available in share/music/readme.txt
- Add new "networkable" folders in share (overlays, shaders, scripts, screenshots)
- Add *LOTS* of awesome homebrews (more info in related subfolders):
  - A2600: Arguna (Nathan Tolbert)
  - A2600: Halo2600 (Ed Fries, former VP of Microsoft XBox division)
  - A2600: The End 2600 (Rayman_C)
  - A5200: BeefDrop (Ken Siders)
  - A7800: Meteor Shower (Robert DeCrescenzo, exclusively licensed to recalbox)
  - A7800: BonQ (Ken Siders)
  - Amstrad CPC: UWOL 2 (The MojonTwins)
  - Amstrad CPC: The Dawn Of Kernel (Juan J. Martinez)
  - Amstrad CPC: Relentless (Axelay)
  - Amstrad GX4000: Baba's Palace (Rafael Castillo, John McKlain & Dragon)
  - Apple II: Mystery House (Ken & Roberta Williams)
  - Apple II: Lamb Chops (TanRuNomad)
  - Apple II: Retro Fever (TanRuNomad)
  - Atari8bits: Crownland (Piotr Wisniewski)
  - Atari8bits: Ridiculous Reality (Matosimi)
  - Atari8bits: Time Pilot (Solo & Laoo)
  - C64: Wolfling (LazyCow)
  - C64: Transe Sector Ultimate (Richard Richard Bayliss)
  - C64: Vortex Crystals (Richard Richard Bayliss)
  - C64: X-Force (Richard Richard Bayliss)
  - C64: Zap Fight 2 Special Edition (Richard Richard Bayliss)
  - C64: Rescuing Orc (Juan J. Martinez)
  - C64/Plus/4: Adventure in Time (Kisnémeth Róbert)
  - C64/Plus/4: Adventure Park (Varga Gábor)
  - C64/Plus/4: Digiloi (Tero Heikkinen)
  - C64/Plus/4: XPlode-Man (Varga Gábor)
  - Colecovision: MazezaM (PortableDev)
  - Gameboy: Into The Blue (Jonas Fischbach)
  - Gameboy: Retroïd (Jonas Fischbach)
  - Gameboy: Tuff (Ivo Wetzel)
  - Gameboy Advance: Lindsi Luna Blast (PortableDev)
  - Gameboy Color: Wing Warriors (Kitmaker Entertainment & Francisco Javier Del Pino Cuesta)
  - GameGear: Wing Warriors (Kitmaker Entertainment & Francisco Javier Del Pino Cuesta)
  - Intellivision: Princes Lidie (Marco Marrero)
  - Intellivision: Deep zone (Artrag)
  - Intellivision: Hotel Bunny (Sebastian Mihaï)
  - Lynx: Nutmeg (RETROGURU & PHOTON STORM)
  - Lynx: Silas' Adventure (Krzysztof Kluczek)
  - Master System: Wing Warriors (Kitmaker Entertainment & Francisco Javier Del Pino Cuesta)
  - Master System: Astro Force (Enrique Ruiz)
  - Master System: Silver Valley (Enrique Ruiz)
  - Master System: Galactic Revenge (Enrique Ruiz)
  - Master System: Papi Commando in CPP Land (Studio Vetea)
  - Megadrive/Genesis: Old Towers (Retrosouls)
  - Megadrive/Genesis: Misplaced (Retrosouls)
  - Megadrive/Genesis: Yazzie (Retrosouls)
  - Megadrive/Genesis: Gluf (Retrosouls)
  - Megadrive/Genesis: Papi Commando Remix Deluxe (Studio Vetea)
  - Megadrive/Genesis: Bomb on Basic City (Studio Vetea)
  - Megadrive/Genesis: L'Abbaye des Morts (Mun)
  - Megadrive/Genesis: Bug Hunt (Mun)
  - Megadrive/Genesis: Griel's Quest (Mun)
  - MSX: Wing Warriors (Kitmaker Entertainment & Francisco Javier Del Pino Cuesta)
  - MSX: Yazzie (Retrosouls)
  - MSX: XSpelunker (Santi Ontañón Villar)
  - MSX: XRacing (Santi Ontañón Villar)
  - MSX: Night Knight (Juan J. Martinez)
  - MSX2: The Sword of Ianna (RetroWorks)
  - MSX2: Brunilda (Retroworks)
  - NES: Nova The Squirrel (Novasquirrel)
  - NES: Cheril Perils Classic (MojonTwins)
  - Oric/Atmos: Pulsoids (Twilighte)
  - Oric/Atmos: Oricium (Jose Maria Enguita)
  - Pokemini: Cortex (Simon Bradley)
  - Pokemini: Galactix (Lupin, Okkie & p0p)
  - Pokemini: Psychic Seeds (JustBurn, Palkone & Loather)
  - ScummVM: Beneath the Steel Sky (Revolution Software Ltd.)
  - ScummVM: Flight of the Amazon Queen (John Passfield & Steve Stamatiadis)
  - ScummVM: Lure of the Tempress (Revolution Software Ltd.)
  - ScummVM: Soltys (Laboratorium Komputerowe Avalon)
  - SG1000: Cheril Perils Classic (MojonTwins)
  - SNES: UWOL Quest For Money (PortableDev - MojonTwins)
  - Solarus: YarnTown (Max Braz)
  - TIC80: 8 Bit Panda (Bruno Oliveira)
  - TIC80: Shadow Over The Twelve Lands (Bruno Oliveira)
  - TIC80: Cauliflower Power (Librorumque)
  - TIC80: Todor Saved Ludmilla (Andraaspar)
  - TIC80: Secret Agents (msx80)
  - Vectrex: VecZ (LA1N.CH)
  - Vectrex: Thrust (Ville Krumlinde)
  - ZX Spectrum: Cray-5 (RetroWorks)
  - ZX Spectrum: The Sword of Ianna (RetroWorks)
  - ZX Spectrum: Gommy, Medieval Defender (RetroWorks)
  - ZX Spectrum: Genesis, Dawn of the Day (RetroWorks)
  - ZX Spectrum: Brunilda (Retroworks)
  INFO: Some of these games are available on physical supports.
        If you like it, own the appropriate hardware, and wish to support their authors,
        you can buy the physical package, most ofen for a very reasonable price.
  WARNING: Most of these games are still copyrighted by their respective authors.
           Some of them are free to distribute, some others are not.
           You're strongly invited to read carefully all text files and/or pdf in
           games folders. Thank you.

### Improvements
- Bump Buildroot to version 2020.02
- Bump KODI to Leia 18.7.1:
  - 4k x265 available on Pi4, x86/x64
  - Netflix plugin included (need manual activation)
- Bump cores/emulators:
  - Bump DosBox to r4290
  - Bump ScummVM and map D-pad when no analog available
  - Bump ResidualVM
  - Bump Dolphin emulator
  - Bump libretro-cores family
  - Bump oricatmos emulator on lastest version
  - Bump simcoupe emulator
  - Bump Amiberry to v3.1.3.1
  - Bump AdvanceMame to v3.9
  - Bump Theodore core (add emulation of Thomson TO7 and TO7/70 computers)
- Bump retroarch to v1.9.0
- Bump retroarch-cheats to v1.9.0
- Bump libretro-assets on last version
- Bump Odroid UBOOT to 2017/05 version
- Move x86/x64 graphic backend from GLES to OpenGL. Improve overall quality.
- Improved NetPlay:
  - Removed filtering of non-Recalbox players in netplay game lists
  - Add spectator mode management
  - Add password-protected game management (both player & spectator)
  - Add 15 pre-configured & editable passwords for quick selection
  - Available netplay games sorted by state/name
  - Add Recalbox icon in front of game names
  - Add Password icon for password-protected games
  - Add automatic core switch on client side to match host selected core
- Improve sound management:
  - Simplified output device selection
  - Volume control is working for boot-videos
- Rename Fba_libretro to Fbneo (updated to the new official logo)
- Rename 4do to Opera (newer libretro 3do core)
- Manage GameCube Bios
- Improve internal Scraper (ScreenScraper):
  - Faster! Use your ScreenScraper's threads to parallelize workloads
  - Better! Lots of options, snaps video, and more...
  - Stronger! Improved reliability and strongness
- Improve overall Emulationstation's stability & reliability
- Optimize memory requirements of EmulationStation
- Optimize EmulationStation boot time (up to 20 times faster!)
- Improve pad processing in EmulationStation. Perform auto-mapping of 70% of commonly available pads (incluxing all 8BitDo)
- Improved Genre & Region processing in EmulationStation *
- Improved translations:
  - Fix some bad translations in FR, ES, PT, IT & DE texts
  - Fix missing translations in FR, ES, PT, IT & DE texts
- Faster-than-light CRC calculation for netplay
- Game sorting and Jump-to-letter are now unicode compatible
- Simplify emulator/core selection UI
- Set vice_x64sc as default c64 emulator + JiffyDOS support
- Improve the management of external screens and selection from recalbox.conf (by Chriskt78)
- Improve Apple IIGS slot detection & auto-boot
- Improve the following standalone emulators:
  - AdvanceMame: Support roms in sub-folders, integer scale & show FPS
  - Amiberry (Amiga): Center screen, show FPS (LED bar) & Support scanline shader
  - DosBox (DOS): Support retro & scanline shaders
  - GSPlus (Apple IIGS): Support scanline shader
  - Linapple (Apple II): Support all screen resolutions
  - Oricutron (Oric): Support scanline shader
  - Simcoupé (SAM Coupé): Support scanline shader & smooth rendering
  - ScummVM: Support scanline shader & smooth rendering
- Auto discover and configure multi-disk games for:
  - Amiberry (Amiga): up to 4 disks loaded at once
  - GSPlus (Apple IIGS): from 2 up to 32 disks loaded at once, depending on floppy types
  - Quasi88 (PC88): up to 6 disks loaded at once
- Improve GPI support (boot image, installation phases, ...)
- Improve shader management in EmulationStation (Add raw shader selection)
- Move shaders into user's SHARE partition
- Move Libretro's cheat into SHARE partition
- Improved WIFI & Bluetooth management

### Fixes
- Fix Odroid XU4 fan issue
- Fix Odroid XU4 sound issue
- Fix EMMC boot on XU4
- Fix x86/x64 Boot videos
- Fix music popup crashes
- Fix music loop play
- Fix netplay popup crashes
- Fix Apple2 not working on x86/x64
- Fix fullscreeen on Oric/Atmos and Apple2 on x86/x64
- Fix duplicate folders in Arcade systems
- Fix quit menu not being accessible when boot-on-gamelist is on
- Fix some bad behaviors in favorite management
- Fix reboot Emulationstation with webmanager
- Fix volume issues in boot video
- Fix REICAST bug on XU4
- Fix FPS show/hide in Retroarch
- Fix multiple audio issues
- Fix Gamelist reset when editing Metadata
- Fix multiple crashes in EmulationStation

*  : Require to scrape missing data using the internal scraper
** : Availailable in both EmulationStation menu and configuration file


## Version 6.1.1

### News

### Improvements
- Add zfast shaders, fast CRT shaders for all platforms
- Odroid C2 removed from supported boards
- Improved button filtering while mapping pads (especially PS2/PS3 pads)
- Make apple II mapping more consistent: Use button A/B instead of L/R
- Bump kernel firmwares to latest version
- Add new Game Boy shader
- New wifi management
- Improve Blutooth association
- Bump FreeIntV to include latest fixes
- Bump FBNeo to include latest fixes
- Bump Retroarch to 1.8.1  to include latest fixes & optimizations
- Bump Doplhin-emu on 5.0-11288 add automatic configuration and netplay compatibility

### Fixes
- Fix Reicast on XU4 and PC platforms and fix wrong flycast entry on Rpi
- Fix .cpr and .bin extensions for the Amstrad GX4000 system
- Fix libretro-uae bios list
- Fix GPIO driver not loading (Arcade, GameCon & DB9)
- Fix random crashed while remapping pads
- Fix AppleIIGS not running with some pads
- Fix AppleIIGS multidisk game configuration
- Fix PiSNES fullscreen/integer-scale
- Fix demo screen ratio on non-16:9 screens
- Fix Arcade system not available in GUI
- Fix demo mode on PC (first game launched no longer runs indefinitely)
- Fix emulationstation reporting wrong free space in System menu
- Fix demo mode exit after the user pressed start
- Fix missing apple IIGS bios information in Manager and bios/readme.txt
- Fix diacritic characters in uppercase texts
- Fix atari800 core crashes
- Fix enable cheevos badge in retroarch menu (retroachievement.org)
- Fix advancemame availability on xu4 and x86 (not x64)
- Fix some retroarch core random crashes and/or lags/slowness
- Fix videosnaps crashs/artifects while scrolling
- Fix missing videosnaps keys in recalbox.conf
- Fix videosnaps crashes & white textures on x64 (Thanks to cpasjuste)


## Version 6.1

### News
- Pi 3A+ compatibility (Firmware bump)
- New system! Uzebox on all systems (RetroArch core: libretro-uzem)
- New system! Amstrad GX4000 added with libretro's cap32 emulator
- New system! Apple IIGS on all systems (Standalone: GSplus 0.14)
- New system! Spectravideo added with libretro's bluemsx emulator
- New system! Sharp X1 added with libretro's xmil emulator
- New system! Palm added with libretro's mu emulator
- New core! Added Gearsystem libretro core. An optimized GG/SMS/SG core working better on some games.
- New core! Added SameBoy libretro core. Game link support for GB/GBC
- New video snaps! Now see short videos of games before launching them
- GPi Case Plug & Play: Autodetect and install drivers & appropriate themes and settings
- New system! PC-88 added with libretro's quasi88 emulator
- New system! TIC-80 added with libretro's tic80 emulator
- New system! MSXturboR as own system. Generic msx system has been removed.
- New system! Multivision added with libretro's gearsystem emulator
- New system! Atomiswave added with libretro's flycast emulator (pc only)
- New system! NAOMI added with libretro's flycast emulator (pc only)
- New core! Added flycast libretro core. Dreamcast (et al.) emulator
- New system! Saturn added with libretro's bettle-saturn, yabause and yabasanshiro emulators (pc only)
- New configuration override system to fine tune all configuration per system, per folder or per game
- Easy AI Service configuration (Retroarch translation service)
- Add Arcade meta-system to group piFBA, FBN, MAME and Neogeo into a single system
- New core! Added UAE libretro core. (Experimental) Amiga emulator on all platforms
- New documentation available on gitbook: http://recalbox.gitbook.io (still WIP)

### Improvements
- Retroarch updated to version 1.7.8v3!
- Retroarch cheats updated to version 1.7.8!
- Switched default Retroarch UI to Ozone (except on GPI)
- Improved RGUI configuration on GPI
- Libretro core updated! FBA Libretro core updated to latest version (Neogeo CDRom Speed fix)
- Libretro core updated! Picodrive updated to latest version (Fix shifted down screen)
- Libretro core updated! Theodore updated to latest version (add emulation of Thomson MO6 and Olivetti Prodest PC128)
- Libretro core updated! Migrate Glupen64 to Mupen64Plus
- Libretro core split! Rebrand stella to stella2014 and add upstream stella core
- Emulator updated! ScummVM updated to September 2019 version (new theme included)
- Emulator updated! ResidualVM updated to September 2019 Version
- Emulator updated! Linapple-Pie now uses upstream repository
- Emulator updated! Oricutron updated
- Enable both hotkeys and I2C on RasberryPi GPIO
- Add .7z support to more emulators
- MoonLight updated! Add support for latest GForceExperience softwares
- New GUI Menu: In gamelist views, START opens game contextual menu. SELECT switch between all games/favorites only
- Add game information screen in demo mode
- Mame changes its default emulator to mame2003_plus
- Add option to confirm before exiting a game (libretro cores only)
- Shutdown System option moved at the top of the Quit menu
- Retroarch ratio resynchronized:
  - "Core provided" and "Retroarch Custom" added to ratio menu
  - Old "custom" ratio renamed "Do not set" (let the user choose in Retroarch)
  - Ratio text localized
- New theme folder in share root folder (old folders are still working)
- New pads added to known controllers: Moga Pro Power, Wiimote, Logitech RumblePad
- Bluetooth pads configuration easier than ever: previously configured pads automatically unpaired before pairing again
- Enable "Threaded DSP" option in 4DO by default (improve emulation speed)
- Odroid XU4 linux kernel upgraded to version 4.14
- pcsx_rearmed supports .chd files now too
- Improve volume balancing between RetroArch and Recalbox
- Add .zip and .7z extensions for N64 (only work with libretro-mupen64plus)
- RetroArch Disc Project: Groundwork for playing games directly from your CDROM
- Almost all core/emulators has been bumped again. Too much for the detailed list!
- ScreenScraper internal scraper is back with new options
  - Choose your image: Screenshot, Title, Front Case, 3D Case, Mix V1 and Mix V2
  - Use your ScreenScraper account to get higher per day limitations
  - Choose your favorite region/language to get appropriate images and texts
- TheGameDB internal scraper is back using newest APIs
- Commodore 64 has now two emulators: x64 (speed) and x64sc (accuracy)

### Fixes
- Fix boot intro videos on x86 and x64
- Fix system unique key (fullname + platform)
- Fix demo mode using retroachievements
- Fix demo mode using autoload/autosave
- Fix non working pads in Dolphins
- Fix ScummVM not starting if no pad is configured
- Fix x86/x64 stuck in demo mode in some circumstances
- Fix support archive upload error
- Fix intro video fullscreen on x86/x64
- Fix steam controller driver
- Fix Amiga multi-disk bug with [] characters
- Fix Amiga WHDL loader
- Fix default scraper

## Version 6.0 - DragonBlaze

### News
- Raspberry Pi3b+ & CM3 compatibility
- Updated emulator names! catsfc -> snes9x2005, pocketsnes -> snes9x2002, snes9x_next -> snes9x2010, pce -> mednafen_pce_fast, vb -> mednafen_vb, imame -> mame2000, mame078 -> mame2003, fba -> fbalpha
- New emulator! Pokemon Mini on all systems (RetroArch core: libretro-pokemini)
- New emulator! Mattel Intellivision on all systems (RetroArch core: libretro-freeintv)
- New emulator! Fairchild Channel-F on all systems (RetroArch core: libretro-freechanf)
- New emulator! Atari Jaguar on x86/64 (RetroArch core: libretro-virtualjaguar)
- New emulator! MGT Sam Coupé on all systems (Standalone simcoupe)
- New emulator! Tangerine Oric/Atmos and later clones on all raspberry (Standalone oricutron)
- New emulator! Atari 8bits series on all systems (800, XL, XE) and Atari 5200 console (RetroArch core: libretro-atari800) - Thanks to Dubbows!
- New emulator! PC-FX emulator on x86 and XU4 systems (Retroarch core: libretro-beetle-pcfx)
- New emulator! PC-98xx series on all systems but odroidC2 (Retroarch core: libretro-np2kai)
- New System! SNES extensions Satellaview and SuFami Turbo added as own systems
- New System! NeoGeo CD with audio track support (via RetroArch core libretro-fba)
- New System! Amiga CD32 with audio track support (via Amiberry)
- New Core! Added mame2003-plus libretro core. A MAME078 version with added games support plus many fixes and improvements. DAT files available in rom folder
- New free game! "Mission: Liftoff" for Thomson emulator.
- New free game! "Flower" for Amiga emulator.
- New Capcom RB intro video
- New PCEngine intro video
- Add support for Mayflash GameCube adapter
- Add 7z file support for many libretro cores
- Add overlay possibility for advmame core
- Add new Screensaver "DEMO" to play continuously random games (Press START/ENTER to play the current game)

### Improvements
- KODI updated to 17.6
- Youtube plugin for KODI updated to 6.3.1
- Joypads management updated, mostly impacting joypads advertising as a complete keyboard.
- Preconfigured gamepads file rewritten
- x86: Now requires a CPU that can handle at least SSE and SS2 (Core2duo and higher)
- x86: Add support for newer AMD GPU
- Retroarch updated to version 1.7.6!
- Retroarch cheats updated to version 1.7.6!
- Emulator updated! 4DO updated to lastest revision. Also available on Rpi3 (3B+ recommended).
- Emulator updated! Amiberry (Amiga) - Huge up! Add supports of IPF, RP9, CD, zip, 7z & Retroarch pad configuration - Available also on Odroid XU4.
- Emulator updated! DoxBox updated to December 2018 version.
- Emulator updated! ScummVM and ResidualVM updated to January 2019 versions.
- Libretro core updated! 81 (EX-81) updated to latest version.
- Libretro core updated! Atari800 (Atari 5200Atari 8bits) updated to latest version.
- Libretro core updated! Beetle Lynx (Atari Lynx) updated to latest version. Support Headerless roms.
- Libretro core updated! Beetle NGP (Neo Geo Pocket/Color) updated to latest version.
- Libretro core updated! Beetle PCE (PCEngine-CD) updated to latest version.
- Libretro core updated! Beetle PSX (Playstation 1) updated to latest version.
- Libretro core updated! Beetle PSX-HW (Playstation 1) updated to latest version.
- Libretro core updated! Beetle SGX (PCE/PCE-CD/SGX) updated to latest version.
- Libretro core updated! Beetle VB (Virtual Boy) updated to latest version.
- Libretro core updated! Beetle Wonderswan (Wonderswan/Color) updated to latest version.
- Libretro core updated! BlueMSX (MSX) updated to latest version.
- Libretro core updated! Caprice32 (Amstrad) updated to latest version.
- Libretro core updated! Crocods (Amstrad) updated to latest version.
- Libretro core updated! DeSmuME (NDS) updated to latest version.
- Libretro core updated! FBA Libretro core updated to latest version. Recommended Romset: 0.2.97.44
- Libretro core updated! FCEUmm (Nintendo NES) updated to latest version.
- Libretro core updated! fMSX (MSX) updated to latest version.
- Libretro core updated! FreeIntV (Intellivision) updated to latest version.
- Libretro core updated! Fuse (ZX Spectrum) updated to latest version.
- Libretro core updated! Gambatte (Nintendo GB/Color) updated to latest version.
- Libretro core updated! Genesis Plus GX (Sega MD/GG/MS/CD) updated to latest version.
- Libretro core updated! GNupeN64 (Nintendo64) updated to latest version.
- Libretro core updated! GPSP (Nintendo GBA) updated to latest version.
- Libretro core updated! GW (Game & Watch) updated to latest version.
- Libretro core updated! Handy (Atari Lynx) updated to latest version.
- Libretro core updated! Hatari (Atari ST) updated to latest version.
- Libretro core updated! iMAME (Mame 2000) updated to latest version.
- Libretro core updated! Lutro (Lua Game Engine) updated to latest version.
- Libretro core updated! MAME2003 (Mame 2003) updated to latest version.
- Libretro core updated! MAME2010 (Mame 2010) updated to latest version.
- Libretro core updated! MelonDS (NDS) updated to latest version.
- Libretro core updated! mGBA (Nintendo GBA) updated to latest version.
- Libretro core updated! Nestopia (Nintendo NES) updated to latest version.
- Libretro core updated! NxEngine (Cave Story) updated to latest version.
- Libretro core updated! O2EM (Odyssey²) updated to latest version.
- Libretro core updated! PCSX (Playstation 1) updated to latest version.
- Libretro core updated! Picodrive (Sega MS/MD/CD/32X) updated to latest version.
- Libretro core updated! PocketSnes (Nintendo SNES) updated to latest version.
- Libretro core updated! PrBoom (Doom engine) updated to latest version.
- Libretro core updated! ProSystem (Atari 7800) updated to latest version.
- Libretro core updated! Px68k (Sharp X68000) updated to latest version.
- Libretro core updated! QuickNES (Nintendo NES) updated to latest version.
- Libretro core updated! Snes9x (Nintendo SNES) updated to latest version.
- Libretro core updated! Snes9x2005 (Nintendo SNES, formerly CatSFC) updated to latest version.
- Libretro core updated! Snes9x2010 (Nintendo SNES, formerly Snes9x-Next) updated to latest version.
- Libretro core updated! Stella (Atari 2600) updated to latest version.
- Libretro core updated! TGBDual (Nintendo GB/Color) updated to latest version.
- Libretro core updated! Theodore (Thomson) - Supports now TO8, TO8D, TO9, TO9+ & MO5 machines
- Libretro core updated! VecX (Vectrex) updated to latest version.
- Libretro core updated! Vice (Commodores 8bits) updated to latest version.
- GPIO arcade driver upgraded: now support I2C and GPIO reconfiguring from the command line
- EmulationStation: Now automatically reboots once the upgrade is ready
- "overlays" folder and roms subfolders automatically created
- Updated all MAME hiscore/cheat files in bios folder - Thanks to olivierdroid92!
- Static IP configuration available for WIFI connections (in recalbox.conf)
- Add support for AZERTY/QWERTY virtual keyboards
- ScummVM/ResidualVM now use Recalbox pad configurations

### Fixes
- bluetooth detection on Odroid XU4
- bluetooth recovery on Odroid XU4
- EmulationStation: better cyrillic display
- EmulationStation: fix scraped folders display
- MSX Emulator: Fixed Bluemsx core
- PS3/PS4 pads are working now
- SSH is now working when moving the share on Fat32/exFat partitions
- Recalbox sends 0000 to BT devices asking for pincode
- WIFI settings saved in a more secure way

---

## Version 18.07.13 - 2018-07-13

### Improvements
- Bump desmume to desmume2015
- EmulationStation: Arcade roms fullname in Netplay GUID
- EmulationStation: No game launch if core doesn't match

### Fixes
- SELECT as hotkey sometimes messy with arcade
- EmulationStation: remove netplay popup to prevent some crash
- Wifi: options were not saved with nfs cifs or configurations
- N64 rice & GlideN64: fix blank screen
