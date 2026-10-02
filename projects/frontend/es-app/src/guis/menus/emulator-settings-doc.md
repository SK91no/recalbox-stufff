# Introduction

Un petit thread ici pour expliquer le fonctionnement des nouveaux "settings emulators" directement accessibles dans ES.

Le but de ces settings, c'est de proposer aux utilisateurs de pouvoir agir sur les options impactantes des émulateurs. Pas question ici de mettre la totalité des options d'un core Retroarch dans ES.
En revanche, permettre de changer la langue sur Dolphin par exemple, ou permettre de changer la machine émulée sur l'émulateur Atari 16/32 bits apporteront un vrai plus et permettront à ceux qui ne sont pas a l'aise avec Retroarch ou les options dans les standalones, de pouvoir le faire en toute simplicité.
Ça fonctionne sur quasiment tous les émulateurs Libretro ou standalone. Seuls les quelques rares émulateurs avec des fichiers de configuration au format batard, ne pourront être adressé pour l'instant.
Ces settings permettent d'attaquer directement les fichiers de configuration des émulateurs. Il faut donc prendre garde que les options que vous allez proposer ne soient pas prises en charge par le configgen, auquel cas, ce que l'utilisateur modifiera sera écrasé par le configgen.

Ces settings sont dit "déclaratifs", c'est-à-dire qu'ils ne sont pas hardcodé dans ES. Ils sont déclarés et définis dans un XML puis interprétés par ES.

***Tout le monde*** peut en ajouter (enfin dans les betas du moins), et je compte sur vous pour nous aider ;)

# Fonctionnement interne et edition

Ces settings sont déclarés dans le XML des menus, dans un menu spécial. Pour ES, ce sont des entrées menu standard sur lesquelles il doit exécuter des operations spéciale lorsque le menu est créé et lorsqu'on change une entrée.

Le XML de déclaration des menus est embarqué dans ES et ne peut donc être modifié, mais... on a prévu des raccourcis claviers (il vous en faudra donc un) pour générer un fichier réel qui sera modifiable et qui sera chargé en priorité par ES:
- **ALT+F8** : Modifie le fichier XML à l'emplacement `/recalbox/share/system/.emulationstation/menu.xml` (ou `\\RECALBOX\system\.emulationstation\menu.xml`). Si le fichier existe déjà, il vous demandera confirmation pour l'écraser, donc pas de risque de perdre vos settings par inadvertance.
- **ALT+F7** : Recharge les menus. Il ne doit y avoir aucun menu ou fenêtre (bios/Scrape, ...) à l'écran. Si c'est le cas, il vous sera demandé de tout fermer avant de recommencer.

Ces raccourcis ne fonctionnent **que sur les alphas**.

Le fichier menu.xml est ultra-contrôlé par ES et toute malformation se soldera par un log + arrêt immédiat d'ES. Il est donc conseillé de le lancer:
- Sur PC : `killall -9 emulationstation` puis `emulationstation --debug`. Vous pouvez utiliser CTRL+C pour le stopper asi besoin.
- Sur Pi : `killall -9 emulationstation` ou `es stop`, puis pareil que pour PC.

En le lançant en console et en debug, vous aurez les logs de rechargement de menus, les erreurs et les warnings s'il y en a.

***Disponible à partir de la 10 alpha 18 !***

# Edition et syntaxe

La zone qui nous intéresse se situe en début de fichier :
```xml
	<!--
		EMULATOR SPECIFIC SETTINGS
	-->
	<menu id="EmulatorSpecificSettings">
		<menu id="auto" caption="Libretro HatariB Settings" include="true">
			<setting caption="Machine Type" core="libretro:hatari" file="$configs/retroarch/cores/retroarch-core-options.cfg" fileType="ini" key="hatari_machine" type="list" props="d" values="0:ST|1:Mega ST|2:STE|3:Mega STE|4:TT|5:Falcon" default="0" help="Choose the machine model to emulate. STE is a good choice for most games." />
			<setting caption="Memory Size" core="libretro:hatari" file="$configs/retroarch/cores/retroarch-core-options.cfg" fileType="ini" key="hatari_memory" type="list" props="d" values="256:256 KB|512:512 KB|1024:1 MB|2048:2 MB|2560:2.5 MB|4096:4 MB|8192:8 MB|10240:10 MB|14336:14 MB" default="1024" help="Select the amount of memory. 1 MB is enough for gaming." />
			<setting caption="Fast Floppy" core="libretro:hatari" file="$configs/retroarch/cores/retroarch-core-options.cfg" fileType="ini" key="hatari_fast_floppy" type="nbool" props="d" default="0" help="Set ON to accelerate the floppy disc emulation. May cause some games to fail !" />
		</menu>
	</menu>
```

## Modifier des options ou en rajouter

Si vous voulez rajouter une option ou modifier une option existante, repérez le menu qui vous intéresse grace au nom de l'émulateur dans le `caption`
Si vous voulez rajouter un set d'options inexistantes pour un core/émulateur, commencez par ajouter une entrée menu comma ça :
```xml
	<!--
		EMULATOR SPECIFIC SETTINGS
	-->
	<menu id="EmulatorSpecificSettings">
	  ...
		<menu id="auto" caption="Émulateur PS12 Standalone" include="true">
		</menu>
	</menu>
```
L'id est toujours `auto` et la propriété `include` est nécessaire pour que le menu puisse s'inclure au menu avancé d'un émulateur, sinon ça va générer un sous-menu disgracieux. Le nom dans `caption` sera utilisé pour créer un header de séparation

Ensuite, on ajoute une ou plusieurs entrées `<setting ... />`, une entrée par option.

## Ajouter des options

Pour ajouter une option, on va ajouter une entrée `<setting ... />` avec une série de propriétés. Plusieurs propriétés sont obligatoires, d'autre facultatives ou dépendantes d'autres propriétés.

```xml
	<!--
		EMULATOR SPECIFIC SETTINGS
	-->
	<menu id="EmulatorSpecificSettings">
	  ...
		<menu id="auto" caption="Émulateur PS12 Standalone" include="true">
            <setting caption="To infinite and beyond!" core="ps12" file="$config/ps12/ps12.ini" fileType="sini" key="youredreaming.rts" type="bool" default="0" help="Activate RTX on all GPU!" />
            <setting caption="PS13 compatibility" core="ps12" file="$config/ps12/ps12.ini" fileType="sini" key="youredreaming.ps5" type="bool" default="0" help="Activate compatibility with PS13 and even PS14 games" />
            <setting caption="Internal HDD Capacity in TB" core="ps12" file="$config/ps12/ps12.ini" fileType="sini" key="youredreaming.hdd" type="range" values="1:128:1" default="0" help="Define the emulated internal HDD capacity in TeraByte" />
            <setting caption="Scanlines" core="ps12" file="$config/ps12/ps12.ini" fileType="sini" key="youredreaming.scanlines" type="list" values="0:What? Scanlines in HDMI 16K ?!|1:Soft scanlines|2:Heavy scanlines" default="2" help="Run PS12 like in good ol'times :D" />
		</menu>
	</menu>
```

### Propriétés partagées avec les menus

- `caption` : [obligatoire] c'est le texte de l'entrée qui va apparaitre dans le menu. Ce doit être court, concis et en anglais (il pourra être traduit comme tous les autres textes des menus).
- `help` : [facultatif] texte d'aide quand l'utilisateur fera Y sur l'entrée menu. Facultatif, mais vivement conseillé, surtout s'il y a des remarques sur son usage !

### Propriété de définition de l'option

- `core` : [obligatoire] Indique quel emulator ou quel core on cible. Cette propriété permet à ES de savoir quoi afficher et dans quel système. Si vous ciblez un standalone, indiquez seulement le nom court du standalone (si vous avez un doute, jetez un oeil au fichier systemlist.xml pour avoir les noms courts). Si vous ciblez un core libretro ou un core comme il en existe pour la N64, la syntaxe est `emulateur:core` (exemple: `libretro:puae`). En cas de doute, direction le fichier systemlist.xml ! Si vous vous trompez, votre option ne s'affichera pas dans le système :)
- `file` : [obligatoire] Spécifie le fichier de configuration à modifier. Certains raccourcis sont disponibles et expliqués dans une section ci-après
- `fileType` : [obligatoire] Spécifie le type de fichier de configuration. Voir ci-après
- `key` : [obligatoire] Cible la clef qu'on doit aller modifier. Si c'est une clef simple (clef de fichier ini ou fichier sini hors section), c'est le nom de la clef. Si c'est une clef dans une section, la syntaxe est: `section/clef`.
- `type` : [obligatoire] Indique le type de valeurs de la clef. Details dans une section ci-après
- `props` : [facultatif] Propriété permettant d'indiquer comment la valeur doit être écrite dans le fichier.
- `values` : Obligatoire pour les listes et les ranges, inutile pour les valeurs binaires. Voir ci-après la syntaxe de cette clef.
- `default` : Valeur par default à afficher dans le menu si la clef ou le fichier ne sont pas encore présents.

### Raccourcis utilisables dans la propriété `file`

- `$home` équivaut à `/recalbox/share/system`, certains standalone mettent leur fichier de config ici
- `$bios` équivaut à `/recalbox/share/bios` au cas où certains émulateurs iraient encore mettre des choses dans les bios :)
- `$saves` équivaut à `/recalbox/share/saves` pareil, au cas où des émulateurs iraient écrire des settings dans les saves
- `$configs` équivaut à `/recalbox/share/system/configs`, c'est là où on trouvera beaucoup de fichier de config
- `$.config` équivaut à `/recalbox/share/system/.config`, idem

### Detail du type de fichier

Pour l'instant, nous ne gérons que 2 types de fichiers, parmi :
- `ini` : fichier de type clef=valeur simple
- `sini` : fichier ini avec sections entre []

Ces deux types devraient pouvoir gérer la très grande majorité des fichiers de config, mais si vous en trouvez des non compatibles... on les rajoutera :)

### Detail des valeurs de la propriété `type`

Les type de valeurs sont à choisir parmi :
- `list` : un choix parmi une liste de valeurs
- `nbool` : Choix binaire, dont les valeurs sont obligatoirement 0 ou 1.
- `ybool` : Choix binaire dont les valeurs sont `yes` et `no`.
- `tbool` : Choix binaire dont les valeurs sont obligatoirement `true` et `false`.
- `cbool` : Choix binaire dont les valeurs sont définies par la propriété `values`
- `range` : Valeur entière variant entre un minimum et un maximum avec un pas de 1 ou plus.

Ces différents types devraient permettre de gérer tous les cas de figure, mais il n'est pas exclus que nous devions en rajouter au fil du temps

### Détail de le propriété `props`

Les propriétés sont une série de caractères, determinant la façon dont la valeur va être lue et surtout écrite.
`props` peut donc contenir un ou plusieurs caractères, parmi :
- `q` : Simple quote. La valeur est écrite entre ''
- `d` : Double quote. La valeur est écrite entre ""
- `u` : Uppercase. La valeur est convertie en majuscule avant écriture. Intéressant pour les booléens dont les valeurs sont `TRUE` et `FALSE` par exemple
- `l` : Lowercase. La valeur est convertie en minuscule avant écriture.

D'autres pourront venir se rajouter au fil du temps

### Détail des possibilités de la propriété `values`

Dans le cas où on veut un setting de type liste, la propriété value va permettre de détailler les valeurs réelles de la liste ainsi que les valeurs affichées.
Par exemple, dans les settings plus haut, pour le core HatariB, nous avons `values="0:ST|1:Mega ST|2:STE|3:Mega STE|4:TT|5:Falcon"` qui permet de définir les 6 machines émulables par le core.
Les entrées de la liste sont séparées par des `|`. Chaque entrée est constituée de `valeur réelle:valeur affichées`. Ainsi dans notre exemple, dans le fichier nous trouverons des valeurs chiffrées de `0` à `5`, mais l'utilisateur ne verra que les noms des machines dans le menu.
Il existe des cas pour lesquels les valeurs affichables sont les mêmes que les valeurs réelles, auxquels cas, vous pouvez omettre les valeurs affichables et les `:`

Pour les ranges, nous utilisons une autre syntaxe : `min:max:step`. Ca va créer un slider qui va permettre de régler une valeur numérique comprise entre le `min` et le `max` inclus, et qui progressera de `step`.
Si le `step` ne tombe pas juste sur le max, l'utilisateur ne pourra monter que jusqu'à la dernière valeur possible avant le max. Par exemple `1:10:2` ne permettra de régler que les valeurs 1, 3, 5, 7 et 9.
Le `step` n'est pas non plus obligatoire. S'il n'est pas précisé, sa valeur par défaut est 1.

### Troubleshooting

Q: Que faire si mon setting n'apparait pas ?

A: Commencer par verifier le contenu de `core`. Si c'est bon, vérifiez les logs, vous avez certainement commis une erreur et le setting est invalide.

Q: L'émulateur qui m'intéresse a un type de fichier qui n'est ni un `ini`, ni un `sini`, que faire ?

A: Demander a BK si c'est gérable et si on peut ajouter ce type de fichier

Q: Je n'arrive pas à ...

A: Demander à BK ... :D