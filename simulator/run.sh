#!/usr/bin/env bash
# Compile (si besoin) puis lance le simulateur PC depuis Git Bash / WSL sur
# Windows, avec la meme chaine de compilation reproductible que run.ps1
# (pip: ziglang + cmake + ninja). Le simulateur cible Windows (SDL2 vendorise
# en x86_64-w64-mingw32) -- voir simulator/README.md pour une adaptation Linux
# native (SDL2 systeme) si besoin.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v python3 >/dev/null 2>&1 && ! command -v python >/dev/null 2>&1; then
  echo "Python introuvable. Installez Python 3.10+ puis relancez ce script." >&2
  exit 1
fi
PY=$(command -v python3 || command -v python)

if ! "$PY" -c "import ziglang, cmake, ninja" >/dev/null 2>&1; then
  echo "Installation de ziglang, cmake, ninja via pip (une seule fois)..."
  "$PY" -m pip install --quiet ziglang cmake ninja
fi

SCRIPTS_DIR="$("$PY" -c "import sysconfig; print(sysconfig.get_path('scripts'))")"
export PATH="$SCRIPTS_DIR:$PATH"

BUILD_DIR="$SCRIPT_DIR/build"
TOOLCHAIN_FILE="$SCRIPT_DIR/toolchain/zig-toolchain.cmake"

if [ ! -d "$BUILD_DIR" ]; then
  echo "Configuration CMake (premiere fois)..."
  cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -G Ninja \
    "-DCMAKE_MAKE_PROGRAM=$SCRIPTS_DIR/ninja" \
    "-DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
fi

echo "Compilation..."
cmake --build "$BUILD_DIR"

"$BUILD_DIR/trophy-display-simulator.exe" --screenshot-dir "$SCRIPT_DIR/screenshots" "$@"
