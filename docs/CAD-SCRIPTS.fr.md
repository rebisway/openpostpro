# Référence de l'API des scripts CAO

*English version: [CAD-SCRIPTS.en.md](CAD-SCRIPTS.en.md).*

Cette référence s'adresse à qui écrit un script CAO : un fichier Python qui crée de la géométrie (lignes, cercles, arcs) dans le dessin, à la position de la souris. Elle liste chaque méthode de la classe `Cadscript` et chaque appel du moteur, avec les arguments, le moment de l'appel et un exemple. Le fichier fourni `script/oblong.py` (un trou oblong, avec une boîte `tkinter`) est l'implémentation de référence. Les post-processeurs sont dans [POSTPROCESSOR.fr.md](POSTPROCESSOR.fr.md).

Contenu : installation, déroulement d'une exécution, méthodes de `Cadscript`, appels de dessin du moteur et un exemple minimal complet.

## 0. Conventions

### Fichiers et chargement

| Élément | Règle |
|---|---|
| Emplacement | le dossier `script` à côté de l'exécutable |
| Chargement | au démarrage, chaque fichier `.py` est copié dans `~/.openpostpro/script` et chargé depuis cet emplacement |
| Nom | le vôtre (`my_rectangle.py`) : la copie **écrase les fichiers de même nom** |
| Classe | le fichier définit une classe nommée `Cadscript` |
| Moteur | `cadcore.py` est le moteur. Il n'est jamais chargé comme script. Ne le modifiez pas |
| Erreurs | écrites dans `~/.openpostpro/openpostpro.log` |
| Python | 3.12 ou plus, installé avec « Add Python to PATH » (voir le README) |
| `tkinter` | écrivez `import tkinter` : `cadcore.py` indique déjà à Python où trouver les bibliothèques Tcl/Tk de l'installation |

### Qui appelle qui

```
OpenPostPro (C++)  ->  Cadcore (cadcore.py)  ->  Cadscript (votre fichier)
```

Le moteur se transmet à votre classe : c'est `self.core`, renseigné par `set_core()`. Votre classe dessine en appelant `self.core` ; le moteur collecte le dessin sous forme de texte et le renvoie à l'application quand `run()` se termine.

### Types et encodages

- Les coordonnées sont des nombres en unités du dessin (`float`).
- `cw` d'un arc est un booléen : `True` pour le sens horaire.
- Codes de langue : `en-us`, `en-uk`, `fr-fr`, `es-es`.

## 1. Déroulement d'une exécution

```
set_language(lang)    au chargement du script
set_core(core)        une fois
get_description() get_category()    pour construire le menu Scripts
get_property / set_property         chargement et valeurs sauvegardées
-- quand l'utilisateur lance le script --
core.mouse_x / mouse_y sont renseignés   position de la souris
run()                               le script dessine et peut ouvrir une fenêtre
-> le dessin renvoyé par le moteur est ajouté au document
```

## 2. Méthodes de `Cadscript`

| Méthode | Arguments | Retour | Rôle |
|---|---|---|---|
| `set_language(value)` | `'en-us'`, `'en-uk'`, `'fr-fr'` ou `'es-es'` | rien | choisit le dictionnaire des textes de l'interface |
| `set_core(value)` | le moteur | rien | à mémoriser dans `self.core` ; ne pas modifier |
| `get_description()` | aucun | chaîne | nom affiché dans le menu Scripts |
| `get_category()` | aucun | chaîne | catégorie du menu, en général `'CAD'` |
| `get_properties_count()` | aucun | nombre | nombre de paramètres du script |
| `get_property_description(index)` | indice depuis 0 | chaîne | libellé du paramètre |
| `get_property_key(index)` | indice depuis 0 | chaîne | optionnel : nom utilisé pour sauvegarder la valeur (par défaut : l'indice) |
| `use_properties()` | aucun | `bool` | optionnel : `False` si le script affiche sa propre fenêtre (défaut `True`) |
| `get_property(index)` | indice depuis 0 | `bool`, `int`, `float`, `str` ou `list` de `str` | valeur courante |
| `set_property(index, value)` | indice, valeur | rien | reçoit la valeur sauvegardée au démarrage, ou la valeur modifiée |
| `run()` | aucun | rien | crée la géométrie via `self.core` |

Si `get_properties_count()` renvoie plus de 0, OpenPostPro affiche la boîte de propriétés (mêmes contrôles que pour les post-processeurs, voir POSTPROCESSOR.fr.md section 4) quand le script est choisi. **Ok** envoie les valeurs par `set_property` puis exécute le script ; **Annuler** l'arrête. Les valeurs sont sauvegardées par script et redonnées à l'appel suivant. Renvoyez un nom stable dans `get_property_key` pour que l'ajout ou le déplacement d'une propriété ne mélange jamais les valeurs sauvegardées.

Un script peut aussi gérer sa propre fenêtre (par exemple `tkinter`, pour ouvrir un sélecteur de fichier) : renvoyez `False` dans `use_properties()` et aucune boîte n'est affichée ; `oblong.py` en est un exemple.

## 3. Appels du moteur (`self.core`)

### Position de la souris

| Attribut | Type | Rôle |
|---|---|---|
| `self.core.mouse_x` | `float` | X de la souris au lancement du script |
| `self.core.mouse_y` | `float` | Y de la souris au lancement du script |

À utiliser comme origine de la forme.

### Dessin

| Appel | Arguments | Effet |
|---|---|---|
| `self.core.line(x1, y1, x2, y2)` | début, fin | ajoute une ligne |
| `self.core.circle(x, y, r)` | centre, rayon | ajoute un cercle |
| `self.core.arc(x1, y1, cx, cy, x2, y2, cw)` | début, centre, fin, sens | ajoute un arc de `(x1, y1)` à `(x2, y2)` autour de `(cx, cy)` ; `cw` vaut `True` pour le sens horaire |
| `self.core.append_output(text)` | texte brut | ajoute du texte à la sortie du dessin (avancé) |

Le moteur écrit une ligne de texte par élément, dans ce format :

| Élément | Texte |
|---|---|
| ligne | `L:x1:y1:x2:y2` |
| cercle | `C:x:y:r` |
| arc | `A:x1:y1:cx:cy:x2:y2:clock` où `clock` vaut `1` pour le sens horaire, `0` sinon |

`append_output` permet à un script d'écrire lui-même de telles lignes. La géométrie est ajoutée au dessin quand `run()` se termine.

## 4. Exemple minimal complet

Enregistrez sous `script/my_rectangle.py`. Il n'a pas de fenêtre et dessine à partir de ses deux propriétés :

```python
class Cadscript:
	def __init__(self):
		self.width = 100.0
		self.height = 50.0
		self.en_us = {'description': 'Rectangle', 'width': 'Width', 'height': 'Height'}
		self.fr_fr = {'description': 'Rectangle', 'width': 'Largeur', 'height': 'Hauteur'}
		self.es_es = {'description': 'Rectangulo', 'width': 'Ancho', 'height': 'Alto'}
		self.Language = {'en-us': self.en_us, 'fr-fr': self.fr_fr, 'es-es': self.es_es}

	def set_language(self, value):
		self.lang = self.Language.get(value, self.en_us)   # unknown language: English

	def set_core(self, value):
		self.core = value

	def get_description(self):
		return self.lang['description']

	def get_category(self):
		return 'CAD'

	def get_properties_count(self):
		return 2

	def get_property_description(self, index):
		return self.lang['width'] if index == 0 else self.lang['height']

	def get_property(self, index):
		return self.width if index == 0 else self.height

	def set_property(self, index, val):
		if index == 0:
			self.width = float(val)
		else:
			self.height = float(val)

	def run(self):
		x = self.core.mouse_x
		y = self.core.mouse_y
		w = self.width
		h = self.height
		self.core.line(x, y, x + w, y)
		self.core.line(x + w, y, x + w, y + h)
		self.core.line(x + w, y + h, x, y + h)
		self.core.line(x, y + h, x, y)
```

Pour un script avec sa propre fenêtre de saisie, voir `script/oblong.py` : il construit une boîte `tkinter` dans `run()`, lit les valeurs, puis appelle `compute()` pour produire lignes et arcs.
