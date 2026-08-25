#!/bin/bash

# El script vive en la raiz del repo, pero build/ y deploy/ siguen en QtClient/.
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")/QtClient" && pwd)"

echo "🧹 Limpiando build y deploy..."
rm -rf build
rm -rf deploy
echo "✅ Listo"
