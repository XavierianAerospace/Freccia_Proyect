# ═══════════════════════════════════════════════════════════════════════════════
#  FRECCIA_XAE — client_viewer.py
#  Las env vars de WebEngine van ANTES de cualquier import de Qt.
#  NO se pone QT_OPENGL=software porque rompe pyqtgraph OpenGL.
#  En su lugar se usa AA_ShareOpenGLContexts para que ambos compartan contexto.
# ═══════════════════════════════════════════════════════════════════════════════
import sys
import os

# ── Solo flags de Chromium/WebEngine, NO tocar QT_OPENGL ─────────────────────
os.environ.setdefault(
    "QTWEBENGINE_CHROMIUM_FLAGS",
    "--disable-gpu-compositing "
    "--disable-gpu-sandbox "
    "--enable-webgl "
    "--enable-unsafe-webgl "
    "--ignore-gpu-blocklist"
)
os.environ.setdefault("QTWEBENGINE_DISABLE_SANDBOX", "1")
# ─────────────────────────────────────────────────────────────────────────────

import socket
import threading
import time
from pathlib import Path

# AA_ShareOpenGLContexts y AA_UseDesktopOpenGL deben ir antes de crear QApplication
from PyQt6.QtWidgets import QApplication
from PyQt6.QtCore import Qt
QApplication.setAttribute(Qt.ApplicationAttribute.AA_ShareOpenGLContexts)
QApplication.setAttribute(Qt.ApplicationAttribute.AA_UseDesktopOpenGL)

# Resto de imports Qt
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QFileDialog, QSplitter, QFrame
)
from PyQt6.QtCore  import pyqtSignal, QObject, QUrl, QTimer
from PyQt6.QtGui   import QCloseEvent, QIcon
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebEngineCore    import QWebEngineProfile, QWebEngineSettings

import numpy as np
import pyqtgraph.opengl as gl

_trimesh = None
_npstl   = None
try:
    import trimesh as _trimesh
except Exception:
    pass
try:
    from stl import mesh as _npstl_mesh
    _npstl = _npstl_mesh
except Exception:
    pass

# ── Configuración backend ─────────────────────────────────────────────────────
FLASK_HOST = os.environ.get("FRECCIA_FLASK_HOST", "127.0.0.1")
FLASK_PORT = int(os.environ.get("FRECCIA_FLASK_PORT", "5001"))
CESIUM_URL = f"http://{FLASK_HOST}:{FLASK_PORT}"

# Lista de puertos candidatos para TileServer-GL
TILESERVER_CANDIDATE_PORTS = [8080, 8099, 8081, 8888]
# ─────────────────────────────────────────────────────────────────────────────


def _is_port_open(host: str, port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(1)
        return s.connect_ex((host, port)) == 0


def get_logo_paths():
    if hasattr(sys, "_MEIPASS"):
        base = Path(sys._MEIPASS)
        return [base / "Map" / "assets" / "logo_xae.png", base / "logo_xae.png"]
    base = Path(__file__).resolve().parent
    return [
        base / "Map" / "assets" / "logo_xae.png",
        base / "assets" / "logo_xae.png",
        Path("Map/assets/logo_xae.png"),
    ]


# ─────────────────────────────────────────────────────────────────────────────
class DataSignals(QObject):
    gps_update      = pyqtSignal(float, float, float)
    attitude_update = pyqtSignal(float, float, float)
    status          = pyqtSignal(str)
    backend_ready   = pyqtSignal()


# ─────────────────────────────────────────────────────────────────────────────
class SensorClientWindow(QWidget):

    def __init__(self, host: str = "127.0.0.1", port: int = 5000,
                 stl_fixed_path: str = None):
        super().__init__()
        self.setWindowTitle("FRECCIA_XAE — MAPA 3D CESIUM | Attitude Viewer")
        self.resize(1280, 760)
        self._apply_window_icon()
        self.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose, True)

        self.signals = DataSignals()
        self.signals.gps_update.connect(self.on_gps_update)
        self.signals.attitude_update.connect(self.on_attitude_update)
        self.signals.status.connect(self.on_status)
        self.signals.backend_ready.connect(self._load_cesium_url)

        self.current_lat   = 0.0
        self.current_lon   = 0.0
        self.current_alt   = 0.0
        self.current_roll  = 0.0
        self.current_pitch = 0.0
        self.current_yaw   = 0.0
        self.fixed_stl_path = stl_fixed_path or "CoheteGUI.STL"

        self._setup_ui()
        self._configure_webview()
        self._init_3d_scene()

        if self.fixed_stl_path and os.path.exists(self.fixed_stl_path):
            self.load_stl(self.fixed_stl_path)

        # Backend Cesium
        if _is_port_open(FLASK_HOST, FLASK_PORT):
            self.signals.status.emit(f"Backend detectado en {CESIUM_URL}")
            QTimer.singleShot(300, self._load_cesium_url)
        else:
            self.signals.status.emit("Iniciando backend Cesium…")
            threading.Thread(target=self._start_backend_inline,
                             daemon=True).start()

        # TCP telemetría
        threading.Thread(target=self._tcp_worker,
                         args=(host, port), daemon=True).start()

    # ── Icono ─────────────────────────────────────────────────────────────────
    def _apply_window_icon(self):
        for p in get_logo_paths():
            if Path(p).exists():
                self.setWindowIcon(QIcon(str(p)))
                return
        from PyQt6.QtGui import QPixmap
        px = QPixmap(64, 64)
        px.fill(Qt.GlobalColor.darkBlue)
        self.setWindowIcon(QIcon(px))

    # ── UI ────────────────────────────────────────────────────────────────────
    def _setup_ui(self):
        root = QVBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        # Toolbar
        toolbar = QWidget()
        toolbar.setFixedHeight(40)
        toolbar.setStyleSheet("background:#1a1a2e;")
        tb = QHBoxLayout(toolbar)
        tb.setContentsMargins(10, 4, 10, 4)
        self.lbl_status = QLabel("Status: inicializando…")
        self.lbl_status.setStyleSheet("color:#90caf9; font-size:12px;")
        tb.addWidget(self.lbl_status)
        tb.addStretch()
        btn = QPushButton("Cargar .stl")
        btn.setFixedHeight(28)
        btn.setStyleSheet(
            "QPushButton{background:#1565c0;color:white;"
            "border-radius:4px;padding:0 12px;}"
            "QPushButton:hover{background:#0d47a1;}")
        btn.clicked.connect(self.select_stl)
        tb.addWidget(btn)
        root.addWidget(toolbar)

        # Splitter 60 / 40
        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.setHandleWidth(3)
        splitter.setStyleSheet("QSplitter::handle{background:#2a2a4a;}")

        # Panel izquierdo — Cesium
        left = QFrame()
        ll = QVBoxLayout(left)
        ll.setContentsMargins(0, 0, 0, 0)
        self._loading_lbl = QLabel(
            "⏳  Iniciando backend Cesium Ion…\n\n"
            "TileServer-GL + Flask arrancando.")
        self._loading_lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self._loading_lbl.setStyleSheet(
            "color:#90caf9;font-size:15px;background:#0d0d1a;")
        self.webview = QWebEngineView()
        self.webview.setVisible(False)
        ll.addWidget(self._loading_lbl)
        ll.addWidget(self.webview)

        # Panel derecho — STL 3D
        right = QFrame()
        rl = QVBoxLayout(right)
        rl.setContentsMargins(4, 4, 4, 4)
        rl.setSpacing(4)
        self.view3d = gl.GLViewWidget()
        self.view3d.opts["distance"] = 200
        self.view3d.setCameraPosition(distance=200, elevation=30, azimuth=45)
        # NO usar setStyleSheet en GLViewWidget para evitar corrupción de contexto OpenGL
        self.view3d.setBackgroundColor('#0d0d1a')
        rl.addWidget(self.view3d, stretch=1)

        info = QWidget()
        info.setStyleSheet("background:#12122a;")
        il = QVBoxLayout(info)
        il.setContentsMargins(6, 4, 6, 4)
        self.lbl_coords = QLabel("Lat: –   Lon: –   Alt: – m")
        self.lbl_att    = QLabel("Roll: 0.00°   Pitch: 0.00°   Yaw: 0.00°")
        for lbl in (self.lbl_coords, self.lbl_att):
            lbl.setStyleSheet(
                "color:#80cbc4;font-size:11px;"
                "font-family:Consolas,monospace;")
        il.addWidget(self.lbl_coords)
        il.addWidget(self.lbl_att)
        rl.addWidget(info)

        splitter.addWidget(left)
        splitter.addWidget(right)
        splitter.setSizes([768, 512])
        root.addWidget(splitter, stretch=1)

    # ── WebView ───────────────────────────────────────────────────────────────
    def _configure_webview(self):
        try:
            profile = self.webview.page().profile()
            profile.setHttpCacheType(
                QWebEngineProfile.HttpCacheType.DiskHttpCache)
            profile.setPersistentCookiesPolicy(
                QWebEngineProfile.PersistentCookiesPolicy.AllowPersistentCookies)
            profile.setHttpUserAgent(
                "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                "AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/124.0.0.0 Safari/537.36")
            s = self.webview.settings()
            for attr in (
                QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls,
                QWebEngineSettings.WebAttribute.AllowRunningInsecureContent,
                QWebEngineSettings.WebAttribute.JavascriptEnabled,
                QWebEngineSettings.WebAttribute.WebGLEnabled,
                QWebEngineSettings.WebAttribute.Accelerated2dCanvasEnabled,
            ):
                s.setAttribute(attr, True)
            self.webview.page().setBackgroundColor(Qt.GlobalColor.black)
        except Exception as e:
            print(f"DEBUG WebView: {e}")

    # ── Backend inline ────────────────────────────────────────────────────────
    def _start_backend_inline(self):
        """Importa main.py y arranca TileServer-GL + Flask en hilos."""
        try:
            main_dir = str(Path(__file__).resolve().parent)
            if main_dir not in sys.path:
                sys.path.insert(0, main_dir)

            import main as freccia_main

            # Detectar puerto de TileServer dinámicamente
            ts_port = int(os.environ.get("FRECCIA_TILESERVER_PORT", "8080"))
            if not _is_port_open(FLASK_HOST, ts_port):
                for p in TILESERVER_CANDIDATE_PORTS:
                    if _is_port_open(FLASK_HOST, p):
                        ts_port = p
                        os.environ["FRECCIA_TILESERVER_PORT"] = str(p)
                        break

            base_dir = Path(freccia_main.__file__).resolve().parent

            # 1. .mbtiles
            try:
                mbtiles = freccia_main.resolve_mbtiles_path(base_dir)
            except FileNotFoundError as e:
                self.signals.status.emit(f"mbtiles no encontrado: {e}")
                return

            # 2. TileServer-GL
            try:
                # Actualizar el puerto en el módulo main antes de correrlo
                freccia_main.TILESERVER_PORT = ts_port
                freccia_main.run_tileserver(base_dir, mbtiles)
            except Exception as e:
                self.signals.status.emit(f"TileServer error: {e}")
                return

            # 3. Flask
            static   = base_dir / "cesium_app" / "static"
            template = base_dir / "cesium_app" / "templates"
            from backend.server import create_app
            # El create_app de backend.server usa env vars para tileserver_port
            app = create_app(str(mbtiles), str(static), str(template))

            threading.Thread(
                target=freccia_main.run_flask,
                args=(app,), daemon=True).start()

            # 4. Esperar Flask (reducido a 30s según plan)
            deadline = time.time() + 30
            while time.time() < deadline:
                if _is_port_open(FLASK_HOST, FLASK_PORT):
                    self.signals.status.emit(f"Backend listo — {CESIUM_URL}")
                    self.signals.backend_ready.emit()
                    return
                time.sleep(0.5)

            self.signals.status.emit("TIMEOUT: Flask no respondió en 30 s")

        except Exception as e:
            self.signals.status.emit(f"Error backend: {e}")
            print(f"DEBUG backend: {e}")

    def _load_cesium_url(self):
        self._loading_lbl.hide()
        self.webview.setVisible(True)
        print(f"DEBUG: Cargando {CESIUM_URL}")
        self.webview.load(QUrl(CESIUM_URL))
        self.webview.loadFinished.connect(self._on_loaded)

    def _on_loaded(self, ok: bool):
        if ok:
            self.signals.status.emit("Mapa Cesium listo ✓")
        else:
            self.signals.status.emit("Error Cesium — reintentando…")
            QTimer.singleShot(3000, self._load_cesium_url)

    # ── Escena 3D ─────────────────────────────────────────────────────────────
    def _init_3d_scene(self):
        try:
            ax = gl.GLAxisItem()
            ax.setSize(50, 50, 50)
            self.view3d.addItem(ax)
            self._add_compass()
            self._add_reference_lines()
            self._create_placeholder_cube()
        except Exception as e:
            print(f"Error escena 3D: {e}")

    def _add_compass(self):
        r = 70
        for pos, text, color in [
            (np.array([0,  r, 0]), "NORTE", (1, 0, 0, 1)),
            (np.array([0, -r, 0]), "SUR",   (0, 1, 0, 1)),
            (np.array([ r, 0, 0]), "ESTE",  (0, 0, 1, 1)),
            (np.array([-r, 0, 0]), "OESTE", (1, 1, 0, 1)),
            (np.array([0, 0,  r]), "ARRIBA",(1, 0, 1, 1)),
            (np.array([0, 0, -r]), "ABAJO", (0, 1, 1, 1)),
        ]:
            self.view3d.addItem(
                gl.GLTextItem(pos=pos, text=text, color=color))

    def _add_reference_lines(self):
        s = 80
        for pts, col in [
            (np.array([[-s,0,0],[s,0,0]]), (1,0,0,0.7)),
            (np.array([[0,-s,0],[0,s,0]]), (0,1,0,0.7)),
            (np.array([[0,0,-s],[0,0,s]]), (0,0,1,0.7)),
        ]:
            self.view3d.addItem(
                gl.GLLinePlotItem(pos=pts, color=col, width=2))
        t = np.linspace(0, 2*np.pi, 100)
        circle = np.vstack([50*np.cos(t), 50*np.sin(t), np.zeros(100)]).T
        self.view3d.addItem(
            gl.GLLinePlotItem(pos=circle, color=(1,1,1,0.4), width=1))
        off = 12
        for pos, text, col in [
            (np.array([ s+off, 0, 0]),  "X+", (1,.5,.5,1)),
            (np.array([-s-off, 0, 0]),  "X-", (1,.5,.5,1)),
            (np.array([0,  s+off, 0]),  "Y+", (.5,1,.5,1)),
            (np.array([0, -s-off, 0]),  "Y-", (.5,1,.5,1)),
            (np.array([0, 0,  s+off]),  "Z+", (.5,.5,1,1)),
            (np.array([0, 0, -s-off]),  "Z-", (.5,.5,1,1)),
        ]:
            self.view3d.addItem(
                gl.GLTextItem(pos=pos, text=text, color=col))

    def _create_placeholder_cube(self):
        verts = np.array([
            [-5,-5,-5],[5,-5,-5],[5,5,-5],[-5,5,-5],
            [-5,-5, 5],[5,-5, 5],[5,5, 5],[-5,5, 5],
        ], dtype=float)
        faces = np.array([
            [0,1,2],[0,2,3],[4,5,6],[4,6,7],
            [0,1,5],[0,5,4],[2,3,7],[2,7,6],
            [1,2,6],[1,6,5],[0,3,7],[0,7,4],
        ], dtype=int)
        md = gl.MeshData(vertexes=verts, faces=faces)
        self.mesh_item = gl.GLMeshItem(
            meshdata=md, smooth=False, shader="shaded",
            drawEdges=True, edgeColor=(1,1,1,1))
        self.view3d.addItem(self.mesh_item)

    # ── STL ───────────────────────────────────────────────────────────────────
    def select_stl(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Seleccionar STL", "",
            "STL Files (*.stl);;All Files (*)")
        if path:
            self.load_stl(path)

    def load_stl(self, path: str) -> bool:
        if not os.path.exists(path):
            self.signals.status.emit(f"No existe: {path}")
            return False
        try:
            verts, faces = self._load_mesh(path)
            verts -= verts.mean(axis=0)
            span = np.max(np.ptp(verts, axis=0))
            if span > 0:
                verts *= 50.0 / span
            md = gl.MeshData(vertexes=verts, faces=faces)
            try:
                self.view3d.removeItem(self.mesh_item)
            except Exception:
                pass
            self.mesh_item = gl.GLMeshItem(
                meshdata=md, smooth=True, shader="shaded",
                drawEdges=True, edgeColor=(.2,.2,.2,1))
            self.view3d.addItem(self.mesh_item)
            self.signals.status.emit(f"STL: {Path(path).name}")
            return True
        except Exception as e:
            self.signals.status.emit(f"Error STL: {e}")
            return False

    def _load_mesh(self, path):
        if _trimesh is not None:
            try:
                tm = _trimesh.load(path, force="mesh")
                return (np.array(tm.vertices, dtype=float),
                        np.array(tm.faces, dtype=int))
            except Exception:
                pass
        if _npstl is not None:
            try:
                m = _npstl.Mesh.from_file(path)
                flat = m.vectors.reshape(-1, 3)
                v, inv = np.unique(flat, axis=0, return_inverse=True)
                return v.astype(float), inv.reshape(-1, 3).astype(int)
            except Exception:
                pass
        pts, tris = [], []
        with open(path, "r", errors="ignore") as f:
            for ln in f:
                ln = ln.strip()
                if ln.lower().startswith("vertex"):
                    p = ln.split()
                    if len(p) >= 4:
                        pts.append((float(p[1]), float(p[2]), float(p[3])))
                        if len(pts) == 3:
                            tris.append(pts[:])
                            pts = []
        if not tris:
            raise RuntimeError("No se detectaron triángulos")
        flat = np.vstack(tris)
        v, inv = np.unique(flat, axis=0, return_inverse=True)
        return v.astype(float), inv.reshape(-1, 3).astype(int)

    # ── Callbacks ─────────────────────────────────────────────────────────────
    def on_status(self, msg: str):
        self.lbl_status.setText("Status: " + msg)

    def on_gps_update(self, lat: float, lon: float, alt: float):
        self.current_lat = lat
        self.current_lon = lon
        self.current_alt = alt
        self.lbl_coords.setText(
            f"Lat: {lat:.6f}   Lon: {lon:.6f}   Alt: {alt:.1f} m")
        import json
        payload = json.dumps({
            "latitude": lat, "longitude": lon, "altitude": alt,
            "roll": self.current_roll,
            "pitch": self.current_pitch,
            "yaw": self.current_yaw,
        })
        js = (
            f"if(window.updateTelemetryFromPython){{"
            f"window.updateTelemetryFromPython({payload});"
            f"}}else if(window.updatePosition){{"
            f"window.updatePosition({lat},{lon},{alt});}}"
        )
        try:
            self.webview.page().runJavaScript(js)
        except Exception:
            pass

    def on_attitude_update(self, roll: float, pitch: float, yaw: float):
        self.current_roll  = roll
        self.current_pitch = pitch
        self.current_yaw   = yaw
        self.lbl_att.setText(
            f"Roll: {roll:.2f}°   Pitch: {pitch:.2f}°   Yaw: {yaw:.2f}°")
        try:
            self.mesh_item.resetTransform()
            self.mesh_item.rotate(yaw,   0, 0, 1, local=False)
            self.mesh_item.rotate(pitch, 0, 1, 0, local=False)
            self.mesh_item.rotate(roll,  1, 0, 0, local=False)
        except Exception:
            pass

    # ── TCP worker ────────────────────────────────────────────────────────────
    def _tcp_worker(self, host: str, port: int):
        self.signals.status.emit("Conectando TCP…")
        while True:
            try:
                with socket.create_connection((host, port), timeout=5.0) as s:
                    s.settimeout(5.0)
                    self.signals.status.emit(f"TCP {host}:{port} ✓")
                    buf = ""
                    while True:
                        try:
                            chunk = s.recv(4096)
                        except socket.timeout:
                            continue
                        if not chunk:
                            break
                        buf += chunk.decode("utf-8", errors="ignore")
                        while "\n" in buf:
                            line, buf = buf.split("\n", 1)
                            self._parse_line(line)
            except Exception as e:
                self.signals.status.emit(f"TCP error: {e}")
                time.sleep(2)

    def _parse_line(self, raw: str):
        line = raw.strip()
        if not line:
            return
        if ":" in line and line.find(":") < line.find(","):
            line = line.split(":", 1)[1].strip()
        parts = line.split(",")
        if len(parts) < 10:
            return
        try:
            lat   = float(parts[0])
            lon   = float(parts[1])
            alt   = float(parts[14]) if len(parts) > 14 and parts[14] else 0.0
            roll  = float(parts[7])  if parts[7]  else 0.0
            pitch = float(parts[8])  if parts[8]  else 0.0
            yaw   = float(parts[9])  if parts[9]  else 0.0
            self.signals.gps_update.emit(lat, lon, alt)
            self.signals.attitude_update.emit(roll, pitch, yaw)
        except Exception as e:
            print(f"Parse error: {e} | {raw[:60]}")

    def closeEvent(self, ev: QCloseEvent):
        super().closeEvent(ev)


# ═════════════════════════════════════════════════════════════════════════════
if __name__ == "__main__":
    app = QApplication(sys.argv)

    try:
        for p in get_logo_paths():
            if Path(p).exists():
                app.setWindowIcon(QIcon(str(p)))
                break
    except Exception:
        pass

    host = "127.0.0.1"
    port = 5000
    if len(sys.argv) >= 2:
        host = sys.argv[1]
    if len(sys.argv) >= 3:
        port = int(sys.argv[2])

    win = SensorClientWindow(host=host, port=port)
    win.show()
    sys.exit(app.exec())