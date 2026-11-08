# OpenPostPro

**Un outil FAO/CAO open source pour dessiner, préparer des parcours d'outil et générer le code machine.**

OpenPostPro réunit un environnement de dessin 2D léger, des opérations d'usinage (FAO) et des post-processeurs Python configurables. Importez un dessin DXF ou créez la géométrie directement dans l'application, préparez des opérations d'usinage ou de découpe, puis générez une sortie adaptée à votre machine grâce à un post-processeur.

La version actuelle est la **1.0**. Le projet est stable et utilisable, mais certains algorithmes de géométrie et de parcours d'outil peuvent encore échouer sur des formes difficiles : la sortie générée doit donc toujours être vérifiée avec soin avant d'être utilisée sur une vraie machine.

Manuel d'utilisation : [Manuel d'utilisation](docs/MANUAL.fr.md) · [User manual](docs/MANUAL.en.md)

English version: [README.md](README.md)

![OpenPostPro](https://github.com/user-attachments/assets/fc224d77-bbef-4d37-98aa-c54a5e4b89e1)

## ✏️ Dessiner et préparer la géométrie

OpenPostPro contient des outils pour créer et modifier directement la géométrie 2D :

- Points, lignes, cercles, arcs, ellipses, polylignes, splines et textes
- **Texte le long d'une courbe** : un texte peut suivre une ligne, un arc, un cercle ou n'importe quelle chaîne de formes connectées (sélectionnez le texte, puis **Dessin → Lier le texte à une courbe** ou le bouton de ses propriétés, et cliquez sur la courbe). Les lettres gardent leur taille, le début du texte se règle avec un curseur, le texte peut faire le tour d'une courbe fermée, et il se poursuit en ligne droite au-delà de l'extrémité d'une courbe ouverte. Le texte suit sa courbe quand celle-ci est déplacée à la souris ou quand l'origine ou l'unité change ; les icônes en haut de ses propriétés (lier, mettre à jour, délier) et le bouton de rafraîchissement des parcours relisent la courbe dans les autres cas
- Transformations géométriques : décalage, symétrie, échelle, rotation et déplacement de l'origine
- **Prolonger** : prolonge une ligne ou un arc, à partir de l'extrémité la plus proche de la souris, jusqu'à la forme la plus proche, avec un aperçu du résultat sous la souris
- **Couper** : coupe les lignes, arcs, cercles, polylignes et ellipses au point cliqué (l'aimantation reste active) ou au croisement des formes situées sous le clic
- **Contour** (Édition → Sélectionner) : à partir de la sélection, sélectionne toutes les formes qui lui sont connectées
- **Vérifier le dessin** (menu Dessin) : repère les formes de longueur nulle, les formes dessinées deux fois et les extrémités libres (contours ouverts), avec des marqueurs sur le dessin
- Chanfrein et congé (scripts CAO), coupe aux intersections et opérations booléennes pour combiner, soustraire ou intersecter les formes prises en charge
- Import DXF des entités de dessin prises en charge : lignes, points, cercles, arcs, ellipses, arcs d'ellipse, polylignes, splines (B-splines et NURBS), textes et blocs (INSERT, imbriqués, avec leur position, leur échelle et leur rotation). Les extrémités de lignes et d'arcs distantes de moins de 0,01 sont jointes en un seul contour
- Export DXF (AutoCAD R12) : lignes, cercles, arcs, points, polylignes, splines de lignes et d'arcs (conservées exactes avec des renflements), textes, calques et couleurs ; seulement les formes sélectionnées s'il y a une sélection
- Import et export SVG, import Gerber (RS-274X) et Excellon : scripts CAO des menus **Import** et **Export**
- Unité du document (millimètres ou pouces), avec la conversion d'un DXF importé ou de tout le dessin
- Calques (un calque peut être verrouillé : ses formes ne peuvent plus être déplacées, modifiées, supprimées, coupées ni prolongées, mais l'aimantation s'y accroche toujours), annuler et rétablir, et une aimantation sur les points du dessin

## 🐍 Scripts Python pour la CAO

OpenPostPro prend aussi en charge des scripts Python au niveau de la CAO. Les scripts peuvent créer ou modifier la géométrie directement dans l'application, ce qui permet d'automatiser un travail répétitif ou de générer des formes complexes de façon paramétrique au lieu de dessiner chaque élément à la main. Ils reçoivent la sélection courante, la position de la souris et l'unité du document, peuvent afficher une boîte de propriétés, et chaque exécution s'annule en une seule étape.

L'application est livrée avec une bibliothèque de scripts, classés par catégorie dans le menu **Scripts** :

- **Génération** : rectangle, hexagone, étoile, trou oblong, rayons de roue, engrenage à développante extérieure, profil d'aile à partir d'un fichier de coordonnées, grille de trous
- **Géométrie et modification** : réseaux linéaires et polaires, congé, chanfrein, centre de gravité, échelle de cercles
- **Import** et **Export** : import et export SVG, import Gerber et Excellon (pastilles, pistes, régions, trous), vectorisation d'image avec le programme externe Potrace
- **Exemples** : petits scripts pour démarrer (milieu de lignes, rectangle...)

Les scripts livrés sont en lecture seule : utilisez **Cloner** pour en faire votre propre copie modifiable, avec un éditeur intégré qui vérifie la syntaxe et affiche la trace d'erreur Python dans le journal. Les scripts sont distincts des post-processeurs : les scripts CAO génèrent la géométrie du dessin, alors que les post-processeurs traduisent les opérations d'usinage en sortie propre à une machine.

Guide : [Écrire un script CAO](docs/CAD-SCRIPTS.fr.md) · [Writing a CAD script](docs/CAD-SCRIPTS.en.md)

## ⚙️ Opérations d'usinage

Créez des parcours d'outil à partir de votre dessin, organisés en **opérations** (un outil avec sa profondeur, sa profondeur de passe et ses avances, et les parcours usinés avec lui) :

- **Suivre** : suit la géométrie sélectionnée, avec une entrée et une sortie facultatives (une ligne ou un quart de cercle) et la **compensation de rayon** de l'outil, ou du trait de coupe d'une torche plasma (G41 / G42) : l'outil est centré sur la ligne, à l'extérieur ou à l'intérieur de la forme, et le côté est déduit du sens du parcours
- **Décalage** : crée des décalages intérieurs ou extérieurs
- **Poche** : génère des parcours de poche, avec trois stratégies : zigzag (lignes de balayage à n'importe quel angle, recouvrement réglable, avec ou sans le contour), décalages concentriques et spirale (vers l'intérieur ou vers l'extérieur). Les passes de finition sont usinées en dernier, et les îlots sont respectés
- **Perçage** : configure des cycles de perçage, dont le débourrage, le taraudage et l'alésage
- **Spirale** : crée des parcours en spirale
- **Aller à** : ajoute un déplacement de positionnement
- **Ponts** (pattes de maintien) sur les parcours fermés de suivi et de décalage, avec des positions indépendantes déplaçables au curseur, et un choix du point de départ du parcours

Un bouton de l'onglet Opérations recalcule les parcours à partir de leurs formes (ceux qui sont sélectionnés, ou tous) après une modification du dessin.

**Outils et machines.** Une **table d'outils** (fraises et torches : plasma, jet d'eau, électro-érosion à fil) donne à une opération son diamètre, ses avances, sa profondeur de passe, sa broche et son refroidissement, et une **table de données de coupe** par matériau les calcule à partir du diamètre et du nombre de dents. Une opération qui utilise une torche masque les réglages qu'elle n'a pas, et les boutons de poche et de perçage sont désactivés. L'outil peut **descendre en oblique** le long de l'entrée d'un parcours au lieu de plonger. Une **machine** donne la surface de la table, le zéro et le sens des axes (X qui compte de la droite vers la gauche, par exemple) : la surface est dessinée sous le dessin, et le post-processeur et le simulateur suivent les axes.

Guide : [Outils, torches et machines](docs/TOOLS-MACHINES.fr.md) · [Tools, torches and machines](docs/TOOLS-MACHINES.en.md)

Les opérations d'usinage ont des paramètres configurables. Le projet étant encore jeune, les résultats, en particulier les calculs de contours, de décalages et de poches, peuvent ne pas être fiables pour toutes les formes ou situations d'usinage.

## 🎞️ Simulation

L'onglet **Simulation** exécute un programme G-code sur un bloc de matière, en 3D, avant son envoi à une machine. Il charge la sortie du post-processeur courant (ou n'importe quel fichier G-code, qui peut aussi être déposé sur la fenêtre), affiche les parcours (avances et rapides), l'outil et la matière qu'il enlève (fraise plate, sphérique, torique, en V ou torche plasma), et mesure la durée de l'usinage à partir des avances.

- Lecture, pause, pas à pas par commande ou par ligne, retour en arrière, aller à une ligne (un clic dans le texte du programme), aller au résultat final, vitesses de x1 à x500 et pleine vitesse. Les arrêts (M0, M1 facultatif) et les temporisations du programme sont exécutés comme le fait une machine, et ignorés quand le simulateur va vite.
- Le programme est traduit par un **traducteur Python** (`simulator/gcode.py` pour le G-code générique : déplacements, arcs, hélices, cycles fixes, unités, plans, broche, arrosage, outils) en quelques commandes simples que le simulateur joue. Un traducteur pour un autre contrôleur ou un autre format n'a besoin que des trois méthodes `start`, `next` et `close` (voir le guide ci-dessous).
- C'est une vérification du programme, pas un modèle de la machine : la compensation de rayon de la fraise (G41 / G42) n'est pas appliquée, et les axes A, B, C sont ignorés.

Guide : [Simulateur et traducteurs](docs/SIMULATOR.fr.md) · [Simulator and translators](docs/SIMULATOR.en.md)

## 🧩 Post-processeurs Python

Les post-processeurs sont écrits en Python, ce qui rend la personnalisation de la sortie machine accessible sans modifier l'application C++. Ils peuvent être clonés et modifiés depuis l'application.

Guide : [Écrire un post-processeur](docs/POSTPROCESSOR.fr.md) · [Writing a post-processor](docs/POSTPROCESSOR.en.md)

Le dépôt contient des post-processeurs génériques pour :

- **Fraisage / défonceuse**
- **Découpe plasma**

et les variantes de quelques contrôleurs, chacune pour le fraisage et pour la découpe plasma : **Mach3** (fichier `.tap`, centres d'arc absolus avec `G90.1`), **Num 1060** (espaces entre les mots, pas de `G64`, temporisation avec `F`) et **Burny 2.5** (chaque arc donne `X Y I J`, temporisation avec `F`). **HPGL** est un post-processeur pour traceurs et plotters de découpe vinyle (arcs découpés en petits segments, le numéro d'outil est le stylo). Une variante est un court fichier qui dérive du générique et n'écrit que ce qui diffère : un bon modèle pour votre propre contrôleur.

Les réglages des post-processeurs comprennent l'extension du fichier de sortie, la précision décimale, la numérotation des lignes, les commentaires, le début et la fin de fichier, les unités de coordonnées, et les coordonnées relatives ou absolues des centres d'arc. Le début et la fin de fichier et les commandes plasma s'écrivent sur plusieurs lignes, chacune numérotée quand la numérotation est active. Une option **Retour 0** ramène l'outil en X0 Y0 à la hauteur de sécurité à la fin du programme. Le post-processeur plasma propose aussi des réglages pour les commandes de la torche, la temporisation d'amorçage, la prise de référence et la gestion de l'axe Z.

Ce sont des points de départ génériques, pas des profils garantis pour tous les contrôleurs. Relisez et adaptez la sortie à votre machine et à votre contrôleur avant de l'utiliser.

## 🌍 Interface et technologie

La langue de l'interface est gérée par des fichiers clé/valeur dans `openpostpro/lang`. L'anglais (UK et US), le français, l'espagnol, l'allemand, l'italien, le néerlandais et le portugais du Brésil sont inclus, et la langue et l'unité se choisissent au premier démarrage. Le journal peut être ancré en bas de la fenêtre, et **Aide → À propos** affiche la version et la date de compilation.

OpenPostPro est écrit en **C++20** et utilise :

- **OpenGL 3** pour l'affichage
- **Dear ImGui** pour l'interface utilisateur
- **GLFW** pour la fenêtre et les entrées
- **GLM** pour les mathématiques
- **FreeType** pour le rendu des polices
- **Python 3.12 ou plus récent**, embarqué, pour les post-processeurs et les scripts (ABI stable, aucune recompilation nécessaire quand Python est mis à jour)

## 🖥️ Utilisation sous Windows

Les fichiers de projet actuels et les bibliothèques d'exécution fournies visent Windows. Une archive prête à l'emploi (`OpenPostPro-v<version>-win64.zip`) est publiée dans la section **Releases** du dépôt : décompressez-la où vous voulez et lancez `openpostpro.exe`.

Il vous faut :
- **Python 3.12 ou plus récent**, installé avec **« Add Python to PATH »** coché. OpenPostPro ne livre pas Python : `python3.dll` est trouvée par le PATH, et sans elle Windows signale « python3.dll was not found » au démarrage. La même installation fournit la bibliothèque standard et tkinter (utilisé par certains scripts CAO).
- Le **Microsoft Visual C++ x64 Redistributable**

Vous pouvez aussi compiler l'application vous-même. Le projet contient une solution Visual Studio dans `openpostpro/openpostpro.sln`.

## 🛠️ Compiler depuis les sources

1. Installez Visual Studio avec les outils de développement C++ pour le bureau, et Python 3.12 ou plus récent (avec « Add Python to PATH »). Les en-têtes Python et `python3.lib` viennent du dépôt (`src/third/python`, `lib/python`).
2. Ouvrez `openpostpro/openpostpro.sln`.
3. Compilez la solution pour la configuration x64 voulue (Debug ajoute un menu **Débogage** avec des fonctions de développement ; Release non).
4. Le script `copy_libs_output.bat`, lancé par la solution après la compilation, copie les ressources d'exécution nécessaires à côté de l'exécutable : polices, fichiers de langue, post-processeurs, scripts, textures, shaders et `freetype.dll`.

Le numéro de version, le numéro de build et la date de compilation affichés dans la fenêtre À propos sont dans `openpostpro/src/common/version.h` (la date est celle de la compilation).

Le code source contient relativement peu de parties propres à Windows, et des compilations Linux ou macOS sont peut-être possibles, mais **aucune n'a encore été testée**.

## ✅ Tests

Les tests vérifient les parties les plus délicates : la géométrie utilisée par les décalages, les poches et les outils couper et prolonger, et les scripts CAO.

- **Python** (scripts CAO et leur bibliothèque de géométrie) : `python tests/run_tests.py`
- **C++** (géométrie, poches, prolonger et couper), compilés avec le compilateur de Visual Studio et lancés sans aucune fenêtre : voir l'en-tête de chaque fichier de `tests/cpp` pour ce qu'il vérifie, compilez-le avec le `build_*.bat` correspondant, puis lancez le `.exe`. Le code de sortie est 0 quand tout passe.

Lancez-les après toute modification du code de géométrie (`curve.cpp`, `geometry.cpp`, `pocket.cpp`, les fichiers de coupe et de prolongement).

## 🐧 Linux et macOS

OpenPostPro est conçu dans un esprit multiplateforme, avec très peu de dépendances à des fonctions propres à Windows. L'application a déjà été compilée et exécutée avec succès sur **Ubuntu 21.04**, avec Visual Studio Code. Cela montre que compiler et lancer OpenPostPro sous Linux est faisable, même si la configuration exacte de compilation n'a pas encore été documentée et devra peut-être être recréée. macOS devrait aussi être une cible possible, mais n'a pas encore été testé. Les contributions pour rétablir, documenter et valider des compilations sur ces plateformes sont les bienvenues.

## 🚧 État du projet

OpenPostPro est en version 1.0, un projet open source indépendant. La priorité principale pour les améliorations à venir reste la robustesse et la précision des calculs de géométrie et de contours (décalages et poches sur des formes difficiles) ; certains algorithmes utilisent encore la précision des nombres à virgule flottante et demandent encore du travail.

Merci de considérer les parcours d'outil et le code machine générés comme une sortie qui demande une vérification. Simulez ou inspectez le programme, et contrôlez les réglages de la machine, les définitions d'outils, les dégagements, les avances et les vitesses avant de l'exécuter sur un équipement réel.

Les rapports de bogues, les retours et les contributions sont les bienvenus.

## 📜 Licence

OpenPostPro est publié sous la licence **GNU Affero General Public License v3.0 ou ultérieure (AGPL-3.0-or-later)**. Voir [`LICENSE`](LICENSE) pour le texte complet de la licence.

Les composants tiers gardent leurs propres licences, toutes compatibles avec l'AGPL ; voir [`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md).

## 🔗 Liens

- **Dépôt :** https://github.com/rebisway/openpostpro
- **Contact :** contact@rebisway.com

Je me suis beaucoup amusé à construire OpenPostPro. J'espère que vous aurez plaisir à l'explorer, à l'utiliser et à l'aider à s'améliorer.
