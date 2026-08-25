#!/bin/bash
set -euo pipefail
# Equivalente mac de compilar_release.bat: compila en Release, en el arbol build-release/.
#
# Wrapper de dos lineas: no configura nada por su cuenta, solo forwardea al motor de
# desarrollo con --release. Uso normal: correrlo a mano para probar un Release sin pasar
# por deploy.sh, que compila directo contra ./compilar.sh --release --no-run.
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
./compilar.sh --release "$@"
