# Tololoche X (VST3)

Excitador de subarmónicos + saturación "madera" para bajo sexto/tololoche.

## Compilar (Windows)
Requisitos: Visual Studio 2022 (carga de trabajo C++) + CMake 3.22+ + Git.

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release

El .vst3 queda en `build/TololocheX_artefacts/Release/VST3/Tololoche X.vst3`.
Copia esa carpeta a `C:\Program Files\Common Files\VST3\`.

## Sin instalar nada
Sube esta carpeta a un repo de GitHub: el workflow `.github/workflows/build.yml`
compila en la nube y deja el VST3 en Actions > Artifacts.
