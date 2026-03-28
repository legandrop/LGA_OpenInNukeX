> **Regla de documentacion**: este archivo describe el estado actual del codigo. No es un historial de cambios, changelog ni bitacora temporal.
> **Regla de documentacion**: este archivo debe incluir una seccion de referencias tecnicas con rutas completas a los archivos mas importantes relacionados, y para cada archivo nombrar las funciones, clases o metodos clave vinculados a este tema.

# Google Doc a Markdown (LGA_OpenInNukeX)

## Objetivo

Dejar documentado el flujo actual para convertir el Google Doc exportado como `.docx` de `LGA_OpenInNukeX` a un `README.md`, manteniendo el material fuente y la conversión base dentro de `docs/DocToMD`.

## Estado actual

En este proyecto ahora existen:

- [LGA_OpenInNukeX.docx](/Users/leg4/.nuke/LGA_OpenInNukeX/docs/DocToMD/LGA_OpenInNukeX.docx): copia local del export del documento.
- [README.md](/Users/leg4/.nuke/LGA_OpenInNukeX/README.md): README final de raíz.
- [README.md](/Users/leg4/.nuke/LGA_OpenInNukeX/docs/DocToMD/README.md): salida base generada por script.
- [convert_docx_to_md.py](/Users/leg4/.nuke/LGA_OpenInNukeX/docs/DocToMD/convert_docx_to_md.py): script para regenerar la base.
- [Doc_Media/Originals](/Users/leg4/.nuke/LGA_OpenInNukeX/Doc_Media/Originals): imágenes originales extraídas del `.docx`.
- [Doc_Media](/Users/leg4/.nuke/LGA_OpenInNukeX/Doc_Media): imágenes que usa el README final.

## Flujo acordado

1. Exportar el Google Doc como `Microsoft Word (.docx)`.
2. Copiarlo a:

`/Users/leg4/.nuke/LGA_OpenInNukeX/docs/DocToMD/LGA_OpenInNukeX.docx`

3. Regenerar la base:

```bash
python3 /Users/leg4/.nuke/LGA_OpenInNukeX/docs/DocToMD/convert_docx_to_md.py
```

4. Revisar la salida preliminar en:

`/Users/leg4/.nuke/LGA_OpenInNukeX/docs/DocToMD/README.md`

5. Si la versión final cambia, actualizar el `README.md` de raíz y la línea visual de versión.

## Criterio editorial usado

- Se mantiene la misma cabecera HTML usada en `LGA_ToolPack` y `LGA_ToolPack-Layout`: logo a la izquierda, nombre del pack y línea `Lega | vX.XX`.
- La sección `Instalación` conserva el formato visual de los otros packs, adaptando solamente el nombre de carpeta y la línea `pluginAddPath`.
- La media del `.docx` se preserva primero en `Doc_Media/Originals` y luego se reutiliza desde `Doc_Media` para el README final.
- El `.docx` de origen puede quedar desactualizado respecto de la versión publicada. En este caso, el contenido base venía con `OpenInNukeX v1.52`, pero el `README.md` final quedó ajustado a la versión acordada `1.65`.
- Como el documento fuente es corto, el script genera una base preliminar y el README final se termina de curar con la documentación vigente del proyecto para reflejar el estado multiplataforma actual.

## Referencias técnicas

- [README_Qt.md](/Users/leg4/.nuke/LGA_OpenInNukeX/QtClient/README_Qt.md): detalle técnico del cliente Qt/C++.
- [init.py](/Users/leg4/.nuke/LGA_OpenInNukeX/init.py): servidor TCP dentro de NukeX.
- [LGA_OpenInNukeX.md](/Users/leg4/.nuke/LGA_OpenInNukeX/LGA_OpenInNukeX.md): documentación general previa, usada como apoyo para el README final.
- [README_OLD.md](/Users/leg4/.nuke/LGA_OpenInNukeX/README_OLD.md): README anterior conservado como referencia histórica.
