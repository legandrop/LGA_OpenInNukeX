# LGA_WinSetFTA

Helper de consola para Windows que escribe asociaciones de archivos con hash
`UserChoice` y `UserChoiceLatest` (Windows 11). Codigo derivado de
[PS-SFTA](https://github.com/DanysysTeam/PS-SFTA) y [DefaultApps](https://github.com/araghon007/DefaultApps) (MIT).

No se ejecuta directamente por el usuario: `LGA_OpenInNukeX.exe` lo invoca al pulsar
**APPLY** en la ventana de configuración (asociación de `.nk`).

También lo usa LGA Shot Player (`LGA_MediaTools_v2`) para asociaciones de extensiones del player.

Build:

```bat
tools\win_file_assoc\build_win_setfta.bat
```

Salida: `build\LGA_WinSetFTA.exe` + `build\LookUpLut4.bin` (deben copiarse junto al player).
