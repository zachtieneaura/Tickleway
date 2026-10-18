# Tick Leeway (Geode)

Mod educativo/local para Geometry Dash 2.2081 (Geode 5.10.1): input buffer y
tolerancia de ticks (coyote experimental), menu ImGui (F6) e indicador en pantalla.

> Usalo solo en local. No lo uses para records ni envios a listas competitivas.

## Compilar con GitHub Actions
1. Sube la carpeta a un repositorio de GitHub.
2. Ve a **Actions > Build Geode Mod** (se ejecuta en cada push o manualmente).
3. Descarga el artefacto **Build Output**: contiene el `.geode`.
4. Copia el `.geode` a `Geometry Dash/geode/mods/`.

## Compilar en local (Windows)
1. Instala Geode CLI y el SDK, y define `GEODE_SDK`.
2. `geode build` dentro de esta carpeta.

## Notas
- Los nombres de campos (`m_gameState.m_currentProgress`, `m_isShip`, etc.) y la firma de
  `dispatchKeyboardMSG` dependen de los bindings; ajustalos si el compilador se queja.
- `sdk: latest` en `.github/workflows/build.yml` puede cambiarse a una version fija.
