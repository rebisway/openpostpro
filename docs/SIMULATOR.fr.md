# Référence du simulateur et des traducteurs

*English version: [SIMULATOR.en.md](SIMULATOR.en.md).*

L'onglet **Simulation** joue un programme de machine sur un bloc de matière, en 3D, avant de l'envoyer à une machine. Cette référence s'adresse à qui utilise l'onglet et à qui écrit un **traducteur**, le module Python qui transforme un programme (du G-code, ou un autre format) en quelques commandes simples que le simulateur joue. L'implémentation de référence est `simulator/gcode.py` : copiez-la pour démarrer. Les scripts CAO sont dans [CAD-SCRIPTS.fr.md](CAD-SCRIPTS.fr.md), les post-processeurs dans [POSTPROCESSOR.fr.md](POSTPROCESSOR.fr.md).

Contenu : installation d'un traducteur, l'onglet Simulation, le déroulement d'une traduction, les trois méthodes de la classe `Translator`, toutes les commandes du simulateur, les erreurs et avertissements, le traducteur de G-code générique et un exemple minimal complet.

## 0. Conventions

### Fichiers et chargement

| Élément | Règle |
|---|---|
| Emplacement | le dossier `simulator` à côté de l'exécutable |
| Chargement | au démarrage, chaque fichier `.py` est copié dans `~/.openpostpro/simulator` et utilisé à partir de là |
| Nom | le vôtre (`mon_controleur.py`), placé dans `~/.openpostpro/simulator`. La copie du démarrage **écrase les fichiers de même nom** : ne modifiez jamais un fichier livré, copiez-le sous un autre nom |
| Classe | le fichier définit une classe nommée `Translator` avec une méthode `next`. Un fichier `.py` qui n'en a pas est une bibliothèque utilisée par d'autres fichiers : il n'est pas listé |
| Choix | la liste de l'onglet Simulation montre *Auto* puis les traducteurs du dossier. *Auto* (par défaut) choisit pour chaque fichier le traducteur qui en lit l'extension (méthode `extensions()`, plus bas), et `gcode` si aucun ne la déclare. Choisir un traducteur dans la liste le **force** pour tous les fichiers. Le choix est conservé avec les réglages ; en changer traduit de nouveau le programme |
| Dossier | le menu **Simulation / Ouvrir le dossier des traducteurs** ouvre `~/.openpostpro/simulator` |
| Rechargement | le module est relu à chaque chargement d'un programme : modifiez le fichier, enregistrez-le, chargez de nouveau le programme, sans redémarrer |
| Erreurs | une exception du traducteur est écrite dans le journal avec sa trace Python et la ligne du programme ; la traduction continue à la ligne suivante |
| Python | 3.12 ou plus récent, installé avec « Add Python to PATH » (voir le README) |

### Qui appelle qui

```
OpenPostPro (C++)  ->  Translator (votre fichier)  ->  commandes (dictionnaires)  ->  lecteur et matière (C++)
```

L'application lit le texte du programme et donne **une ligne à la fois** à votre `next()`. Votre classe retourne les commandes de cette ligne, et garde elle-même son état (position, avance, unités, modes) : le simulateur ne garde aucune mémoire du langage du programme.

**Le traducteur ne fait que traduire.** La lecture, la pause, le pas à pas, le retour en arrière, l'aller à une ligne, la vitesse et le résultat final appartiennent au simulateur : ses boutons agissent quoi que dise le programme, et un arrêt ou une temporisation du programme est une commande comme les autres (section 4).

### Types

- Une commande est un dictionnaire Python avec une clé `'cmd'` ; les nombres sont des `float` ou des `int`, les indicateurs des `bool`, les noms des `str`.
- Les coordonnées sont **absolues, en millimètres, et complètes** (`x`, `y` et `z` dans chaque déplacement) : le traducteur résout les distances relatives, les pouces, les décalages d'origine et les cycles fixes ; le simulateur n'a jamais besoin de connaître un mode.
- La ligne donnée à `next()` est en UTF-8 et n'a pas de fin de ligne. Un programme en Windows-1252 ou en Latin-1 (des accents dans les commentaires, par exemple) est converti par l'application : toute ligne qui n'est pas de l'UTF-8 valide est prise pour l'un de ces codages sur 8 bits.

## 1. L'onglet Simulation

**Charger un programme.** *Programme actuel* charge le fichier de sortie du document (lancez d'abord le post-processeur, ou utilisez **Post-processeur / Lancer et simuler**). *Ouvrir...* charge un fichier de G-code quelconque, qu'on peut aussi déposer sur la fenêtre. Le traducteur se choisit dans la liste à côté des boutons.

**Lecteur.** Tous ces boutons agissent à tout moment, quoi que fasse le programme :

| Bouton | Effet |
|---|---|
| Lecture / Pause | joue à la vitesse choisie, ou arrête (la barre d'espace fait de même) |
| Début | retour au début, la matière est de nouveau entière |
| Pas | joue jusqu'à la fin du déplacement suivant, puis met en pause |
| Ligne | joue les commandes de la ligne suivante du programme, puis met en pause |
| Résultat | va directement à la fin du programme, en calculant par tranches pour que la fenêtre reste vivante |
| Vitesse | x1 est la vitesse réelle de la machine (d'après les avances) ; x2 à x500 vont plus vite. *Pleine vitesse* joue aussi vite que possible |
| Un clic sur une ligne du programme | va à cette ligne (le programme est joué depuis le début jusqu'à elle, rapidement) et met en pause |

Les commandes du programme sont exécutées comme le fait une machine quand la vitesse est un multiple : une **temporisation** (`G4`) attend son temps, un **arrêt** (`M0`, et `M1` si *Arrêt facultatif* est coché) met le lecteur en pause jusqu'à ce qu'on appuie sur Lecture, `M2` et `M30` terminent le programme. Quand le simulateur va vite (*Pleine vitesse*, *Résultat*, un clic sur une ligne, *Pas*), les temporisations et les arrêts sont ignorés. Le temps affiché est toujours le temps de la machine, calculé d'après les avances, et ne dépend pas de la vitesse du lecteur. Les déplacements rapides sont calculés à 5000 mm/min.

**La scène.** Le bouton gauche tourne la vue, le bouton droit la déplace, la molette zoome. *Dessus*, *Face* et *Iso* sont des vues standard. Les cases montrent ou cachent la matière, les déplacements d'usinage (orange), les déplacements rapides (bleu), l'outil et les axes (l'origine du programme). Le panneau de droite est le programme, avec la ligne jouée en couleur (rouge pour une ligne en erreur, jaune pour un avertissement).

**Réglages** (case *Réglages*) :

| Réglage | Sens |
|---|---|
| Forme | fraise plate, hémisphérique, torique, conique (V), ou torche plasma (elle coupe la matière de part en part, seulement quand la broche `M3` ou la torche est allumée) |
| Diamètre | de l'outil (la largeur de coupe d'une torche). Avec *Programme actuel* il part du diamètre du premier groupe du document |
| Rayon du coin / Angle | d'une fraise torique / d'une fraise en V (angle total) |
| Dessus de la matière (Z) | hauteur du dessus du bloc, 0 par défaut |
| Épaisseur | `0` est automatique : la coupe la plus profonde du programme fois 1,25 plus 1 mm, au moins 3 mm |
| Précision | pas entre deux colonnes du bloc, en pourcentage du rayon de l'outil. Le pas est agrandi s'il y avait plus de 2 millions de colonnes |
| Arrêt facultatif (M1) | `M1` met le lecteur en pause quand la case est cochée |

Le bloc est fait autour des déplacements qui coupent, avec une marge de deux fois le rayon de l'outil plus 2 mm. Changer un réglage refait le bloc et rejoue le programme jusqu'à l'endroit où il en était.

**Messages.** Les erreurs et avertissements du traducteur sont écrits dans le journal avec la ligne du programme, comptés à côté du nom du programme, et les lignes sont colorées dans le panneau du programme.

## 2. Déroulement d'une traduction

```
start(options)        une fois, au chargement d'un programme (le module est relu)
next(line)            une fois pour chaque ligne du programme, dans l'ordre, numérotée à partir de 1 par l'application
close()               une fois, à la fin ou quand la traduction est annulée
```

La traduction se fait par tranches de 10 ms à chaque image de la fenêtre, avec une barre de progression et un bouton Annuler : un programme de 100 000 lignes est traduit en environ 2 secondes de Python pendant que la fenêtre reste fluide. Vos méthodes doivent donc être rapides et n'attendre rien. Elles sont toutes appelées depuis un seul fil, dans l'ordre.

## 3. Méthodes de `Translator`

| Méthode | Arguments | Retourne | Sens |
|---|---|---|---|
| `start(options)` | dictionnaire de nombres (aucun n'est donné aujourd'hui) | dictionnaire d'informations, ou `None` | oublier tout : position, modes, avance, unités. La seule information lue est `'unit'` (`'mm'` ou `'inch'`, l'unité du programme) |
| `next(line)` | une ligne du programme, sans fin de ligne | **liste** de commandes, `[]` si la ligne n'en donne pas | traduire une ligne. Votre état change ici |
| `close()` | aucun | rien | libérer ce que vous utilisez. Appelée même après une erreur |
| `extensions()` (facultative) | aucun | liste de chaînes | les extensions des fichiers que le traducteur lit, sans le point (`['plt', 'hpgl']`), pour que *Auto* le choisisse. Sans cette méthode, le traducteur n'est jamais choisi tout seul |

Une exception levée par `next()` n'est pas fatale : elle devient un message d'erreur sur cette ligne, et la ligne suivante est traduite. Une exception dans `start()` arrête le chargement et est écrite dans le journal.

## 4. Commandes

Chaque dictionnaire a `'cmd'`. L'application ajoute le numéro de la ligne du programme : vous ne le donnez pas.

| `cmd` | Champs | Sens |
|---|---|---|
| `rapid` | `x y z` | déplacement rapide au point |
| `linear` | `x y z f` | déplacement à l'avance `f` en mm/min (une avance absente ou nulle est prise pour 1 mm/min) |
| `arc` | `x y z cx cy cz cw f plane` | arc à l'avance `f`. `x y z` est la fin, `cx cy cz` le centre **absolu**, `cw` vaut `True` pour le sens horaire, `plane` vaut `'xy'`, `'xz'` ou `'yz'` |
| `dwell` | `seconds` | pause du programme (`G4`) : le lecteur attend ce temps, qui compte dans le temps de la machine |
| `tool` | `number` | changement d'outil, affiché dans le numéro d'outil |
| `spindle` | `state` (`'cw'`, `'ccw'`, `'off'`), `rpm` | broche. Une torche plasma coupe tant qu'elle n'est pas `'off'` |
| `coolant` | `state` (`'flood'`, `'mist'`, `'off'`) | arrosage |
| `torch` | `state` (`'on'`, `'off'`) | torche plasma, pour un traducteur qui a sa propre commande pour elle |
| `stop` | `optional`, `end` | arrêt du programme. `end` (`M2`, `M30`) termine le programme, ce qui suit n'est pas joué. Sans `end`, le lecteur se met en pause jusqu'à ce qu'on appuie sur Lecture, sauf si `optional` est vrai et *Arrêt facultatif* décoché |
| `message` | `level` (`'info'`, `'warning'`, `'error'`), `text` | un message pour la personne, écrit dans le journal |

Les autres champs d'une commande (par exemple `a`, `b`, `c` pour des axes rotatifs) sont ignorés : le simulateur montre un usinage 2,5 axes en X, Y, Z. Une commande dont le `'cmd'` n'est pas dans ce tableau est notée une fois et ignorée : un traducteur peut donc envoyer de nouvelles commandes avant que le simulateur les connaisse.

**Arcs.** Le plan donne les deux axes du cercle et leur sens, comme le fait le G-code : `'xy'` (`G17`) tourne de X vers Y, `'xz'` (`G18`) de Z vers X, `'yz'` (`G19`) de Y vers Z. Un angle positif est le sens antihoraire (`cw` vaut `False`) vu du côté positif du troisième axe. Le début est la position après la commande précédente. Si le troisième axe change entre le début et la fin, le déplacement est une hélice. Une fin égale au début est un cercle complet. Le traducteur convertit en centre les arcs donnés par un rayon. Le centre et la fin devraient être à la même distance : sinon le rayon varie linéairement du début à la fin.

**Avance.** L'avance est dans chaque déplacement, en millimètres par minute : le traducteur convertit les pouces et les tours.

## 5. Erreurs et avertissements

Un message est une commande : `{'cmd': 'message', 'level': 'warning', 'text': '...'}`. Il est écrit dans le journal avec la ligne qui l'a donné, et le programme continue : tous les problèmes d'un fichier se voient en un seul passage. Avertissez **une seule fois** d'un problème qui se répète, sinon le journal est inondé (voir `warn_once` dans `gcode.py`). Une erreur est pour ce qui rend une ligne sans sens (un arc sans centre), un avertissement pour ce qui est simulé autrement (un code ignoré), une information pour les messages du programme lui-même.

## 6. Le traducteur de G-code générique (`gcode.py`)

| Codes | Sens |
|---|---|
| `G0` `G1` `G2` `G3` | rapide, linéaire, arcs horaire et antihoraire (centre `I J K` relatif, ou rayon `R`, avec une hélice) |
| `G4` | temporisation, avec `P` en secondes ou `X` en secondes |
| `G17` `G18` `G19` | plans |
| `G20` `G21` | pouces, millimètres |
| `G90` `G91`, `G90.1` `G91.1` | distances absolues ou relatives, centres d'arc absolus ou relatifs |
| `G94` `G95` | avance en mm/min ou par tour (`G93` est pris pour `G94`, avec un avertissement) |
| `G80` à `G89`, `G73` | cycles fixes, développés en déplacements (`G81` `G82` `G83` `G73` `G85` `G86` `G89`, et `G84` comme `G81`), avec `G98` `G99` |
| `M3` `M4` `M5` | broche, avec `S` |
| `M6` et `T` | outil |
| `M7` `M8` `M9` | arrosage |
| `M0` `M1` `M2` `M30` | arrêts et fin |
| commentaires | `(texte)`, `; texte`, `(MSG, texte)` affiche un message ; `%` et les lignes qui commencent par `/` sont ignorées ; les numéros de ligne `N` sont ignorés |
| acceptés sans effet | `G40` `G43` `G49` `G54` à `G59` `G61` `G64`, `M48` `M49` |
| `G41` `G42` | la compensation de rayon n'est **pas appliquée** : le chemin programmé est montré (un message) |
| `G28` `G30` `G92` et autres | un avertissement (une fois) et le code est ignoré |

## 7. Exemple minimal complet

Un format avec un point par ligne, `X Y Z` et une avance facultative. Enregistrez sous `~/.openpostpro/simulator/points.py` (le fichier est `tests/sim/points_translator.py` dans les sources), puis choisissez `points` dans la liste de l'onglet Simulation et ouvrez un fichier de points :

```python
class Translator:
	def start(self, options):
		self.first = True
		return {'unit': 'mm'}

	def next(self, line):
		line = line.strip()
		if not line or line.startswith('#'):
			return []
		try:
			values = [float(v) for v in line.replace(',', ' ').split()]
		except ValueError:
			return [{'cmd': 'message', 'level': 'error', 'text': 'not numbers: ' + line}]
		if len(values) < 3:
			return [{'cmd': 'message', 'level': 'warning', 'text': 'a point needs X Y Z: ' + line}]
		x, y, z = values[:3]
		if self.first:
			self.first = False
			return [{'cmd': 'rapid', 'x': x, 'y': y, 'z': z}]
		feed = values[3] if len(values) > 3 else 600.0
		return [{'cmd': 'linear', 'x': x, 'y': y, 'z': z, 'f': feed}]

	def close(self):
		pass
```

## 8. Limites

- Le simulateur vérifie un programme, il ne modélise pas la machine : pas de limites de course, pas d'accélération, pas de collision du porte-outil. La compensation de rayon (`G41` / `G42`) n'est pas appliquée.
- Trois axes X, Y, Z. Les commandes d'un programme 4 ou 5 axes peuvent être jouées en X, Y, Z, mais la matière n'est alors pas fiable.
- L'outil a une seule forme pour tout le programme (les changements d'outil sont affichés, pas simulés).
- La surface de la matière est une grille de colonnes : une paroi très raide montre les marches de la grille, et une coupe étroite dans un très grand bloc peut paraître en pointillés quand la grille est agrandie pour rester sous 2 millions de colonnes.
