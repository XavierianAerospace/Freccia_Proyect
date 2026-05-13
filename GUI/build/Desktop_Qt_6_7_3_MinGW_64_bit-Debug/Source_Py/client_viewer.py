import sys
import os
import socket
import threading
import tempfile
from pathlib import Path

# Qt6
from PyQt6.QtWidgets import (
    QApplication, QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton, QFileDialog
)
from PyQt6.QtCore import pyqtSignal, QObject, QUrl, Qt
from PyQt6.QtGui import QCloseEvent, QIcon

# Web engine
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebEngineCore import QWebEngineProfile, QWebEnginePermission, QWebEngineSettings

# 3D viewer
import numpy as np
import pyqtgraph as pg
import pyqtgraph.opengl as gl

_trimesh = None
_npstl = None
try:
    import trimesh as _trimesh
except Exception:
    _trimesh = None
try:
    from stl import mesh as npstl_mesh
    _npstl = npstl_mesh
except Exception:
    _npstl = None

def resource_path(relative_path):
    """Obtiene la ruta absoluta para recursos, funciona para desarrollo y para PyInstaller"""
    try:
        # PyInstaller crea una carpeta temporal y almacena la ruta en _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    
    return os.path.join(base_path, relative_path)

def get_logo_paths():
    """Obtiene las rutas de logos compatibles con PyInstaller"""
    paths = []
    
    if hasattr(sys, '_MEIPASS'):
        # Estamos en el ejecutable de PyInstaller
        base_dir = Path(sys._MEIPASS)
        paths.extend([
            base_dir / "Map / assets" / "logo_xae.png",
            base_dir / "logo_xae.png"
        ])
    else:
        # Estamos en desarrollo
        base_dir = Path(__file__).resolve().parent
        paths.extend([
            base_dir / "assets" / "logo_xae.png",
            base_dir / ".." / "assets" / "logo_xae.png",
            Path("Map/assets/logo_xae.png")
        ])
    
    return paths

class DataSignals(QObject):
    gps_update = pyqtSignal(float, float)
    attitude_update = pyqtSignal(float, float, float)
    status = pyqtSignal(str)

class SensorClientWindow(QWidget):
    def __init__(self, host="127.0.0.1", port=5000, stl_fixed_path: str = None):
        super().__init__()
        self.setWindowTitle("FRECCIA_XAE - MAP - Angle of attack")
        self.resize(1200, 720)

        # CONFIGURAR EL LOGO DE LA VENTANA
        try:
            logo_paths = get_logo_paths()
            logo_set = False
            
            for logo_path in logo_paths:
                if logo_path.exists():
                    self.setWindowIcon(QIcon(str(logo_path)))
                    print(f"DEBUG: Logo de ventana cargado desde: {logo_path}")
                    logo_set = True
                    break

            if not logo_set:
                print("DEBUG: No se encontró el archivo de logo, usando icono por defecto")
                self.setWindowIcon(self._create_default_icon())
                
        except Exception as e:
            print(f"DEBUG: Error cargando icono: {e}")
            self.setWindowIcon(self._create_default_icon())

        # Configurar atributos correctamente
        self.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)

        # Señales internas
        self.signals = DataSignals()
        self.signals.gps_update.connect(self.on_gps_update)
        self.signals.attitude_update.connect(self.on_attitude_update)
        self.signals.status.connect(self.on_status)

        # Estado
        self.current_lat = None
        self.current_lon = None
        self.current_roll = 0.0
        self.current_pitch = 0.0
        self.current_yaw = 0.0

        # Ruta fija del STL
        self.fixed_stl_path = stl_fixed_path or "CoheteGUI.STL"

        # Configurar UI
        self._setup_ui()

        # Configurar WebView con permisos mejorados
        self._configure_webview_permissions()

        # Cargar mapa con manejo robusto
        self._create_enhanced_map()

        # Inicializar escena 3D
        self._init_3d_scene()

        # Cargar STL fijo si existe
        if self.fixed_stl_path and os.path.exists(self.fixed_stl_path):
            self.load_stl(self.fixed_stl_path)
            self.signals.status.emit(f"STL cargado: {Path(self.fixed_stl_path).name}")
        else:
            self.signals.status.emit("STL no encontrado en ruta fija.")

        # Iniciar cliente TCP en hilo
        self.client_thread = threading.Thread(target=self._client_worker, args=(host, port), daemon=True)
        self.client_thread.start()

    def _create_default_icon(self):
        """Crea un icono por defecto si no se encuentra el archivo de icono"""
        from PyQt6.QtGui import QPixmap, QPainter
        from PyQt6.QtCore import QSize
        
        # Crear un pixmap de 64x64
        pixmap = QPixmap(64, 64)
        pixmap.fill(Qt.GlobalColor.blue)  # Fondo azul
        
        return QIcon(pixmap)

    def _setup_ui(self):
        """Configura la interfaz de usuario"""
        main_layout = QHBoxLayout()
        left_col = QVBoxLayout()
        right_col = QVBoxLayout()

        # Top controls/status
        top_controls = QHBoxLayout()
        self.lbl_status = QLabel("Status: desconectado")
        top_controls.addWidget(self.lbl_status)
        top_controls.addStretch(1)
        btn_load = QPushButton("Cargar .stl (otra ruta)")
        btn_load.clicked.connect(self.select_stl)
        top_controls.addWidget(btn_load)
        left_col.addLayout(top_controls)

        # WebEngineView (mapa)
        self.webview = QWebEngineView()
        left_col.addWidget(self.webview, stretch=1)

        # 3D viewer
        self.view3d = gl.GLViewWidget()
        self.view3d.opts['distance'] = 200
        self.view3d.setMinimumWidth(480)
        
        # Configuración adicional para OpenGL
        self.view3d.setCameraPosition(distance=200, elevation=30, azimuth=45)
        right_col.addWidget(self.view3d, stretch=1)

        # Info footer
        info_layout = QVBoxLayout()
        self.lbl_coords = QLabel("Lat: -, Lon: -")
        self.lbl_att = QLabel("Roll: 0.0 Pitch: 0.0 Yaw: 0.0")
        info_layout.addWidget(self.lbl_coords)
        info_layout.addWidget(self.lbl_att)
        right_col.addLayout(info_layout)

        main_layout.addLayout(left_col, stretch=2)
        main_layout.addLayout(right_col, stretch=2)
        self.setLayout(main_layout)

    def _configure_webview_permissions(self):
        """Configura el WebView con permisos mejorados para tiles"""
        try:
            # Configurar el perfil para permitir recursos externos
            profile = self.webview.page().profile()
            
            # Configurar políticas de permisos más permisivas
            profile.setHttpCacheType(QWebEngineProfile.HttpCacheType.DiskHttpCache)
            profile.setPersistentCookiesPolicy(QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies)
            
            # Configurar settings para permitir recursos externos
            settings = self.webview.settings()
            settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls, True)
            settings.setAttribute(QWebEngineSettings.WebAttribute.AllowRunningInsecureContent, True)
            settings.setAttribute(QWebEngineSettings.WebAttribute.AllowWindowActivationFromJavaScript, True)
            settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptCanOpenWindows, True)
            settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptCanAccessClipboard, True)
            
            # Configurar user agent estándar para evitar bloqueos
            profile.setHttpUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36")
            
            print("DEBUG: WebView configurado con permisos mejorados")
            
        except Exception as e:
            print(f"DEBUG: Error configurando permisos WebView: {e}")

    def _create_enhanced_map(self):
        """Crea un mapa mejorado con múltiples estrategias de carga"""
        print("DEBUG: Creando mapa mejorado...")
        
        leaflet_css, leaflet_js = self._get_leaflet_from_map_assets()
        
        # Definir las coordenadas iniciales en Python
        initial_lat = 4.6286
        initial_lon = -74.0647
        
        html = f"""<!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <title>Mapa Telemetría FRECCIA</title>
        {leaflet_css}
        <style>
            body, html {{ 
                margin: 0; 
                padding: 0; 
                height: 100%; 
                font-family: 'Segoe UI', Arial, sans-serif;
                background: #1e1e1e;
                overflow: hidden;
            }}
            #map {{ 
                height: 100vh; 
                width: 100%; 
                background: #2d2d2d;
            }}
            .info-panel {{ 
                position: absolute; 
                top: 20px; 
                right: 20px;
                background: rgba(40, 40, 40, 0.95);
                padding: 20px;
                border-radius: 12px;
                border: 1px solid #555;
                z-index: 1000;
                box-shadow: 0 4px 20px rgba(0,0,0,0.3);
                max-width: 320px;
                backdrop-filter: blur(10px);
                color: white;
            }}
            .coordinates {{
                background: #3d3d3d;
                padding: 12px;
                border-radius: 8px;
                margin-bottom: 15px;
                border-left: 4px solid #007bff;
            }}
            .status {{
                padding: 10px;
                border-radius: 8px;
                font-weight: 500;
                text-align: center;
                margin-bottom: 10px;
            }}
            .status.ready {{
                background: #155724;
                color: #d4edda;
                border: 1px solid #c3e6cb;
            }}
            .status.warning {{
                background: #856404;
                color: #fff3cd;
                border: 1px solid #ffeaa7;
            }}
            .status.error {{
                background: #721c24;
                color: #f8d7da;
                border: 1px solid #f5c6cb;
            }}
            .grid-overlay {{
                position: absolute;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                background: 
                    linear-gradient(90deg, rgba(255,255,255,0.1) 1px, transparent 1px),
                    linear-gradient(0deg, rgba(255,255,255,0.1) 1px, transparent 1px);
                background-size: 50px 50px;
                pointer-events: none;
                z-index: 500;
            }}
            
            /* Estilos para el menú hamburguesa */
            .hamburger-menu {{
                position: absolute;
                top: 80px;  /* Cambiado de 20px a 80px para bajar el menú */
                left: 20px; /* Mantenido en izquierda pero más abajo */
                z-index: 1001;
            }}
            
            .hamburger-btn {{
                background: rgba(40, 40, 40, 0.95);
                border: 1px solid #555;
                border-radius: 8px;
                color: white;
                cursor: pointer;
                font-size: 16px;
                padding: 10px 15px;
                backdrop-filter: blur(10px);
                transition: all 0.3s ease;
            }}
            
            .hamburger-btn:hover {{
                background: rgba(60, 60, 60, 0.95);
                transform: scale(1.05);
            }}
            
            .slide-menu {{
                position: absolute;
                top: 0;
                left: -320px;
                width: 300px;
                height: 100%;
                background: rgba(30, 30, 30, 0.98);
                box-shadow: 2px 0 10px rgba(0,0,0,0.5);
                padding: 20px;
                overflow-y: auto;
                transition: left 0.3s ease;
                z-index: 1000;
                backdrop-filter: blur(10px);
            }}
            
            .slide-menu.open {{
                left: 0;
            }}
            
            .menu-header {{
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 20px;
                padding-bottom: 15px;
                border-bottom: 1px solid #555;
            }}
            
            .menu-header h3 {{
                color: white;
                margin: 0;
            }}
            
            .close-btn {{
                background: none;
                border: none;
                color: white;
                font-size: 24px;
                cursor: pointer;
                padding: 5px;
                border-radius: 4px;
                transition: background 0.3s ease;
            }}
            
            .close-btn:hover {{
                background: rgba(255,255,255,0.1);
            }}
            
            .tile-controls {{
                display: flex;
                flex-direction: column;
                gap: 8px;
            }}
            
            .tile-btn {{
                background: #007bff;
                color: white;
                border: none;
                padding: 12px 15px;
                border-radius: 8px;
                cursor: pointer;
                font-size: 14px;
                text-align: left;
                transition: all 0.3s ease;
                display: flex;
                align-items: center;
                gap: 10px;
            }}
            
            .tile-btn:hover {{
                background: #0056b3;
                transform: translateX(5px);
            }}
            
            .tile-btn:active {{
                transform: translateX(2px);
            }}
            
            .auto-hide-notice {{
                position: absolute;
                bottom: 15px;
                left: 15px;
                right: 15px;
                background: rgba(0, 123, 255, 0.1);
                color: #4dabf7;
                padding: 8px 12px;
                border-radius: 6px;
                font-size: 12px;
                text-align: center;
                border: 1px solid rgba(0, 123, 255, 0.3);
            }}
            
            .menu-overlay {{
                position: fixed;
                top: 0;
                left: 0;
                width: 100%;
                height: 100%;
                background: rgba(0,0,0,0.5);
                z-index: 999;
                opacity: 0;
                visibility: hidden;
                transition: all 0.3s ease;
            }}
            
            .menu-overlay.active {{
                opacity: 1;
                visibility: visible;
            }}
        </style>
    </head>
    <body>
        <div id="map"></div>
        <div class="grid-overlay" id="gridOverlay"></div>
        
        <!-- Overlay para cerrar menú al hacer clic fuera -->
        <div class="menu-overlay" id="menuOverlay" onclick="closeMenu()"></div>
        
        <!-- Menú hamburguesa -->
        <div class="hamburger-menu">
            <button class="hamburger-btn" onclick="toggleMenu()">
                ☰
            </button>
        </div>
        
        <!-- Panel deslizante -->
        <div class="slide-menu" id="slideMenu">
            <div class="menu-header">
                <h3>🗺️ Fuentes de Mapa</h3>
                <button class="close-btn" onclick="closeMenu()">×</button>
            </div>
            
            <div class="tile-controls">
                <button class="tile-btn" onclick="switchTileLayer('osm')">
                    <span style="font-size: 16px;">🗺️</span>
                    <span>OpenStreetMap</span>
                </button>
                <button class="tile-btn" onclick="switchTileLayer('carto')">
                    <span style="font-size: 16px;">🏙️</span>
                    <span>CartoDB Voyager</span>
                </button>
                <button class="tile-btn" onclick="switchTileLayer('opentopo')">
                    <span style="font-size: 16px;">⛰️</span>
                    <span>OpenTopoMap</span>
                </button>
                <button class="tile-btn" onclick="switchTileLayer('stadia')">
                    <span style="font-size: 16px;">🌆</span>
                    <span>Stadia Maps</span>
                </button>
                <button class="tile-btn" onclick="switchTileLayer('cyclosm')">
                    <span style="font-size: 16px;">🚴</span>
                    <span>CyclOSM</span>
                </button>
                <button class="tile-btn" onclick="showGridOnly()">
                    <span style="font-size: 16px;">🔲</span>
                    <span>Solo Cuadrícula</span>
                </button>
            </div>
            
            <div class="auto-hide-notice">
                ⏰ El menú se ocultará automáticamente en <span id="countdown">6</span> segundos
            </div>
        </div>
        
        <div class="info-panel">
            <h3 style="color: white; margin-top: 0;">📍 Posición GPS</h3>
            <div class="coordinates">
                <div>Latitud: <strong id="lat">{initial_lat:.6f}</strong></div>
                <div>Longitud: <strong id="lng">{initial_lon:.6f}</strong></div>
                <div id="accuracy" style="color: #adb5bd;">Ubicación: Javeriana</div>
            </div>
            
            <div class="status warning" id="status">
                🔄 Inicializando mapa...
            </div>
        </div>

        {leaflet_js}
        <script>
            let map = null;
            let marker = null;
            let currentTileLayer = null;
            const userLocation = {{ lat: {initial_lat}, lng: {initial_lon} }};
            let menuTimeout = null;
            let countdownInterval = null;
            let countdownValue = 6;

            // Definir múltiples fuentes de tiles
            const tileSources = {{
                'osm': {{
                    name: 'OpenStreetMap',
                    url: 'https://tile.openstreetmap.org/{{z}}/{{x}}/{{y}}.png',
                    options: {{
                        attribution: '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
                        maxZoom: 19
                    }}
                }},
                'carto': {{
                    name: 'CartoDB Voyager',
                    url: 'https://{{s}}.basemaps.cartocdn.com/rastertiles/voyager/{{z}}/{{x}}/{{y}}.png',
                    options: {{
                        attribution: '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> © <a href="https://carto.com/attributions">CARTO</a>',
                        maxZoom: 20,
                        subdomains: 'abcd'
                    }}
                }},
                'opentopo': {{
                    name: 'OpenTopoMap',
                    url: 'https://{{s}}.tile.opentopomap.org/{{z}}/{{x}}/{{y}}.png',
                    options: {{
                        attribution: '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
                        maxZoom: 17,
                        subdomains: 'abc'
                    }}
                }},
                'stadia': {{
                    name: 'Stadia Maps',
                    url: 'https://tiles.stadiamaps.com/tiles/alidade_smooth/{{z}}/{{x}}/{{y}}.png',
                    options: {{
                        attribution: '© <a href="https://stadiamaps.com/">Stadia Maps</a> © <a href="https://openmaptiles.org/">OpenMapTiles</a> © <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
                        maxZoom: 20
                    }}
                }},
                'cyclosm': {{
                    name: 'CyclOSM',
                    url: 'https://{{s}}.tile-cyclosm.openstreetmap.fr/cyclosm/{{z}}/{{x}}/{{y}}.png',
                    options: {{
                        attribution: '© <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>',
                        maxZoom: 20,
                        subdomains: 'abc'
                    }}
                }}
            }};

            function toggleMenu() {{
                const slideMenu = document.getElementById('slideMenu');
                const menuOverlay = document.getElementById('menuOverlay');
                
                if (slideMenu.classList.contains('open')) {{
                    closeMenu();
                }} else {{
                    openMenu();
                }}
            }}

            function openMenu() {{
                const slideMenu = document.getElementById('slideMenu');
                const menuOverlay = document.getElementById('menuOverlay');
                
                slideMenu.classList.add('open');
                menuOverlay.classList.add('active');
                
                // Iniciar cuenta regresiva
                startCountdown();
                
                // Configurar auto-ocultar después de 6 segundos
                if (menuTimeout) {{
                    clearTimeout(menuTimeout);
                }}
                menuTimeout = setTimeout(closeMenu, 6000);
            }}

            function closeMenu() {{
                const slideMenu = document.getElementById('slideMenu');
                const menuOverlay = document.getElementById('menuOverlay');
                
                slideMenu.classList.remove('open');
                menuOverlay.classList.remove('active');
                
                // Limpiar timeouts
                if (menuTimeout) {{
                    clearTimeout(menuTimeout);
                    menuTimeout = null;
                }}
                if (countdownInterval) {{
                    clearInterval(countdownInterval);
                    countdownInterval = null;
                }}
            }}

            function startCountdown() {{
                countdownValue = 6;
                const countdownElement = document.getElementById('countdown');
                
                if (countdownInterval) {{
                    clearInterval(countdownInterval);
                }}
                
                countdownElement.textContent = countdownValue;
                
                countdownInterval = setInterval(() => {{
                    countdownValue--;
                    countdownElement.textContent = countdownValue;
                    
                    if (countdownValue <= 0) {{
                        clearInterval(countdownInterval);
                        countdownInterval = null;
                    }}
                }}, 1000);
            }}

            function initMap() {{
                console.log("Inicializando mapa mejorado en:", userLocation);
                
                if (typeof L === 'undefined') {{
                    console.error("ERROR: Leaflet no está definido");
                    showMapError("Leaflet no se cargó correctamente");
                    return false;
                }}
                
                try {{
                    // Crear mapa centrado en la ubicación Javeriana
                    map = L.map('map', {{
                        center: [userLocation.lat, userLocation.lng],
                        zoom: 15,
                        zoomControl: true,
                        attributionControl: true
                    }});
                    
                    // Añadir controles de zoom
                    L.control.zoom({{ position: 'topright' }}).addTo(map);
                    
                    // Intentar cargar la primera fuente de tiles
                    setTimeout(() => {{
                        switchTileLayer('carto'); // Empezar con CartoDB que suele ser más confiable
                    }}, 100);
                    
                    // Crear marcador personalizado
                    createMarker();
                    
                    console.log("Mapa Leaflet inicializado correctamente");
                    return true;
                    
                }} catch (error) {{
                    console.error("Error al inicializar el mapa Leaflet:", error);
                    showMapError("Error técnico: " + error.message);
                    return false;
                }}
            }}
            
            function createMarker() {{
                // Intentar cargar icono personalizado, fallback a marcador estándar
                let customIcon;
                try {{
                    customIcon = L.icon({{
                        iconUrl: 'Map / assets / Icon.png',
                        iconSize: [40, 40],
                        iconAnchor: [20, 20],
                        popupAnchor: [0, -20]
                    }});
                }} catch (e) {{
                    console.warn("No se pudo cargar icono personalizado, usando marcador estándar");
                    customIcon = null;
                }}
                
                marker = L.marker([userLocation.lat, userLocation.lng], {{
                    icon: customIcon,
                    draggable: false
                }}).addTo(map);
                
                marker.bindPopup(`<div style="text-align: center;">
                    <b>📍 Edificio de Ingeniería</b><br>
                    <b>Universidad Javeriana</b><br>
                    Lat: {initial_lat:.6f}<br>
                    Lng: {initial_lon:.6f}<br>
                    <em style="color: #666;">Sistema de telemetría FRECCIA</em>
                </div>`).openPopup();
            }}
            
            function switchTileLayer(sourceKey) {{
                if (!map) return;
                
                const source = tileSources[sourceKey];
                if (!source) {{
                    console.error("Fuente de tiles no encontrada:", sourceKey);
                    return;
                }}
                
                console.log("Cambiando a fuente:", source.name);
                
                // Remover capa actual si existe
                if (currentTileLayer) {{
                    map.removeLayer(currentTileLayer);
                }}
                
                // Actualizar estado
                document.getElementById('status').className = 'status warning';
                document.getElementById('status').textContent = '🔄 Cargando ' + source.name + '...';
                
                // Crear nueva capa con manejo de errores
                const newLayer = L.tileLayer(source.url, source.options);
                
                // Configurar manejadores de eventos
                newLayer.on('load', function() {{
                    console.log(source.name + ' cargado exitosamente');
                    document.getElementById('status').className = 'status ready';
                    document.getElementById('status').textContent = source.name;
                    currentTileLayer = newLayer;
                }});
                
                newLayer.on('tileerror', function(e) {{
                    console.error('Error cargando tile en ' + source.name + ':', e);
                    document.getElementById('status').className = 'status error';
                    document.getElementById('status').textContent = '❌ Error en ' + source.name;
                    
                    // Intentar siguiente fuente después de 2 segundos
                    setTimeout(() => {{
                        const sources = Object.keys(tileSources);
                        const currentIndex = sources.indexOf(sourceKey);
                        const nextIndex = (currentIndex + 1) % sources.length;
                        switchTileLayer(sources[nextIndex]);
                    }}, 2000);
                }});
                
                // Añadir la nueva capa al mapa
                newLayer.addTo(map);
                
                // Cerrar el menú después de seleccionar una fuente
                closeMenu();
            }}
            
            function showGridOnly() {{
                if (!map) return;
                
                // Remover capa actual si existe
                if (currentTileLayer) {{
                    map.removeLayer(currentTileLayer);
                    currentTileLayer = null;
                }}
                
                document.getElementById('status').className = 'status ready';
                document.getElementById('status').textContent = 'Modo cuadrícula';
                
                console.log("Modo cuadrícula activado");
                
                // Cerrar el menú después de seleccionar una fuente
                closeMenu();
            }}
            
            function showMapError(message) {{
                document.getElementById('status').className = 'status error';
                document.getElementById('status').textContent = '❌ ' + (message || 'Error cargando mapa');
            }}

            // Función para actualizar la posición desde Python
            window.updatePosition = function(lat, lng, accuracy = null) {{
                if (marker && map) {{
                    console.log("Actualizando posición desde Python:", lat, lng);
                    
                    // Actualizar marcador
                    const newLatLng = [lat, lng];
                    marker.setLatLng(newLatLng);
                    
                    // Suavizar el movimiento del mapa
                    map.panTo(newLatLng, {{
                        animate: true,
                        duration: 1.0
                    }});
                    
                    // Actualizar información
                    document.getElementById('lat').textContent = lat.toFixed(6);
                    document.getElementById('lng').textContent = lng.toFixed(6);
                    
                    if (accuracy) {{
                        document.getElementById('accuracy').textContent = 
                            "Precisión: ±" + Math.round(accuracy) + " metros";
                    }} else {{
                        document.getElementById('accuracy').textContent = "Datos en tiempo real";
                    }}
                    
                    // Actualizar estado y popup
                    document.getElementById('status').className = 'status ready';
                    document.getElementById('status').textContent = 'Datos en tiempo real';
                    
                    marker.bindPopup(`<div style="text-align: center;">
                        <b>🚀 Posición Actualizada</b><br>
                        Lat: ${{lat.toFixed(6)}}<br>
                        Lng: ${{lng.toFixed(6)}}<br>
                        <em style="color: #666;">Datos en tiempo real</em>
                    </div>`).openPopup();
                    
                }} else {{
                    console.warn("Mapa o marcador no disponible para actualización");
                }}
            }};
            
            // Inicializar cuando se cargue la página
            document.addEventListener('DOMContentLoaded', function() {{
                console.log("DOM cargado - Inicializando mapa mejorado");
                setTimeout(initMap, 100);
            }});
        </script>
    </body>
    </html>"""
        try:
            tmp = tempfile.NamedTemporaryFile(mode='w', delete=False, suffix=".html", encoding='utf-8')
            tmp.write(html)
            tmp.flush()
            tmp.close()
            file_url = QUrl.fromLocalFile(str(tmp.name))
            
            if hasattr(self, 'webview') and self.webview:
                self.webview.load(file_url)
                self._fallback_html_file = tmp.name
                self.signals.status.emit("Mapa mejorado cargado")
                print(f"DEBUG: Mapa mejorado creado en: {tmp.name}")
                
                # Conectar señales básicas
                self._connect_basic_webview_signals()
            else:
                print("DEBUG: WebView no disponible para cargar el mapa")
                
        except Exception as e:
            error_msg = f"Error creando mapa mejorado: {e}"
            self.signals.status.emit(error_msg)
            print(f"DEBUG: {error_msg}")

    def _get_leaflet_from_map_assets(self):
        """Busca recursos de Leaflet en la carpeta Map/assets/"""
        try:
            if hasattr(sys, '_MEIPASS'):
                base_dir = Path(sys._MEIPASS)
            else:
                base_dir = Path(__file__).resolve().parent
                
            map_assets_dir = base_dir / "Map" / "assets"
            
            print(f"DEBUG: Buscando Leaflet en: {map_assets_dir}")
            
            css_files = list(map_assets_dir.glob("*leaflet*.css"))
            js_files = list(map_assets_dir.glob("*leaflet*.js"))
            
            if css_files and js_files:
                css_tag = f'<link rel="stylesheet" href="{css_files[0].as_uri()}" />'
                js_tag = f'<script src="{js_files[0].as_uri()}"></script>'
                print("DEBUG: Usando Leaflet desde assets locales")
                return css_tag, js_tag
                
        except Exception as e:
            print(f"DEBUG: Error buscando Leaflet en Map/assets: {e}")
        
        # Fallback a CDN
        print("DEBUG: Usando CDN para Leaflet")
        css_tag = '<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />'
        js_tag = '<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>'
        return css_tag, js_tag

    # ... (el resto de los métodos se mantienen igual)

    def _connect_basic_webview_signals(self):
        """Conecta solo las señales básicas para evitar errores"""
        try:
            if hasattr(self.webview, 'loadStarted'):
                self.webview.loadStarted.connect(lambda: print("DEBUG: WebView - Carga iniciada"))
            
            if hasattr(self.webview, 'loadProgress'):
                self.webview.loadProgress.connect(lambda p: print(f"DEBUG: WebView - Progreso: {p}%"))
            
            if hasattr(self.webview, 'loadFinished'):
                self.webview.loadFinished.connect(lambda success: 
                    print(f"DEBUG: WebView - Carga {'exitosa' if success else 'fallida'}"))
                    
        except Exception as e:
            print(f"DEBUG: Error conectando señales básicas: {e}")
    
    def _init_3d_scene(self):
        """Inicializa la escena 3D"""
        try:
            # Ejes de referencia
            axis = gl.GLAxisItem()
            axis.setSize(50, 50, 50)
            self.view3d.addItem(axis)

            # Agregar puntos cardinales y direcciones verticales
            self._add_compass_directions()
            
            # Cubo placeholder inicial
            self._create_placeholder_cube()
            
        except Exception as e:
            print(f"Error inicializando escena 3D: {e}")

    def _add_compass_directions(self):
        """Agrega etiquetas para los puntos cardinales y direcciones verticales"""
        try:
            # Radio para las direcciones - aumentado para mejor visibilidad
            radius = 70
            
            # Norte (N) - eje Y positivo - Rojo brillante
            north_text = gl.GLTextItem(pos=np.array([0, radius, 0]), text='NORTE', 
                                    color=(1, 0, 0, 1))
            self.view3d.addItem(north_text)
            
            # Sur (S) - eje Y negativo - Verde brillante
            south_text = gl.GLTextItem(pos=np.array([0, -radius, 0]), text='SUR', 
                                    color=(0, 1, 0, 1))
            self.view3d.addItem(south_text)
            
            # Este (E) - eje X positivo - Azul brillante
            east_text = gl.GLTextItem(pos=np.array([radius, 0, 0]), text='ESTE', 
                                    color=(0, 0, 1, 1))
            self.view3d.addItem(east_text)
            
            # Oeste (W) - eje X negativo - Amarillo brillante
            west_text = gl.GLTextItem(pos=np.array([-radius, 0, 0]), text='OESTE', 
                                    color=(1, 1, 0, 1))
            self.view3d.addItem(west_text)
            
            # Arriba (UP) - eje Z positivo - Magenta brillante
            up_text = gl.GLTextItem(pos=np.array([0, 0, radius]), text='ARRIBA', 
                                color=(1, 0, 1, 1))
            self.view3d.addItem(up_text)
            
            # Abajo (DOWN) - eje Z negativo - Cian brillante
            down_text = gl.GLTextItem(pos=np.array([0, 0, -radius]), text='ABAJO', 
                                    color=(0, 1, 1, 1))
            self.view3d.addItem(down_text)
            
            # Líneas de referencia para mejor orientación
            self._add_reference_lines()
            
        except Exception as e:
            print(f"Error agregando direcciones de brújula: {e}")

    def _add_reference_lines(self):
        """Agrega líneas de referencia para mejor orientación visual"""
        try:
            # Líneas para los ejes principales (más largas y visibles)
            size = 80
            
            # Eje X (Este-Oeste) - Rojo
            x_axis = gl.GLLinePlotItem(pos=np.array([[-size, 0, 0], [size, 0, 0]]), 
                                    color=(1, 0, 0, 0.7), width=2)
            self.view3d.addItem(x_axis)
            
            # Eje Y (Norte-Sur) - Verde
            y_axis = gl.GLLinePlotItem(pos=np.array([[0, -size, 0], [0, size, 0]]), 
                                    color=(0, 1, 0, 0.7), width=2)
            self.view3d.addItem(y_axis)
            
            # Eje Z (Arriba-Abajo) - Azul
            z_axis = gl.GLLinePlotItem(pos=np.array([[0, 0, -size], [0, 0, size]]), 
                                    color=(0, 0, 1, 0.7), width=2)
            self.view3d.addItem(z_axis)
            
            # Círculo de horizonte (plano XY)
            theta = np.linspace(0, 2*np.pi, 100)
            circle_radius = 50
            x_circle = circle_radius * np.cos(theta)
            y_circle = circle_radius * np.sin(theta)
            z_circle = np.zeros_like(theta)
            circle_points = np.vstack([x_circle, y_circle, z_circle]).T
            horizon_circle = gl.GLLinePlotItem(pos=circle_points, color=(1, 1, 1, 0.5), width=1)
            self.view3d.addItem(horizon_circle)
            
            # Agregar etiquetas en las puntas de los ejes con mejor visibilidad
            label_offset = 10  # Offset aumentado para mejor separación
            
            # Eje X - Rojo
            x_plus_label = gl.GLTextItem(pos=np.array([size + label_offset, 0, 0]), text='X+', 
                                        color=(1, 0.5, 0.5, 1))
            x_minus_label = gl.GLTextItem(pos=np.array([-size - label_offset, 0, 0]), text='X-', 
                                        color=(1, 0.5, 0.5, 1))
            self.view3d.addItem(x_plus_label)
            self.view3d.addItem(x_minus_label)
            
            # Eje Y - Verde
            y_plus_label = gl.GLTextItem(pos=np.array([0, size + label_offset, 0]), text='Y+', 
                                        color=(0.5, 1, 0.5, 1))
            y_minus_label = gl.GLTextItem(pos=np.array([0, -size - label_offset, 0]), text='Y-', 
                                        color=(0.5, 1, 0.5, 1))
            self.view3d.addItem(y_plus_label)
            self.view3d.addItem(y_minus_label)
            
            # Eje Z - Azul
            z_plus_label = gl.GLTextItem(pos=np.array([0, 0, size + label_offset]), text='Z+', 
                                        color=(0.5, 0.5, 1, 1))
            z_minus_label = gl.GLTextItem(pos=np.array([0, 0, -size - label_offset]), text='Z-', 
                                        color=(0.5, 0.5, 1, 1))
            self.view3d.addItem(z_plus_label)
            self.view3d.addItem(z_minus_label)
            
        except Exception as e:
            print(f"Error agregando líneas de referencia: {e}")

    def _create_placeholder_cube(self):
        """Crea un cubo placeholder"""
        verts = np.array([
            [-5,-5,-5], [5,-5,-5], [5,5,-5], [-5,5,-5],
            [-5,-5,5], [5,-5,5], [5,5,5], [-5,5,5]
        ], dtype=float)
        faces = np.array([
            [0,1,2],[0,2,3],
            [4,5,6],[4,6,7],
            [0,1,5],[0,5,4],
            [2,3,7],[2,7,6],
            [1,2,6],[1,6,5],
            [0,3,7],[0,7,4]
        ], dtype=int)
        meshdata = gl.MeshData(vertexes=verts, faces=faces)
        self.mesh_item = gl.GLMeshItem(meshdata=meshdata, smooth=False, shader='shaded', 
                                      drawEdges=True, edgeColor=(1,1,1,1))
        self.view3d.addItem(self.mesh_item)

    def select_stl(self):
        path, _ = QFileDialog.getOpenFileName(self, "Seleccionar STL", "", "STL Files (*.stl);;All Files (*)")
        if path:
            self.load_stl(path)

    def load_stl(self, path: str) -> bool:
        if not os.path.exists(path):
            self.signals.status.emit(f"No existe: {path}")
            return False
        try:
            verts, faces = self._load_mesh_vertices_faces(path)
            if verts is None or faces is None:
                raise RuntimeError("No se obtuvieron vértices/caras")
            
            # Centrar el modelo
            center = verts.mean(axis=0)
            verts = verts - center
            
            # Escalar el modelo para que quepa bien en la vista
            max_dim = np.max(np.ptp(verts, axis=0))
            if max_dim > 0:
                scale = 50.0 / max_dim  # Escalar a aproximadamente 50 unidades
                verts = verts * scale
            
            meshdata = gl.MeshData(vertexes=verts, faces=faces)
            try:
                self.view3d.removeItem(self.mesh_item)
            except Exception:
                pass
            
            self.mesh_item = gl.GLMeshItem(meshdata=meshdata, smooth=True, shader='shaded', 
                                          drawEdges=True, edgeColor=(0.2,0.2,0.2,1))
            self.view3d.addItem(self.mesh_item)
            self.signals.status.emit(f"STL cargado: {Path(path).name}")
            return True
        except Exception as e:
            self.signals.status.emit(f"Error cargando STL: {e}")
            return False

    def _load_mesh_vertices_faces(self, path):
        if _trimesh is not None:
            try:
                tm = _trimesh.load(path, force='mesh')
                verts = np.array(tm.vertices, dtype=float)
                faces = np.array(tm.faces, dtype=int)
                return verts, faces
            except Exception:
                pass
        if _npstl is not None:
            try:
                stl_mesh = _npstl.Mesh.from_file(path)
                all_triangles = stl_mesh.vectors.reshape(-1, 3)
                verts, inv = np.unique(all_triangles, axis=0, return_inverse=True)
                faces = inv.reshape(-1, 3)
                return verts.astype(float), faces.astype(int)
            except Exception:
                pass
        try:
            faces_tris = []
            with open(path, 'r', errors='ignore') as f:
                lines = f.readlines()
            pts = []
            for ln in lines:
                ln = ln.strip()
                if ln.lower().startswith('vertex'):
                    parts = ln.split()
                    if len(parts) >= 4:
                        x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                        pts.append((x, y, z))
                        if len(pts) == 3:
                            faces_tris.append((pts[0], pts[1], pts[2]))
                            pts = []
            if not faces_tris:
                raise RuntimeError("No triangles detectados en STL ASCII")
            flattened = np.vstack(faces_tris)
            verts, inv = np.unique(flattened, axis=0, return_inverse=True)
            faces = inv.reshape(-1, 3)
            return verts.astype(float), faces.astype(int)
        except Exception as e:
            raise RuntimeError(f"No pudo cargar STL: {e}")

    def on_status(self, msg: str):
        self.lbl_status.setText("Status: " + msg)

    def on_gps_update(self, lat: float, lon: float):
        self.current_lat = lat
        self.current_lon = lon
        self.lbl_coords.setText(f"Lat: {lat:.6f} Lon: {lon:.6f}")
        try:
            js_code = f"updatePosition({lat}, {lon});"
            self.webview.page().runJavaScript(js_code)
        except Exception as e:
            print(f"Error enviando datos al mapa: {e}")

    def on_attitude_update(self, roll: float, pitch: float, yaw: float):
        self.current_roll = roll
        self.current_pitch = pitch
        self.current_yaw = yaw
        self.lbl_att.setText(f"Roll: {roll:.2f} Pitch: {pitch:.2f} Yaw: {yaw:.2f}")
        try:
            # Método mejorado: usar transformación directa sin recrear el mesh
            # Resetear transformación
            self.mesh_item.resetTransform()
            
            # Aplicar rotaciones en el orden correcto para sistemas aeroespaciales:
            # 1. Yaw (rotación Z)
            # 2. Pitch (rotación Y) 
            # 3. Roll (rotación X)
            self.mesh_item.rotate(yaw, 0, 0, 1, local=False)   # Yaw alrededor del eje Z global
            self.mesh_item.rotate(pitch, 0, 1, 0, local=False) # Pitch alrededor del eje Y global
            self.mesh_item.rotate(roll, 1, 0, 0, local=False)  # Roll alrededor del eje X global
            
        except Exception as e:
            print("Error aplicando rotación:", e)

    def _client_worker(self, host, port):
        self.signals.status.emit("conectando...")
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(5.0)
                s.connect((host, port))
                s.settimeout(None)
                self.signals.status.emit(f"Conectado a {host}:{port}")
                while True:
                    data = s.recv(4096)
                    if not data:
                        break
                    lines = data.decode("utf-8", errors="ignore").splitlines()
                    for line in lines:
                        line = line.strip()
                        if not line:
                            continue
                        parts = line.split(',')
                        if len(parts) < 10:
                            continue
                        try:
                            lat = float(parts[0])
                            lon = float(parts[1])
                            roll = float(parts[7]) if parts[7] != '' else 0.0
                            pitch = float(parts[8]) if parts[8] != '' else 0.0
                            yaw = float(parts[9]) if parts[9] != '' else 0.0
                            self.signals.gps_update.emit(lat, lon)
                            self.signals.attitude_update.emit(roll, pitch, yaw)
                        except Exception as e:
                            print("Parse error:", e, "line:", line)
        except Exception as e:
            self.signals.status.emit(f"Error conexión: {e}")

    def closeEvent(self, ev: QCloseEvent):
        try:
            if hasattr(self, "_fallback_html_file") and os.path.exists(self._fallback_html_file):
                os.remove(self._fallback_html_file)
        except Exception:
            pass
        super().closeEvent(ev)

if __name__ == "__main__":
    # Configuración para evitar errores OpenGL
    os.environ["QT_QUICK_BACKEND"] = "software"
    
    # Configurar el icono de la aplicación (para la barra de tareas también)
    app = QApplication(sys.argv)
    
    # BUSCAR Y ESTABLECER EL LOGO PARA TODA LA APLICACIÓN
    try:
        logo_paths = get_logo_paths()
        logo_set = False
        
        for logo_path in logo_paths:
            if logo_path.exists():
                app.setWindowIcon(QIcon(str(logo_path)))
                print(f"DEBUG: Logo de aplicación cargado desde: {logo_path}")
                logo_set = True
                break
        
        if not logo_set:
            print("DEBUG: No se encontró el logo para la aplicación")
            
    except Exception as e:
        print(f"DEBUG: Error cargando logo de aplicación: {e}")
    
    host = "127.0.0.1"
    port = 5000
    if len(sys.argv) >= 2:
        host = sys.argv[1]
    if len(sys.argv) >= 3:
        port = int(sys.argv[2])

    win = SensorClientWindow(host=host, port=port, stl_fixed_path=None)
    win.show()
    
    # Forzar la actualización del icono
    app.processEvents()
    
    sys.exit(app.exec())