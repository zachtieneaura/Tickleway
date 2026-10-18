# Tick Leeway (Geode)

Mod educativo/local para Geometry Dash 2.2081 (Geode 5.11.x): input buffer y
tolerancia de ticks (coyote experimental), panel en pantalla e indicador (HUD).

> Usalo solo en local. No lo uses para records ni envios a listas competitivas.

## Teclas
- **F6**: mostrar/ocultar el panel (botones ON/OFF, - y +)
- **F7**: activar/desactivar el mod
- **F8 / F9**: subir / bajar los ticks de buffer (0 a 8)

El texto de arriba a la izquierda se puede ocultar en los ajustes del mod ("Mostrar indicador").

## Compilar con GitHub Actions
1. Sube la carpeta a un repositorio de GitHub (crea `.github/workflows/build.yml` desde la web si no se sube).
2. Ve a **Actions > Build Geode Mod**.
3. Descarga el artefacto **Build Output** (contiene el `.geode`).
4. Copialo a `Geometry Dash/geode/mods/`.

## Notas
- El `mod.json` debe coincidir con la version del SDK que use el workflow (`sdk: latest`).
- Se quito ImGui: la libreria `gd-imgui-cocos` (rama main) no compila con los bindings de 2.2081.
