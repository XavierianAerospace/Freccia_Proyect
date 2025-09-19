#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SensorClientWindow - Qt6 (PyQt6)
- Cliente TCP que procesa CSV (lat,lon,...Roll,Pitch,Yaw,...)
- Mapa OSM (Leaflet) embebido en QWebEngineView + QWebChannel
- Visor 3D con pyqtgraph.opengl que carga .stl desde una ruta fija y rota según Roll/Pitch/Yaw
Todo en UNA SOLA CLASE: SensorClientWindow
"""

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
from PyQt6.QtCore import pyqtSignal, QObject, QUrl
from PyQt6.QtGui import QCloseEvent

# Web engine + channel (Qt6)
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebChannel import QWebChannel

# 3D viewer
import numpy as np
import pyqtgraph as pg
import pyqtgraph.opengl as gl

# Optional loaders
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


class DataSignals(QObject):
    gps_update = pyqtSignal(float, float)            # lat, lon
    attitude_update = pyqtSignal(float, float, float)  # roll, pitch, yaw
    status = pyqtSignal(str)


class SensorClientWindow(QWidget):
    """
    Clase única que contiene:
      - UI (mapa + 3D)
      - Cliente TCP en hilo
      - Parser y aplicación de datos (posicion + actitud)
      - Carga automática de STL desde ruta fija
    """

    def __init__(self, host="127.0.0.1", port=5000, stl_fixed_path: str = None):
        super().__init__()
        self.setWindowTitle("SensorClientWindow - Qt6 OSM + STL (única clase)")
        self.resize(1200, 720)

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

        # Ruta fija del STL (la pones aquí). Si None, no carga al inicio.
        # Según tu petición, lo dejamos apuntando a la ruta fija:
        self.fixed_stl_path = stl_fixed_path or "CoheteGUI.STL"

        # === UI ===
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
        self.view3d.opts['distance'] = 400
        self.view3d.setMinimumWidth(480)
        right_col.addWidget(self.view3d, stretch=1)

        # Info footer
        info_layout = QVBoxLayout()
        self.lbl_coords = QLabel("Lat: -, Lon: -")
        self.lbl_att = QLabel("Roll: 0.0  Pitch: 0.0  Yaw: 0.0")
        info_layout.addWidget(self.lbl_coords)
        info_layout.addWidget(self.lbl_att)
        right_col.addLayout(info_layout)

        main_layout.addLayout(left_col, stretch=2)
        main_layout.addLayout(right_col, stretch=2)
        self.setLayout(main_layout)

        # Crear HTML temporal del mapa (Leaflet + OSM)
        self._html_file = self._create_map_html()
        self._init_web_channel()

        # Inicializar escena 3D (grid, eje, mesh placeholder)
        self._init_3d_scene()

        # Si el STL fijo existe, cargarlo automáticamente
        if self.fixed_stl_path and os.path.exists(self.fixed_stl_path):
            self.load_stl(self.fixed_stl_path)
            self.signals.status.emit(f"STL cargado automáticamente: {Path(self.fixed_stl_path).name}")
        else:
            self.signals.status.emit("STL no encontrado en ruta fija.")

        # Iniciar cliente TCP en hilo (no bloqueante)
        self.client_thread = threading.Thread(target=self._client_worker, args=(host, port), daemon=True)
        self.client_thread.start()

    # ---------------------------------------
    # Map HTML & QWebChannel
    # ---------------------------------------
    def _create_map_html(self) -> str:
        """
        Crea archivo HTML temporal con Leaflet + OSM y QWebChannel glue.
        """
        html = r"""
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8" />
  <title>Mapa OSM Telemetría</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
  <style>
    html, body, #map { height: 100%; margin:0; padding:0; }
    .rocket-icon { width: 48px; transform-origin: center; }
  </style>
</head>
<body>
  <div id="map"></div>

  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
  <script>
    var map = L.map('map').setView([0,0], 2);
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 19,
      attribution: '&copy; OpenStreetMap contributors'
    }).addTo(map);

    var rocketIcon = L.divIcon({
      className: '',
      html: '<img id="rocket_img" src="" class="rocket-icon" />',
      iconSize: [48,48],
      iconAnchor: [24,24]
    });

    var marker = L.marker([0,0], {icon: rocketIcon}).addTo(map);

    var defaultSVG = 'data:image/svg+xml;utf8,' + encodeURIComponent(
      '<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 24 24"><path fill="%23ff3300" d="M12 2l3 7h6l-5 4 2 7-6-4-6 4 2-7-5-4h6z"/></svg>'
    );
    document.getElementById('rocket_img').src = defaultSVG;

    new QWebChannel(qt.webChannelTransport, function(channel) {
      window.bridge = channel.objects.bridge;
      // Python emits bridge.setPosition(lat, lon, heading)
      bridge.setPosition.connect(function(lat, lon, heading) {
        marker.setLatLng([lat, lon]);
        var img = document.getElementById('rocket_img');
        img.style.transform = 'rotate(' + heading + 'deg)';
        map.panTo([lat, lon], {animate:false});
      });
      bridge.setIconImage.connect(function(dataUri) {
        document.getElementById('rocket_img').src = dataUri;
      });
      bridge.setZoom.connect(function(z) {
        map.setZoom(z);
      });
      bridge.logFromPython.connect(function(s) {
        console.log("PY:", s);
      });
    });
  </script>
</body>
</html>
"""
        tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".html", prefix="map_")
        tmp.write(html.encode("utf-8"))
        tmp.flush()
        tmp.close()
        return tmp.name

    def _init_web_channel(self):
        """Configura QWebChannel y expone un objeto bridge con señales que JS escucha."""
        self.webview.load(QUrl.fromLocalFile(self._html_file))
        self.channel = QWebChannel()
        class Bridge(QObject):
            setPosition = pyqtSignal(float, float, float)
            setIconImage = pyqtSignal(str)
            setZoom = pyqtSignal(int)
            logFromPython = pyqtSignal(str)
        self.bridge = Bridge()
        self.channel.registerObject('bridge', self.bridge)
        self.webview.page().setWebChannel(self.channel)

    # ---------------------------------------
    # 3D scene: inicialización + carga STL
    # ---------------------------------------
    def _init_3d_scene(self):
        # grid
        g = gl.GLGridItem()
        g.scale(10, 10, 1)
        self.view3d.addItem(g)
        # axis
        axis = gl.GLAxisItem()
        axis.setSize(100, 100, 100)
        self.view3d.addItem(axis)

        # placeholder cube
        verts = np.array([
            [-10,-10,-10], [10,-10,-10], [10,10,-10], [-10,10,-10],
            [-10,-10,10], [10,-10,10], [10,10,10], [-10,10,10]
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
        self.mesh_item = gl.GLMeshItem(meshdata=meshdata, smooth=False, shader='shaded', drawEdges=True)
        self.view3d.addItem(self.mesh_item)

    def select_stl(self):
        path, _ = QFileDialog.getOpenFileName(self, "Seleccionar archivo STL", "", "STL Files (*.stl);;All Files (*)")
        if path:
            self.load_stl(path)

    def load_stl(self, path: str) -> bool:
        """
        Carga STL desde ruta path e instancia el mesh en la escena 3D.
        """
        if not os.path.exists(path):
            self.signals.status.emit(f"No existe: {path}")
            return False
        try:
            verts, faces = self._load_mesh_vertices_faces(path)
            if verts is None or faces is None:
                raise RuntimeError("No se obtuvieron vértices/caras")
            meshdata = gl.MeshData(vertexes=verts, faces=faces)
            # remove old if exists
            try:
                self.view3d.removeItem(self.mesh_item)
            except Exception:
                pass
            self.mesh_item = gl.GLMeshItem(meshdata=meshdata, smooth=False, shader='shaded', drawEdges=True)
            self.view3d.addItem(self.mesh_item)
            self.signals.status.emit(f"STL cargado: {Path(path).name}")
            return True
        except Exception as e:
            self.signals.status.emit(f"Error cargando STL: {e}")
            return False

    def _load_mesh_vertices_faces(self, path):
        # try trimesh
        if _trimesh is not None:
            try:
                tm = _trimesh.load(path, force='mesh')
                verts = np.array(tm.vertices, dtype=float)
                faces = np.array(tm.faces, dtype=int)
                return verts, faces
            except Exception:
                pass
        # try numpy-stl
        if _npstl is not None:
            try:
                stl_mesh = _npstl.Mesh.from_file(path)
                all_triangles = stl_mesh.vectors.reshape(-1, 3)
                verts, inv = np.unique(all_triangles, axis=0, return_inverse=True)
                faces = inv.reshape(-1, 3)
                return verts.astype(float), faces.astype(int)
            except Exception:
                pass
        # fallback ASCII parser
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

    # ---------------------------------------
    # Handlers de señales (actualiza UI y escena)
    # ---------------------------------------
    def on_status(self, msg: str):
        self.lbl_status.setText("Status: " + msg)
        try:
            self.bridge.logFromPython.emit(msg)
        except Exception:
            pass

    def on_gps_update(self, lat: float, lon: float):
        self.current_lat = lat
        self.current_lon = lon
        self.lbl_coords.setText(f"Lat: {lat:.6f}  Lon: {lon:.6f}")
        heading = float(self.current_yaw if self.current_yaw is not None else 0.0)
        try:
            self.bridge.setPosition.emit(float(lat), float(lon), float(heading))
        except Exception:
            pass

    def on_attitude_update(self, roll: float, pitch: float, yaw: float):
        # Guardar y actualizar label
        self.current_roll = roll
        self.current_pitch = pitch
        self.current_yaw = yaw
        self.lbl_att.setText(f"Roll: {roll:.2f}  Pitch: {pitch:.2f}  Yaw: {yaw:.2f}")

        # Aplicar rotación al mesh: centrar, rotar y recrear mesh
        try:
            md = self.mesh_item.meshData()
            v = np.array(md.vertexes(), dtype=float)
            f = np.array(md.faces(), dtype=int)
            center = v.mean(axis=0)
            v_c = v - center
            r = np.deg2rad(roll)
            p = np.deg2rad(pitch)
            y = np.deg2rad(yaw)
            Rx = np.array([[1,0,0],[0,np.cos(r),-np.sin(r)],[0,np.sin(r),np.cos(r)]])
            Ry = np.array([[np.cos(p),0,np.sin(p)],[0,1,0],[-np.sin(p),0,np.cos(p)]])
            Rz = np.array([[np.cos(y),-np.sin(y),0],[np.sin(y),np.cos(y),0],[0,0,1]])
            R = Rz @ Ry @ Rx
            v_rot = (v_c @ R.T) + center
            meshdata = gl.MeshData(vertexes=v_rot, faces=f)
            # reemplazar mesh en la escena
            self.view3d.removeItem(self.mesh_item)
            self.mesh_item = gl.GLMeshItem(meshdata=meshdata, smooth=False, shader='shaded', drawEdges=True)
            self.view3d.addItem(self.mesh_item)
        except Exception as e:
            print("Error aplicando rotación:", e)

        # actualizar rotación del icono en el mapa (heading = yaw)
        if self.current_lat is not None and self.current_lon is not None:
            try:
                self.bridge.setPosition.emit(float(self.current_lat), float(self.current_lon), float(yaw))
            except Exception:
                pass

    # ---------------------------------------
    # TCP client (hilo)
    # ---------------------------------------
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
                        # Esperamos al menos lat,lon y roll/pitch/yaw en índices
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

    # ---------------------------------------
    # Utilities
    # ---------------------------------------
    def closeEvent(self, ev: QCloseEvent):
        # limpiar archivo temporal
        try:
            if hasattr(self, "_html_file") and os.path.exists(self._html_file):
                os.remove(self._html_file)
        except Exception:
            pass
        super().closeEvent(ev)


# === Ejecutable ===
if __name__ == "__main__":
    # Uso: python thisfile.py [host] [port]
    host = "127.0.0.1"
    port = 5000
    if len(sys.argv) >= 2:
        host = sys.argv[1]
    if len(sys.argv) >= 3:
        port = int(sys.argv[2])

    app = QApplication(sys.argv)
    # La ruta STl fija ya viene por defecto en la clase; si quieres cambiar, pásala en el constructor.
    win = SensorClientWindow(host=host, port=port, stl_fixed_path=None)
    win.show()
    sys.exit(app.exec())
