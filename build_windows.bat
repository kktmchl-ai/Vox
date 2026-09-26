@echo off
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 || exit /b 1
cmake --build build --config Release --target VoxChain_VST3 || exit /b 1
echo.
echo Done. Plugin folder:  build\VoxChain_artefacts\Release\VST3\VoxChain.vst3
