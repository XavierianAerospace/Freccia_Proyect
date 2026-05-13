#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SensorClientWindow - Qt6 + PyVista
Compatible Linux, Windows, macOS, WSL, incluso sin entorno gráfico.
- Cliente TCP que procesa CSV (lat,lon,...Roll,Pitch,Yaw,...)
- Mapa OSM embebido en QWebEngineView + QWebChannel
- Visor 3D con PyVista (carga .stl y rota según actitud)
Todo en UNA SOLA CLASE
"""

import sys, os, socket, threading, tempfile
from pathlib import Path

# Qt6
from PyQt6.QtWidgets import (
    QApplication, QWidget, QHBoxLayout, QVBoxLayout,
    QLabel, QPushButton, QFileDialog
)
from PyQt6.QtCore import pyqtSignal, QObject, QUrl
from PyQt6.QtGui import QCloseEvent
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebChannel import QWebChannel

# PyVista (3D viewer)
import numpy as np
import pyvista as pv
from pyvistaqt import QtInteractor

# STL loaders
import trimesh
from stl import mesh as npstl_mesh


class DataSignals(QObject):
    gps_update = pyqtSignal(float, float)
    attitude_update = pyqtSignal(float, float, float)
    status = pyqtSignal(str)


class SensorClientWindow(QWidget):
    def __init__(self, host="127.0.0.1", port=5000, stl_fixed_path="CoheteGUI.STL"):
        super().__init__()
        self.setWindowTitle("SensorClientWindow - Qt6 OSM + PyVista")
        self.resize(1200, 720)

        # Señales
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
        self.fixed_stl_path = stl_fixed_path

        # Layout principal
        main_layout = QHBoxLayout()
        left_col = QVBoxLayout()
        right_col = QVBoxLayout()

        # Controles
        top_controls = QHBoxLayout()
        self.lbl_status = QLabel("Status: desconectado")
        btn_load = QPushButton("Cargar .stl (otra ruta)")
        btn_load.clicked.connect(self.select_stl)
        top_controls.addWidget(self.lbl_status)
        top_controls.addStretch(1)
        top_controls.addWidget(btn_load)
        left_col.addLayout(top_controls)

        # WebEngineView (mapa OSM)
        self.webview = QWebEngineView()
        left_col.addWidget(self.webview, stretch=1)

        # PyVista 3D viewer
        self.plotter = QtInteractor(self, auto_update=True)
        right_col.addWidget(self.plotter.interactor, stretch=1)

        # Info footer
        self.lbl_coords = QLabel("Lat: -, Lon: -")
        self.lbl_att = QLabel("Roll: 0  Pitch: 0  Yaw: 0")
        right_col.addWidget(self.lbl_coords)
        right_col.addWidget(self.lbl_att)

        main_layout.addLayout(left_col, stretch=2)
        main_layout.addLayout(right_col, stretch=2)
        self.setLayout(main_layout)

        # Crear mapa
        self._html_file = self._create_map_html()
        self._init_web_channel()

        # Inicializar escena 3D
        self.mesh_actor = None
        self._init_3d_scene()

        # Cargar STL fijo
        if self.fixed_stl_path and os.path.exists(self.fixed_stl_path):
            self.load_stl(self.fixed_stl_path)

        # Iniciar cliente TCP
        self.client_thread = threading.Thread(target=self._client_worker, args=(host, port), daemon=True)
        self.client_thread.start()

    # ---------------- MAPA ----------------
    def _create_map_html(self) -> str:
        html = r"""
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8"/>
  <title>Mapa OSM Telemetría</title>
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
  <style> html, body, #map { height:100%; margin:0; padding:0; } </style>
</head>
<body>
  <div id="map"></div>
  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
  <script>
    var map = L.map('map').setView([0,0], 2);
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
      maxZoom: 19, attribution: '© OpenStreetMap'
    }).addTo(map);

    var rocketIcon = L.divIcon({
      className:'', html:'<div id="rocket" style="transform:rotate(0deg);">🚀</div>',
      iconSize:[24,24], iconAnchor:[12,12]
    });
    var marker = L.marker([0,0], {icon:rocketIcon}).addTo(map);

    new QWebChannel(qt.webChannelTransport, function(channel){
      window.bridge = channel.objects.bridge;
      bridge.setPosition.connect(function(lat,lon,heading){
        marker.setLatLng([lat,lon]);
        document.getElementById("rocket").style.transform='rotate('+heading+'deg)';
        map.panTo([lat,lon],{animate:false});
      });
    });
  </script>
</body>
</html>
"""
        tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".html", prefix="map_")
        tmp.write(html.encode("utf-8"))
        tmp.flush(); tmp.close()
        return tmp.name

    def _init_web_channel(self):
        self.webview.load(QUrl.fromLocalFile(self._html_file))
        self.channel = QWebChannel()
        class Bridge(QObject):
            setPosition = pyqtSignal(float, float, float)
        self.bridge = Bridge()
        self.channel.registerObject("bridge", self.bridge)
        self.webview.page().setWebChannel(self.channel)

    # ---------------- 3D SCENE ----------------
    def _init_3d_scene(self):
        self.plotter.set_background("black")
        self.plotter.show_axes()
        self.plotter.add_axes()
        self.plotter.add_floor()

    def select_stl(self):
        path, _ = QFileDialog.getOpenFileName(self, "Seleccionar STL", "", "STL (*.stl)")
        if path: self.load_stl(path)

    def load_stl(self, path: str):
        try:
            if path.lower().endswith(".stl"):
                mesh = pv.read(path)
            else:
                mesh = None
            if mesh:
                if self.mesh_actor:
                    self.plotter.remove_actor(self.mesh_actor)
                self.mesh_actor = self.plotter.add_mesh(mesh, color="silver")
                self.signals.status.emit(f"STL cargado: {Path(path).name}")
        except Exception as e:
            self.signals.status.emit(f"Error STL: {e}")

    # ---------------- HANDLERS ----------------
    def on_status(self, msg): self.lbl_status.setText("Status: " + msg)

    def on_gps_update(self, lat, lon):
        self.current_lat, self.current_lon = lat, lon
        self.lbl_coords.setText(f"Lat: {lat:.6f} Lon: {lon:.6f}")
        self.bridge.setPosition.emit(lat, lon, self.current_yaw)

    def on_attitude_update(self, roll, pitch, yaw):
        self.current_roll, self.current_pitch, self.current_yaw = roll, pitch, yaw
        self.lbl_att.setText(f"Roll:{roll:.1f} Pitch:{pitch:.1f} Yaw:{yaw:.1f}")
        if self.mesh_actor:
            transform = pv.transformations.axis_angle_rotation([1,0,0], roll) @ \
                        pv.transformations.axis_angle_rotation([0,1,0], pitch) @ \
                        pv.transformations.axis_angle_rotation([0,0,1], yaw)
            self.mesh_actor.user_matrix = transform
        if self.current_lat and self.current_lon:
            self.bridge.setPosition.emit(self.current_lat, self.current_lon, yaw)

    # ---------------- TCP CLIENT ----------------
    def _client_worker(self, host, port):
        self.signals.status.emit("conectando...")
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((host, port))
                self.signals.status.emit(f"Conectado a {host}:{port}")
                while True:
                    data = s.recv(4096)
                    if not data: break
                    for line in data.decode("utf-8","ignore").splitlines():
                        parts = line.split(",")
                        if len(parts) >= 10:
                            try:
                                lat, lon = float(parts[0]), float(parts[1])
                                roll, pitch, yaw = map(float, parts[7:10])
                                self.signals.gps_update.emit(lat, lon)
                                self.signals.attitude_update.emit(roll, pitch, yaw)
                            except: pass
        except Exception as e:
            self.signals.status.emit(f"Error conexión: {e}")

    # ---------------- CLOSE ----------------
    def closeEvent(self, ev: QCloseEvent):
        try: os.remove(self._html_file)
        except: pass
        super().closeEvent(ev)


if __name__ == "__main__":
    host, port = "127.0.0.1", 5000
    if len(sys.argv) > 1: host = sys.argv[1]
    if len(sys.argv) > 2: port = int(sys.argv[2])

    # Fuerza software rendering si no hay GPU (evita el BadWindow)
    if sys.platform.startswith("linux") and not os.environ.get("DISPLAY"):
        os.environ["QT_OPENGL"] = "software"
        os.environ["PYVISTA_OFF_SCREEN"] = "true"

    app = QApplication(sys.argv)
    win = SensorClientWindow(host=host, port=port)
    win.show()
    sys.exit(app.exec())
