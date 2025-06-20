# Changelog UI - LGA_OpenInNukeX

## v0.15 - Rediseño de Interfaz de Usuario (2024)

### ✨ Nuevas Características

#### Interfaz Moderna
- **Tema oscuro completo** con colores consistentes del ecosistema LGA
- **Tipografía Inter** para mejor legibilidad y aspecto profesional
- **Diseño responsive** con scroll automático para contenido largo
- **Arquitectura de dos secciones** organizadas en tarjetas

#### Secciones Reorganizadas
1. **File Association**
   - Texto descriptivo claro sobre la funcionalidad
   - Botón "APPLY" prominente para establecer asociaciones
   
2. **Preferred Nuke Version**
   - Campo de entrada con etiqueta flotante
   - Botón "BROWSE" integrado para selección de archivo
   - Botón "SAVE" destacado para guardar configuración

#### Paleta de Colores
- **Fondo principal**: `#161616` (bg_principal)
- **Elementos UI**: `#1d1d1d` (bg_items)
- **Texto principal**: `#B2B2B2` (txt_principal)
- **Bordes**: `#303030` (border_principal)
- **Botón de acción**: `#443a91` (violeta_oscuro)
- **Botones secundarios**: `#3b3b3b`

### 🔧 Mejoras Técnicas

#### Arquitectura de Estilos
- **Archivo QSS separado** (`dark_theme.qss`) para fácil mantenimiento
- **Carga automática de estilos** desde el directorio de la aplicación
- **Fallback integrado** en caso de archivo QSS faltante

#### Layout Responsive
- **QScrollArea** para contenido que exceda el tamaño de ventana
- **Centrado automático** del contenido en ventanas grandes
- **Ancho fijo** para consistencia visual (700px)

#### Deploy Mejorado
- **Inclusión automática** del archivo QSS en el paquete de deploy
- **Verificación de archivos** durante el proceso de empaquetado
- **Script actualizado** para copiar todos los recursos necesarios

### 📁 Archivos Modificados

#### Nuevos Archivos
- `QtClient/dark_theme.qss` - Archivo de estilo principal
- `QtClient/CHANGELOG_UI.md` - Este archivo de changelog

#### Archivos Modificados
- `QtClient/src/configwindow.cpp` - Rediseño completo de la UI
- `QtClient/src/configwindow.h` - Agregada función `loadStyleSheet()`
- `QtClient/scripts/deploy.bat` - Inclusión del archivo QSS en deploy
- `LGA_OpenInNukeX.md` - Documentación actualizada

### 🎨 Inspiración de Diseño

La nueva interfaz está basada en el diseño de UserSettingsTab de otra aplicación del ecosistema LGA, garantizando:
- **Consistencia visual** entre aplicaciones
- **Experiencia de usuario familiar** para usuarios del ecosistema
- **Estándares de diseño moderno** con tipografía profesional

### 🔄 Compatibilidad

- **Totalmente compatible** con versiones anteriores
- **Misma funcionalidad** con interfaz mejorada
- **Archivos de configuración** sin cambios
- **Scripts de deploy** retrocompatibles

### 📋 Próximos Pasos

- [ ] Añadir animaciones suaves para transiciones
- [ ] Implementar tooltips informativos
- [ ] Optimizar responsividad para diferentes tamaños de pantalla
- [ ] Considerar modo claro como opción alternativa 