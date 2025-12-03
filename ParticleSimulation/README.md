# Particle Simulator - Compilation CPU

Ce document explique comment compiler et exécuter le simulateur de particules **sans GPU NVIDIA** (mode CPU uniquement).

## 🚀 Démarrage rapide

Si vous avez déjà vcpkg et Qt6 installés:

```bash
# 1. Installer raylib via vcpkg
vcpkg install raylib:x64-windows

# 2. Créer le dossier de build
mkdir build
cd build

# 3. Configurer CMake (adaptez les chemins à votre installation)
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-windows \
    -DQt6_DIR="C:/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6"

# 4. Compiler
cmake --build . --config Debug

# 5. Exécuter
Debug/ParticleSimulator.exe
```

**⚠️ Points critiques à vérifier**:
- Le flag `-DVCPKG_TARGET_TRIPLET=x64-windows` est **obligatoire**
- Les chemins vers vcpkg et Qt6 doivent correspondre à votre installation
- Si CMake échoue, nettoyez le cache: `rm -f CMakeCache.txt && rm -rf CMakeFiles/`


## Prérequis

Pour compiler en mode CPU uniquement, vous avez besoin de:
- **CMake** (version 3.18 ou supérieure)
- **Qt6** (Widgets, Core, Gui)
- **Raylib** (via vcpkg recommandé)
- **Visual Studio 2022** (ou un autre compilateur C++17)

**Note:** CUDA Toolkit n'est **PAS nécessaire** pour la compilation en mode CPU.

## Installation des dépendances

### Option 1: Avec vcpkg (recommandé)

```bash
# Installer vcpkg si ce n'est pas déjà fait
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# Installer raylib
vcpkg install raylib:x64-windows

# Qt6 doit être installé séparément depuis le site officiel
# https://www.qt.io/download
```

### Option 2: Installation manuelle

Installez Qt6 et Raylib manuellement et configurez les variables d'environnement appropriées.

## Compilation manuelle

```bash
# Créer le dossier de build
mkdir build
cd build

# Configurer CMake avec vcpkg (IMPORTANT: spécifier le triplet x64-windows)
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-windows \
    -DQt6_DIR="C:/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6"

# Note: Adaptez les chemins selon votre installation
# - Remplacez C:/vcpkg par le chemin de votre installation vcpkg
# - Remplacez C:/Qt/6.9.3 par votre version de Qt

# Compiler (Debug ou Release)
cmake --build . --config Debug
# ou
cmake --build . --config Release
```

**⚠️ IMPORTANT**: Le flag `-DVCPKG_TARGET_TRIPLET=x64-windows` est **obligatoire** si vous avez installé raylib avec vcpkg en x64-windows. Sans ce flag, CMake cherchera les bibliothèques en x86-windows et ne trouvera pas raylib.

## Activer le support GPU (optionnel)

Si vous avez CUDA Toolkit installé et souhaitez utiliser le GPU:

```bash
# Dans le dossier build
cmake .. -DUSE_CUDA=ON
cmake --build . --config Release --parallel
```

## Exécution

```bash
# Depuis la racine du projet
run.bat

# Ou directement
build\Release\ParticleSimulator.exe
```

## Interface utilisateur

En mode CPU uniquement:
- Le menu déroulant "Mode" affiche seulement "CPU (Sequentiel)"
- L'option GPU n'est pas disponible (grisée)
- Toutes les autres fonctionnalités restent identiques

## Performance

Le mode CPU est plus lent que le mode GPU mais fonctionne sur n'importe quelle machine:
- **CPU**: Adapté pour 100-5000 particules
- **GPU (avec CUDA)**: Peut gérer 10000+ particules

## Dépannage

### Erreur "Qt6 not found"
Définissez la variable d'environnement `CMAKE_PREFIX_PATH`:
```bash
set CMAKE_PREFIX_PATH=C:\Qt\6.x.x\msvc2022_64
```

Ou spécifiez directement le chemin dans CMake:
```bash
cmake .. -DQt6_DIR="C:/Qt/6.9.3/msvc2022_64/lib/cmake/Qt6"
```

### Erreur "raylib not found"
**Solution 1**: Assurez-vous d'avoir installé raylib avec vcpkg pour x64-windows:
```bash
vcpkg install raylib:x64-windows
```

**Solution 2**: Ajoutez le flag de triplet lors de la configuration CMake:
```bash
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-windows
```

**Solution 3**: Si l'erreur persiste après configuration, nettoyez le cache CMake:
```bash
rm -f CMakeCache.txt
rm -rf CMakeFiles/
# Puis relancez cmake avec les bons paramètres
```

### Erreur "no Qt platform plugin could be initialized"
Si le programme se lance mais affiche cette erreur, le plugin de plateforme Qt est manquant.

**Solution automatique**: Le CMakeLists.txt a été mis à jour pour copier automatiquement le plugin. Recompilez:
```bash
cmake --build . --config Debug
```

**Solution manuelle**: Copiez manuellement le plugin:
```bash
# Pour Debug
mkdir Debug/platforms
cp "C:/Qt/6.9.3/msvc2022_64/plugins/platforms/qwindowsd.dll" Debug/platforms/

# Pour Release
mkdir Release/platforms
cp "C:/Qt/6.9.3/msvc2022_64/plugins/platforms/qwindows.dll" Release/platforms/
```

### Le programme ne démarre pas - DLLs manquantes
Vérifiez que les DLLs suivantes sont présentes dans le dossier de l'exécutable:

**DLLs Qt6**:
- Qt6Core.dll (ou Qt6Cored.dll pour Debug)
- Qt6Gui.dll (ou Qt6Guid.dll pour Debug)
- Qt6Widgets.dll (ou Qt6Widgetsd.dll pour Debug)

**Plugin de plateforme Qt**:
- `platforms/qwindows.dll` (ou `platforms/qwindowsd.dll` pour Debug)

**Autres DLLs**:
- raylib.dll
- glfw3.dll (si utilisé)

Le CMakeLists.txt devrait copier automatiquement ces DLLs lors de la compilation.

## Structure du projet

```
ParticleSimulator/
├── src/
│   ├── core/          # Application principale
│   ├── physics/       # Moteurs physiques (CPU et GPU)
│   ├── cuda/          # Code CUDA (non compilé en mode CPU)
│   ├── renderer/      # Rendu avec Raylib
│   └── ui/            # Interface Qt
├── CMakeLists.txt     # Configuration CMake
└── build.bat          # Script de compilation
```

## Notes importantes

- Par défaut, le simulateur compile **sans CUDA** même si CUDA est installé
- Pour activer CUDA, utilisez explicitement `-DUSE_CUDA=ON`
- Le mode CPU utilise un algorithme séquentiel optimisé
- Les performances dépendent du CPU (nombre de cœurs, fréquence)

## Support

Si vous rencontrez des problèmes:
1. Vérifiez que toutes les dépendances sont installées
2. Assurez-vous d'utiliser Visual Studio 2022 ou équivalent
3. Vérifiez les messages d'erreur de CMake
