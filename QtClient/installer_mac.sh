#!/bin/bash
set -euo pipefail

# Instalador del componente de Nuke (el "Nuke Bridge") en macOS.
#
# Quien lo invoca y con que contrato
# ----------------------------------
# Este archivo viaja DENTRO del zip del release y es el entry point que busca el card de LGA
# Updates de PipeSync. Su `UpdateRunner::locateEngine()` busca, recursivamente dentro del zip
# ya extraido, un archivo llamado EXACTAMENTE `installer_mac.sh` y lo corre asi:
#
#     /bin/bash <ruta>/installer_mac.sh --non-interactive --nuke-dir <carpeta .nuke>
#
# con el working directory en la carpeta del script. Cambiarle el nombre, o mover el payload
# a otro lado relativo a el, deja a PipeSync sin poder instalar esta herramienta — y el error
# aparece del lado de PipeSync, no de aca.
#
# Hace lo mismo que el boton INSTALL de la app (`NukeBridge::install`), y por las mismas
# razones: copia el payload, se niega a pisar el repositorio fuente, y APPENDEA la linea al
# `init.py` del usuario si falta. Nunca lo reescribe: ese archivo puede tener toda la
# configuracion de Nuke de esa persona.
#
# Que NO hace, a diferencia del boton: no valida que el `init.py` resultante sea Python
# valido. Esa validacion la tiene el instalador transaccional del plugin (ver ChangeLog
# v1.76); aca el riesgo es menor porque solo appendea una linea en columna 0.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

PLUGIN_FOLDER="LGA_OpenInNukeX"
NUKE_DIR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --nuke-dir)
            if [[ $# -lt 2 ]]; then echo "ERROR: --nuke-dir requiere un valor."; exit 1; fi
            NUKE_DIR="$2"; shift 2 ;;
        # Se acepta porque es parte del contrato con PipeSync, pero no cambia nada: este
        # script NUNCA pregunta. La bandera existe para que el motor pueda pasarla sin que el
        # script muera con "opcion desconocida", no para habilitar un modo distinto.
        --non-interactive) shift ;;
        -h|--help)
            echo "Uso: $0 [--nuke-dir <carpeta .nuke>] [--non-interactive]"
            exit 0 ;;
        *) echo "Opcion desconocida: $1"; exit 1 ;;
    esac
done

if [[ -z "$NUKE_DIR" ]]; then
    NUKE_DIR="$HOME/.nuke"
fi

if [[ ! -d "$NUKE_DIR" ]]; then
    # Se crea en vez de fallar: una instalacion limpia de Nuke puede no tener la carpeta
    # todavia, y el instalador no tiene por que ser el que exija abrir Nuke antes.
    mkdir -p "$NUKE_DIR" || { echo "ERROR: no se pudo crear $NUKE_DIR"; exit 1; }
fi

SRC_DIR="$SCRIPT_DIR/$PLUGIN_FOLDER"
if [[ ! -d "$SRC_DIR" ]]; then
    echo "ERROR: no se encontro el payload '$PLUGIN_FOLDER' junto a este script."
    echo "       script=$SCRIPT_DIR"
    exit 1
fi
for required in init.py LGA_QtAdapter_OpenInNukeX.py VERSION; do
    if [[ ! -f "$SRC_DIR/$required" ]]; then
        echo "ERROR: al payload le falta $required"
        exit 1
    fi
done

DEST_DIR="$NUKE_DIR/$PLUGIN_FOLDER"

# El MISMO guard que tiene el boton INSTALL de la app (`looksLikeSourceRepo` en
# src/nukebridge.cpp). No es defensivo de mas: en una maquina de desarrollo el repo VIVE en
# `~/.nuke/LGA_OpenInNukeX/`, que es exactamente esta carpeta destino. Sin el chequeo, correr
# este script —a mano o desde el card de LGA Updates de PipeSync, que lo invoca con
# `--nuke-dir ~/.nuke`— sobrescribe `init.py`, `LGA_QtAdapter_OpenInNukeX.py` y `VERSION`
# versionados con la copia empaquetada, y se pierde todo lo que no este commiteado.
for candidate in "$DEST_DIR" "$NUKE_DIR"; do
    if [[ -f "$candidate/QtClient/CMakeLists.txt" ]]; then
        echo "ERROR: '$candidate' es el repositorio fuente de OpenInNukeX, no una instalacion."
        echo "       Instalar ahi pisaria los archivos fuente. Elegi otra carpeta .nuke."
        exit 1
    fi
done

mkdir -p "$DEST_DIR"

# ditto y no cp -R: conserva permisos y metadata, y no deja el destino a medias si algo falla.
ditto "$SRC_DIR" "$DEST_DIR"
echo "Payload instalado en $DEST_DIR"

INIT_PY="$NUKE_DIR/init.py"
PLUGIN_LINE="nuke.pluginAddPath('./${PLUGIN_FOLDER}')"

# El mismo criterio que usa la app (`hasActivePluginPathLine` en src/nukebridge.cpp): alcanza
# con que Nuke vaya a cargar esa carpeta, sin importar si esta escrita con comillas simples o
# dobles, con o sin "./".
#
# El `^[[:space:]]*[^#[:space:]]` del arranque SALTEA LAS LINEAS COMENTADAS, y no es un
# detalle: comentar la linea es el gesto natural para desactivar el bridge un rato. Sin eso,
# un `# nuke.pluginAddPath('./LGA_OpenInNukeX')` daba positivo y el instalador informaba exito
# sin agregar nada, con Nuke sin cargar el plugin.
if [[ -f "$INIT_PY" ]] && grep -Eq "^[[:space:]]*[^#[:space:]].*pluginAddPath[[:space:]]*\([[:space:]]*['\"][^'\"]*${PLUGIN_FOLDER}['\"]" "$INIT_PY"; then
    echo "El init.py ya carga $PLUGIN_FOLDER; no se toca."
else
    # Si el archivo existe y no termina en newline, la linea nueva quedaria pegada a la
    # ultima y Python fallaria al parsear el init.py entero.
    if [[ -f "$INIT_PY" && -s "$INIT_PY" && -n "$(tail -c 1 "$INIT_PY")" ]]; then
        printf '\n' >> "$INIT_PY"
    fi
    printf '\n# LGA OpenInNukeX\n%s\n' "$PLUGIN_LINE" >> "$INIT_PY"
    echo "Linea agregada a $INIT_PY"
fi

echo "OK: Nuke Bridge $(tr -d '\r\n' < "$DEST_DIR/VERSION") instalado. Reinicia NukeX."
exit 0
