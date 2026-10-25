CEMU LOW-END - ARCHIVOS PARA COPIAR

1) GeneralSettings2.cpp
   Ruta en el repositorio: src/gui/wxgui/GeneralSettings2.cpp
   Reemplaza el archivo existente por este.

2) lowend-windows-x64.yml
   Ruta en el repositorio: .github/workflows/lowend-windows-x64.yml
   Crea las carpetas .github/workflows si no existen y coloca el archivo ahí.

3) Luego abre GitHub > Actions > Cemu Low-End - Windows x64 > Run workflow.

IMPORTANTE:
- El botón Apply Low-End preset cambia valores de configuración existentes en la interfaz, pero no se ha compilado ni probado aquí.
- El selector de resolución 720p/480p/320p/240p es solo visual en esta versión. NO cambia la resolución interna del juego todavía.
- El workflow es un intento de compilación, no verificado; si falla, comparte el log completo.
