@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo  Particle Simulator - Build Script
echo ==========================================
echo.

set BUILD_DIR=build
set BUILD_TYPE=Release

:: Check for CMake
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERREUR] CMake non trouve. Veuillez l'installer.
    exit /b 1
)

:: Check for NVCC (optional)
where nvcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] NVCC non trouve. Compilation en mode CPU uniquement.
    set USE_CUDA_FLAG=-DUSE_CUDA=OFF
) else (
    echo [INFO] CUDA detecte. Utilisez -DUSE_CUDA=ON pour activer le support GPU.
    set USE_CUDA_FLAG=-DUSE_CUDA=OFF
)

:: Create build directory
if not exist %BUILD_DIR% (
    echo Creation du repertoire de build...
    mkdir %BUILD_DIR%
)

cd %BUILD_DIR%

:: Configure with CMake
echo.
echo Configuration CMake...
echo.

cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    %USE_CUDA_FLAG%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] Configuration CMake echouee.
    echo.
    echo Verifiez que vous avez installe:
    echo   - Qt6 (et defini CMAKE_PREFIX_PATH)
    echo   - Raylib (via vcpkg ou manuellement)
    echo   - [OPTIONNEL] CUDA Toolkit (pour le support GPU)
    echo.
    echo Exemple avec vcpkg:
    echo   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
    echo.
    echo Pour activer CUDA:
    echo   cmake .. -DUSE_CUDA=ON
    cd ..
    exit /b 1
)

:: Build
echo.
echo Compilation...
echo.

cmake --build . --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERREUR] Compilation echouee.
    cd ..
    exit /b 1
)

echo.
echo ==========================================
echo  Build termine avec succes!
echo ==========================================
echo.
echo Executable: %CD%\%BUILD_TYPE%\ParticleSimulator.exe
echo.

cd ..
exit /b 0
