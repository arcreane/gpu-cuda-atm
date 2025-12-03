@echo off
echo Lancement du Simulateur de Particules...
echo.

if exist "build\Release\ParticleSimulator.exe" (
    start "" "build\Release\ParticleSimulator.exe"
) else if exist "build\Debug\ParticleSimulator.exe" (
    start "" "build\Debug\ParticleSimulator.exe"
) else (
    echo [ERREUR] Executable non trouve.
    echo Veuillez d'abord compiler le projet avec build.bat
    pause
)
