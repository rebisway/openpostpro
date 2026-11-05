# Référence de l'API des post-processeurs

*English version: [POSTPROCESSOR.en.md](POSTPROCESSOR.en.md).*

Cette référence s'adresse à qui écrit un post-processeur. Elle liste chaque méthode, regroupée par usage, avec ses arguments, le moment où elle est appelée, ce qu'elle doit retourner et un exemple. L'implémentation de référence est `postpro/generic_mill.py` : copiez-la pour démarrer. Les scripts CAO sont dans [CAD-SCRIPTS.fr.md](CAD-SCRIPTS.fr.md).

Contenu : installation, déroulement d'une génération, toutes les méthodes de la classe `Postpro`, la boîte des propriétés et un exemple minimal complet.

## 0. Conventions

### Fichiers et chargement

| Élément | Règle |
|---|---|
| Emplacement | le dossier `postpro` à côté de l'exécutable |
| Chargement | au démarrage, chaque fichier `.py` est copié dans `~/.openpostpro/postpro` et chargé depuis cet emplacement |
| Nom | le vôtre (`my_mill.py`) : la copie **écrase les fichiers de même nom**, ne modifiez donc jamais un fichier fourni sur place |
| Classe | le fichier définit une classe nommée `Postpro` |
| Moteur | `postcore.py` est le moteur. Il n'est jamais chargé comme post-processeur. Ne le modifiez pas |
| Erreurs | écrites dans `~/.openpostpro/openpostpro.log` |
| Python | 3.12 ou plus, installé avec « Add Python to PATH » (voir le README) |

### Qui appelle qui

```
OpenPostPro (C++)  ->  Postcore (postcore.py)  ->  Postpro (votre fichier)
```

OpenPostPro n'appelle jamais votre classe directement. Il appelle le moteur, qui tient la **position courante** (`x`, `y`, `z`), les numéros de ligne et le nombre de lignes, puis demande à votre classe le texte de chaque étape. Le moteur se transmet à votre classe : c'est `self.core`, renseigné par `set_core()`.

### La règle sur les coordonnées

Tout déplacement ou changement de coordonnées doit passer par l'API du moteur (`self.core.rapid`, `linear`, `clockwise`, `counter_clockwise`, `set_axes`), jamais par une chaîne construite à la main comme `'G0 Z5'`. Le moteur enregistre la position (`self.core.x/y/z`). Un déplacement émis dans son dos fausse les décisions suivantes (hauteur de sécurité, cycles fixes, annulation de compensation), et la sortie devient incohérente.

### Types et encodages

- Chaque méthode de sortie **retourne une chaîne**. Retournez `''` quand il n'y a rien à écrire.
- N'ajoutez pas la fin de ligne : le moteur ajoute `stop_line()` lui-même. Une méthode qui la contient déjà n'est pas doublée.
- `axes` : liste de noms d'axes, par exemple `['X', 'Y', 'Z']`. Les déplacements circulaires ajoutent `'I'`, `'J'`, `'K'` (centre de l'arc).
- `values` : liste de nombres, de même taille et dans le même ordre que `axes`.
- Les nombres sont des `float` Python ; formatez-les vous-même (voir `n()` dans `generic_mill.py`).
- La sortie est du texte, converti en UTF-8 et écrit en mode texte (sous Windows `\n` est écrit `\r\n`). Les fichiers binaires ne sont pas pris en charge par la chaîne retournée.
- Codes de langue : `en-us`, `fr-fr`, `es-es`.

### Écrire moins : les axes optionnels

`pass_xy_to_linear()`, `pass_z_to_linear()`, `pass_xy_to_cw_ccw()` et `pass_z_to_cw_ccw()` choisissent ce que le moteur envoie dans `axes` pour les déplacements linéaires et circulaires. `True` envoie toujours l'axe (X et Y ensemble, ou Z) ; `False` n'envoie que les coordonnées qui ont changé depuis le déplacement précédent. Certains contrôleurs exigent toutes les coordonnées à chaque déplacement, d'autres acceptent qu'elles soient omises : `True` convient aux premiers, `False` donne une sortie plus courte sur les seconds.

## 1. Déroulement d'une génération

Ordre des appels pour un fichier. Les lignes en retrait se répètent.

```
set_language(lang)            au chargement du post-processeur
set_core(core)                une fois
get_property / set_property   chargement et valeurs sauvegardées
-- génération --
start_output()                premiers caractères du fichier
comment(...)                  version, nom du fichier, date
start_program()  initial_position()
  par groupe :
    comment(...)  tool(n)  spindle_speed(s)
    tool_left/right_radius_compensation()  tool_length_compensation(n)
    par parcours d'outil :
      comment(...)  start_toolpath()
      rapid (Z à la hauteur de sécurité, puis XY)
      feed(plongée)  linear (plongée)  feed(travail)
      start_single_path()
      linear / clockwise / counter_clockwise ...
      stop_single_path()   rapid (Z à la hauteur de sécurité)
      stop_toolpath()
    tool_cancel_*_compensation()
final_position()  stop_program()
stop_output()                 derniers caractères du fichier
```

Les groupes de perçage appellent `drilling`, `pecking`, `tapping` ou `boring` à la place de la plongée et des déplacements, puis `cancel_canned_cycle()`.

## 2. Identité

| Méthode | Arguments | Retour | Rôle |
|---|---|---|---|
| `set_language(value)` | `'en-us'`, `'fr-fr'` ou `'es-es'` | rien | choisit le dictionnaire des textes de l'interface |
| `get_description()` | aucun | chaîne | nom dans le menu **Post-processeur / Sélection / Catégorie / Description** |
| `get_category()` | aucun | chaîne | catégorie du menu : `"MILL"`, `"PLASMA"`, ... |
| `get_extention()` | aucun | chaîne | extension du fichier de sortie, sans le point (`'nc'`) |
| `set_core(value)` | le moteur | rien | à mémoriser dans `self.core` ; ne pas modifier |

Chaque langue est un dictionnaire, choisi par `set_language()` :

```python
self.en_us = {'description': 'My mill'}
self.fr_fr = {'description': 'Ma fraiseuse'}
self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr}

def set_language(self, value):
    self.lang = self.Language[value]
```

## 3. Comportement de la machine

| Méthode | Retour | Rôle |
|---|---|---|
| `get_decimal()` | nombre | nombre maximal de décimales |
| `incrementation()` | nombre | pas de la numérotation des lignes (`N5`, `N10`, ...) ; `-1` pour ne pas numéroter |
| `set_safe_position(value)` | rien | hauteur Z de sécurité du groupe courant ; à garder dans `self.safe` |
| `pass_xy_to_linear()`, `pass_z_to_linear()` | booléen | toujours envoyer X Y / Z sur `rapid` et `linear` (voir section 0) |
| `pass_xy_to_cw_ccw()`, `pass_z_to_cw_ccw()` | booléen | idem pour `clockwise` et `counter_clockwise` |
| `use_safe_pos_before_tool_change()` | booléen | remonter à la hauteur de sécurité avant un changement d'outil |
| `get_disable_z()` | booléen | `True` pour une machine sans Z (plasma) : pas de passes de plongée |
| `ijk_relative()` | booléen | `True` quand le centre d'arc `I J K` est donné en relatif, `False` en absolu |

## 4. Propriétés

Les propriétés sont les paramètres du post-processeur. L'utilisateur les modifie dans le menu **Post-processeur / Configuration**. La boîte de dialogue se construit automatiquement d'après le nombre de propriétés, leurs libellés et leurs types : vous n'écrivez aucun code d'interface. OpenPostPro sauvegarde les valeurs lui-même et les redonne via `set_property` au démarrage suivant : le post-processeur ne stocke rien.

| Méthode | Arguments | Retour | Rôle |
|---|---|---|---|
| `get_properties_count()` | aucun | nombre | nombre de propriétés |
| `get_property_description(index)` | indice depuis 0 | chaîne | libellé de la propriété |
| `get_property(index)` | indice depuis 0 | voir ci-dessous | valeur courante ; son type détermine le contrôle |
| `set_property(index, value)` | indice, valeur | rien | reçoit la valeur saisie par l'utilisateur |

| Type retourné par `get_property` | Contrôle |
|---|---|
| `bool` | case à cocher |
| `int` | champ entier avec boutons - et + |
| `float` | champ décimal |
| `str` | champ texte |
| `list` de `str` | liste déroulante ; le premier élément est la valeur sélectionnée, les autres sont les choix |

Pour une liste déroulante, voir la propriété `Unité` de `generic_mill.py`.

## 5. Fichier et format de ligne

| Méthode | Arguments | Retour | Rôle |
|---|---|---|---|
| `start_output()` | aucun | chaîne | premiers caractères du fichier, en général `%\n` |
| `stop_output()` | aucun | chaîne | derniers caractères du fichier, en général `%` |
| `start_program()` | aucun | chaîne | première ligne du programme : mode absolu, unités, ... (`G90 G21`) |
| `stop_program()` | aucun | chaîne | dernière ligne du programme (`M2`) |
| `initial_position()` | aucun | chaîne | premier déplacement ; utilisez `self.core.rapid(...)` |
| `final_position()` | aucun | chaîne | dernier déplacement ; utilisez `self.core.rapid(...)` en testant la position (voir l'exemple) |
| `comment(text)` | le commentaire | chaîne | le commentaire dans la syntaxe du contrôleur ; `''` pour le supprimer |
| `start_line(num)` | numéro de ligne | chaîne | préfixe d'une ligne numérotée (`N00005`) ; `''` sans numérotation |
| `stop_line()` | aucun | chaîne | fin de ligne, en général `\n` |

## 6. Parcours d'outil

| Méthode | Appelée | Retour |
|---|---|---|
| `start_toolpath()` | avant chaque parcours d'outil | chaîne, en général `''` |
| `stop_toolpath()` | après chaque parcours d'outil | chaîne |
| `start_single_path()` | après la première plongée d'un parcours | chaîne |
| `stop_single_path()` | avant la remontée à la hauteur de sécurité | chaîne |

## 7. Déplacements

Chaque méthode retourne la chaîne d'un déplacement. Toutes sont appelées via le moteur, qui enregistre la nouvelle position.

| Méthode | Arguments | Rôle | Exemple (mill générique) |
|---|---|---|---|
| `rapid(axes, values)` | axes, valeurs | déplacement rapide | `rapid(['X','Y'],[10,5.5])` -> `G0X10Y5.5` |
| `linear(axes, values)` | axes, valeurs | déplacement de travail | `linear(['Z'],[-2])` -> `G1Z-2` |
| `clockwise(axes, values)` | `X Y Z I J K` | arc horaire | `G2X10Y0I5J0` |
| `counter_clockwise(axes, values)` | `X Y Z I J K` | arc anti-horaire | `G3X10Y0I5J0` |
| `feed(value)` | vitesse d'avance | vitesse d'avance | `F1200` |
| `pause(time)` | secondes | temporisation | `G4P2` |

Le mill générique est en mode condensé (sans espaces) dans ces exemples ; l'option désactivée donne `G0 X10 Y5.5`.

## 8. Outil et broche

| Méthode | Arguments | Retour (mill générique) |
|---|---|---|
| `tool(number)` | numéro d'outil | `M6T3` ; `''` pour l'outil 0 |
| `start_spindle_clockwise()` | aucun | `M3` |
| `start_spindle_counter_clockwise()` | aucun | `M4` |
| `stop_spindle()` | aucun | `M5` |
| `spindle_speed(speed)` | tr/min | `S12000` |

## 9. Compensation

| Méthode | Arguments | Retour (mill générique) |
|---|---|---|
| `tool_left_radius_compensation()` | aucun | `G41` |
| `tool_right_radius_compensation()` | aucun | `G42` |
| `tool_cancel_radius_compensation()` | aucun | remonte à la hauteur de sécurité si besoin, puis `G40` |
| `tool_length_compensation(tool_number)` | numéro d'outil | `G43H3` |
| `tool_cancel_length_compensation()` | aucun | remonte à la hauteur de sécurité si besoin, puis `G49` |

Quand une méthode d'annulation remonte l'outil, elle le fait avec `self.core.rapid(['Z'], [self.safe])`, seulement si `self.core.z != self.safe`.

## 10. Cycles fixes

| Méthode | Arguments | Rôle |
|---|---|---|
| `drilling(axes, values, retract, pause)` | position, hauteur de dégagement, temporisation | perçage (`G81`, ou `G82` avec temporisation) |
| `pecking(axes, values, retract, delta)` | position, hauteur de dégagement, profondeur de passe | perçage avec débourrage (`G83`) |
| `tapping(axes, values, retract)` | position, hauteur de dégagement | taraudage (`G84`) |
| `boring(axes, values, retract, pause)` | position, hauteur de dégagement, temporisation | alésage (`G85`) |
| `cancel_canned_cycle()` | aucun | annule le cycle (`G80`) |

Une machine sans cycle fixe peut retourner `''` pour tous.

## 11. Autres

| Méthode | Retour (mill générique) |
|---|---|
| `mist()` | `M7` |
| `flood()` | `M8` |
| `stop_coolant()` | `M9` |
| `plane_xy()`, `plane_xz()`, `plane_yz()` | `G17`, `G18`, `G19` |

## 12. Exemple minimal complet

Enregistrez sous `postpro/my_mill.py`. Il contient les méthodes obligatoires et une propriété (le nombre de décimales) :

```python
class Postpro:
	def __init__(self):
		self.safe = 5.0
		self.decim = 3
		self.en_us = {'description': 'My mill', 'decim': 'Decimals'}
		self.Language = {'en-us': self.en_us, 'fr-fr': self.en_us, 'es-es': self.en_us}

	def set_language(self, value):
		self.lang = self.Language[value]

	def set_core(self, value):
		self.core = value

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'MILL'

	def get_extention(self):
		return 'nc'

	def get_decimal(self):
		return self.decim

	def incrementation(self):
		return -1                      # no line numbers

	def set_safe_position(self, value):
		self.safe = value

	def pass_xy_to_cw_ccw(self):
		return False

	def pass_z_to_cw_ccw(self):
		return False

	def pass_xy_to_linear(self):
		return False

	def pass_z_to_linear(self):
		return False

	def use_safe_pos_before_tool_change(self):
		return False

	def get_disable_z(self):
		return False

	def ijk_relative(self):
		return True

	# one property: the number of decimals
	def get_properties_count(self):
		return 1

	def get_property_description(self, index):
		return self.lang['decim']

	def get_property(self, index):
		return self.decim

	def set_property(self, index, val):
		self.decim = int(val)

	# helpers
	def n(self, val):
		text = ('%.' + str(self.decim) + 'f') % val
		text = text.rstrip('0')
		return text[:-1] if text.endswith('.') else text

	def coords(self, axes, values):
		return ' '.join(a + self.n(v) for a, v in zip(axes, values))

	# output
	def start_output(self):
		return '%\n'

	def stop_output(self):
		return '%'

	def start_program(self):
		return 'G90 G21'

	def stop_program(self):
		return 'M2'

	def initial_position(self):
		return self.core.rapid(['Z'], [self.safe])

	def final_position(self):
		if self.core.z != self.safe:           # avoid repeating a move already done
			return self.core.rapid(['Z'], [self.safe])
		return ''

	def comment(self, text):
		return '(' + text + ')'

	def start_line(self, num):
		return ''

	def stop_line(self):
		return '\n'

	def start_toolpath(self):
		return ''

	def stop_toolpath(self):
		return ''

	def start_single_path(self):
		return ''

	def stop_single_path(self):
		return ''

	def rapid(self, axes, values):
		return 'G0 ' + self.coords(axes, values)

	def linear(self, axes, values):
		return 'G1 ' + self.coords(axes, values)

	def clockwise(self, axes, values):
		return 'G2 ' + self.coords(axes, values)

	def counter_clockwise(self, axes, values):
		return 'G3 ' + self.coords(axes, values)

	def feed(self, speed):
		return 'F' + self.n(speed)

	def pause(self, time):
		return 'G4 P' + self.n(time)

	def tool(self, number):
		return 'M6 T' + self.n(number) if number else ''

	def spindle_speed(self, speed):
		return 'S' + self.n(speed)

	def start_spindle_clockwise(self):
		return 'M3'

	def start_spindle_counter_clockwise(self):
		return 'M4'

	def stop_spindle(self):
		return 'M5'
```

Redémarrez l'application : le post-processeur apparaît dans le menu des post-processeurs, dans sa catégorie. L'exemple est volontairement court : les méthodes de compensation, de cycles fixes et d'arrosage sont nécessaires si vos opérations les utilisent. Partez de `generic_mill.py` pour une version complète.
