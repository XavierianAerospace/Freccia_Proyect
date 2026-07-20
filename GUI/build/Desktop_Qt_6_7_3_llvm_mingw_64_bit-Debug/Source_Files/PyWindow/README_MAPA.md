# FRECCIA XAE - Mapa 3D Cesium + OpenFreeMap

## Arquitectura

```
Vector MBTiles (OpenFreeMap)
         ↓
TileServer-GL (renderiza vector → raster PNG)
         ↓
Cesium Globe (consumidor de tiles offline)
         ↓
PyQt webview (interfaz)
```

## ¿Por qué esta arquitectura?

- **MBTiles Vector**: Archivo comprimido de 70GB con datos cartográficos vectoriales
- **TileServer-GL**: Renderiza dinámicamente los vectores a PNG raster en tiempo real
- **Cesium**: Necesita tiles raster (PNG), no vectores
- **Resultado**: Mapa offline profesional, escalable, sin dependencias externas

## Requisitos

✅ **Instalados automáticamente:**
- Node.js v24.15.0+
- TileServer-GL (npm install -g tileserver-gl)

## Estructura

```
PyWindow/
├── main.py                 # Inicia TODO automáticamente
├── start_tileserver.bat    # Script manual para TileServer-GL (opcional)
├── maps/                   # 📁 Carpeta donde va el mbtiles
│   └── maptiler-osm-2020-02-10-v3.11-planet.mbtiles
├── backend/
│   ├── server.py
│   └── mbtiles_server.py
├── cesium_app/
│   ├── static/
│   │   ├── Cesium/
│   │   ├── script.js       # ⭐ Consumidor de TileServer-GL
│   │   └── ...
│   └── templates/
│       └── index.html
```

## Instalación (PRIMER USO SOLAMENTE)

### 1. Asegurar que Node.js está instalado

```bash
node --version
# v24.15.0
```

### 2. Instalar TileServer-GL (una sola vez)

```bash
npm install -g tileserver-gl
```

### 3. Copiar el archivo mbtiles

```bash
# Desde PowerShell:
Copy-Item -Path 'C:\Users\santi\Desktop\Mapa\maptiler-osm-2020-02-10-v3.11-planet.mbtiles' `
          -Destination '.\maps\'
```

O simplemente copiar el archivo manualmente a `PyWindow/maps/`

### 4. ✅ Listo

## Ejecutar

```bash
cd GUI/Source_Files/PyWindow
python main.py
```

**Automáticamente hará:**

1. ✅ Inicia TileServer-GL en puerto 8080
2. ✅ Inicia Flask en puerto 5001  
3. ✅ Abre ventana Cesium
4. ✅ Cesium pide tiles a TileServer-GL
5. ✅ Mapa completo renderizado

## URLs disponibles

- **Cesium App**: http://127.0.0.1:5001
- **TileServer-GL Admin**: http://127.0.0.1:8080
- **Tiles raster**: http://127.0.0.1:8080/styles/basic-preview/{z}/{x}/{y}.png

## Verificación

### ¿Funciona TileServer-GL?

```bash
# Abre en navegador:
http://127.0.0.1:8080
```

Deberías ver:
- ✅ Lista de estilos
- ✅ Vista previa del mapa
- ✅ Zoom funcionando
- ✅ Descarga de tiles

### ¿Funciona Cesium?

```bash
# Abre en navegador:
http://127.0.0.1:5001
```

Deberías ver:
- ✅ Globo terráqueo con mapa renderizado
- ✅ Tiles de OpenFreeMap
- ✅ Zoom y navegación
- ✅ **SIN conexión a internet**

## Próximos pasos

Ahora que el mapa funciona, puedes agregar:

- ✅ TCP telemetría (tracking en tiempo real)
- ✅ Entidades personalizadas (cohete, drones, etc.)
- ✅ Rutas de vuelo
- ✅ Overlays de datos
- ✅ Cámaras personalizadas
- ✅ GIS layers

Todo sin cambiar la arquitectura base.

## Troubleshooting

### "tileserver-gl: command not found"

```bash
npm install -g tileserver-gl
```

### "Tiles not found"

Verifica que el archivo existe:

```bash
ls -la maps/
```

### "Port 8080 already in use"

```bash
# Cambiar puerto en script.js y main.py
# Buscar: 8080 → 8081
```

### "MBTiles file is locked"

No tengas la BD abierta en otro programa (SQLite).

## Arquitectura profesional

Esta solución es similar a:

- ✅ Google Maps offline
- ✅ Mapbox GL offline
- ✅ ArcGIS field apps
- ✅ Servidores GIS modernos

Y funciona en:
- ✅ Windows / Linux / Mac
- ✅ Local network
- ✅ Docker
- ✅ Compilado a .exe

---

**Creado para FRECCIA_XAE**  
OpenFreeMap + TileServer-GL + Cesium.js + PyQt6
