#!/bin/bash
set -euo pipefail
# Equivalente mac de compilar.bat: compila en Release, en el arbol build-release/.
#
# Antes este script tenia su propia copia de toda la logica de compilacion —con
# `-DCMAKE_BUILD_TYPE=Debug` adentro, o sea que el script "de release" compilaba Debug— y
# lanzaba la app en foreground, reteniendo la terminal. Ahora forwardea al unico script que
# sabe compilar, que es el que tiene los dos arboles y los flags de la convencion LGA.
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
./compilar_dev.sh --release "$@"
