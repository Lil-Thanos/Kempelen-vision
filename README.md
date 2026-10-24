# Kempelen

Assistant d'analyse d'échecs en temps réel, écrit en **C++**. Il regarde l'écran, repère l'échiquier, lit la position en direct avec un modèle **YOLO** entraîné, demande le meilleur coup à **Stockfish** et dessine une flèche **directement par-dessus la fenêtre** du jeu.

> Nommé d'après Wolfgang von Kempelen, l'inventeur du « Turc mécanique », l'automate joueur d'échecs du XVIIIe siècle (qui cachait en réalité un joueur humain).

<img width="500" height="600" alt="image" src="https://github.com/user-attachments/assets/f3dac4fe-c47c-4bd9-aa0f-398baa235892" />

## Comment ça marche

```
Capture écran (DXGI) → localisation du plateau (OpenCV) → crop
        → YOLO11n (ONNX, OpenCV DNN) → grille 8×8 → FEN
        → Stockfish (UCI, thread séparé) → flèche sur l'overlay transparent
```

1. **Capture** : DXGI Desktop Duplication, directement depuis la carte graphique, sans passer par des fichiers image.
2. **Plateau** : détection du plus grand contour carré (Canny + `findContours`), revérifiée régulièrement, avec tolérance de quelques pixels pour que la grille ne tremble pas.
3. **Pièces** : YOLO11n à 12 classes (6 types × 2 couleurs), exporté en ONNX et exécuté en C++ avec OpenCV DNN. Le NMS et le décodage de la sortie `[1, 16, 8400]` sont codés à la main.
4. **Position** : chaque boîte est projetée sur sa case. Une lecture n'est acceptée que si elle est **stable sur deux analyses** et **plausible** (un roi par camp, pas plus de 8 pions...).
5. **Trait** : déduit en comparant deux positions successives (les cases modifiées contenant une pièce appartiennent à celui qui vient de jouer).
6. **Moteur** : Stockfish est lancé comme sous-processus et piloté par le protocole UCI (pipes Windows). Il tourne dans un thread dédié et redémarre automatiquement s'il s'arrête.
7. **Affichage** : fenêtre transparente, toujours au-dessus, traversée par la souris, avec double buffering. Elle est **exclue de la capture d'écran** pour que l'IA ne lise pas ses propres dessins.

## Entraînement du modèle

- 5 000 images d'entraînement et 600 de validation, positions aléatoires issues de `python-chess`
- jeux de pièces SVG de Lichess, **les jeux de validation ne sont jamais utilisés à l'entraînement** (mesure honnête de la généralisation)
- variations : 9 palettes de plateau avec bruit de couleur, plateau retourné, dernier coup surligné, coordonnées affichées ou non, marges, flou, compression JPEG, taille des cases

```bash
yolo detect train data=dataset_v2/chess.yaml model=yolo11n.pt imgsz=640 epochs=80 batch=32 \
     hsv_h=0.0 hsv_s=0.3 hsv_v=0.2 fliplr=0.0
yolo export model=runs/detect/trainX/weights/best.pt format=onnx imgsz=640 opset=12
```

Les augmentations de teinte sont réduites volontairement : elles brouillent la distinction pièce blanche / pièce noire. Entrainé avec l'aide d'un GPU nvidia 5070ti.


## Compilation (Windows)

Prérequis : Windows 10/11 version 2004 ou plus récent, Visual Studio (MSVC, C++20), CMake ≥ 3.20, OpenCV 4.x avec le module `dnn`.

```bash
git clone https://github.com/Lil-Thanos/Kempelen-vision/
cd Kempelen-vision
```

Configure et compile avec CMake (le dossier `engine/` est copié automatiquement à côté de l'exécutable).

## Utilisation


| Touche | Action |
|---|---|
| `Echap` | quitter |
| `F8` | inverser le trait (si la déduction s'est trompée) |
| `F9` | inverser l'orientation du plateau |
| `F10` | rendre l'overlay visible pour les captures d'écran |

La console affiche le suivi de la partie : coups détectés, trait, FEN, meilleur coup du moteur, lectures refusées et pourquoi.

## Limites connues

- Plateaux **numériques 2D** uniquement, vus de dessus (pas de photos de plateaux physiques).
- Windows uniquement, écran principal uniquement.
- Droits de roque et prise en passant non suivis : le FEN envoyé contient `-`, donc Stockfish ne propose pas de roquer.
- Le trait est une déduction : au démarrage on suppose que les blancs jouent, et `F8` corrige.
- Une position invraisemblable (pièce en animation, pièce masquée) est ignorée plutôt qu'analysée.
- Le modèle peut mal généraliser aux thèmes de pièces très éloignés de ceux de l'entraînement.

## Usage responsable

Ce projet est un **outil d'analyse et d'apprentissage** : à utiliser pour étudier ses parties, jouer contre des bots ou explorer des positions. L'utiliser pendant une partie classée contre un humain est de la **triche** : c'est contraire aux règles de la grande majorité des plateformes et mène au bannissement.

## Crédits et licences

- [Stockfish](https://stockfishchess.org/) (GPL v3) : lancé comme programme externe, non redistribué dans ce dépôt
- [Ultralytics YOLO11](https://github.com/ultralytics/ultralytics) : entraînement et export du modèle (licence AGPL-3.0 : vérifie ses conditions avant toute redistribution des poids)
- [OpenCV](https://opencv.org/) : capture, traitement d'image, inférence DNN
- [python-chess](https://github.com/niklasf/python-chess) : génération des positions
- Jeux de pièces : [Lichess](https://github.com/lichess-org/lila), chaque jeu a sa propre licence (voir `public/piece` dans leur dépôt) ; ils ne sont pas inclus dans ce dépôt
