import sys
import os
import socket
import threading
from pathlib import Path

from PyQt6.QtCore import (
    QUrl, pyqtSignal, QObject, pyqtSlot, QIODevice,
    QByteArray, QFile, QBuffer
)
from PyQt6.QtWidgets import QApplication, QMainWindow, QVBoxLayout, QWidget
from PyQt6.QtWebEngineWidgets import QWebEngineView
from PyQt6.QtWebEngineCore import (
    QWebEngineProfile,
    QWebEngineUrlSchemeHandler,
    QWebEngineUrlRequestJob,
    QWebEngineUrlScheme,
    QWebEngineSettings
)

# Internal imports
try:
    from backend.mbtiles_server import MBTilesReader
except ImportError:
    # Handle cases where the execution context might differ
    sys.path.append(os.path.dirname(__file__))
    from backend.mbtiles_server import MBTilesReader

# Register the custom scheme 'mbtiles'
# This must be done BEFORE the QApplication is created.
scheme_name = b"mbtiles"
if not QWebEngineUrlScheme.schemeByName(QByteArray(scheme_name)).name():
    scheme = QWebEngineUrlScheme(QByteArray(scheme_name))
    scheme.setFlags(QWebEngineUrlScheme.Flag.Local | QWebEngineUrlScheme.Flag.Content)
    QWebEngineUrlScheme.registerScheme(scheme)

class MBTilesHandler(QWebEngineUrlSchemeHandler):
    def __init__(self, mbtiles_path, app_dir, parent=None):
        super().__init__(parent)
        self.reader = MBTilesReader(mbtiles_path)
        self.app_dir = app_dir

    def requestStarted(self, job: QWebEngineUrlRequestJob):
        url = job.requestUrl()
        path = url.path()

        # Handle tile requests: mbtiles://local/tile/{z}/{x}/{y}
        if path.startswith("/tile/"):
            parts = path.strip("/").split("/")
            if len(parts) == 4:
                try:
                    z, x, y = map(int, parts[1:])
                    tile_data = self.reader.get_tile(z, x, y)
                    if tile_data:
                        buf = QBuffer(job)
                        buf.setData(tile_data)
                        buf.open(QIODevice.OpenModeFlag.ReadOnly)
                        job.reply(b"image/png", buf)
                        return
                except Exception as e:
                    print(f"Error handling tile request: {e}")

        # Handle local assets (CesiumJS, etc.): mbtiles://local/Cesium/...
        # Or mbtiles://local/script.js
        local_file_path = os.path.join(self.app_dir, path.lstrip("/"))

        if os.path.exists(local_file_path) and not os.path.isdir(local_file_path):
            file = QFile(local_file_path, job) # Parent to job for lifetime management
            if file.open(QIODevice.OpenModeFlag.ReadOnly):
                mime = b"text/plain"
                if local_file_path.endswith(".js"): mime = b"application/javascript"
                elif local_file_path.endswith(".css"): mime = b"text/css"
                elif local_file_path.endswith(".html"): mime = b"text/html"
                elif local_file_path.endswith(".png"): mime = b"image/png"
                elif local_file_path.endswith(".jpg") or local_file_path.endswith(".jpeg"): mime = b"image/jpeg"
                elif local_file_path.endswith(".wasm"): mime = b"application/wasm"

                job.reply(mime, file)
                return

        job.fail(QWebEngineUrlRequestJob.Error.UrlNotFound)

class TelemetrySignals(QObject):
    update_gps = pyqtSignal(float, float, float) # lat, lon, alt

class MapWindow(QMainWindow):
    def __init__(self, mbtiles_path):
        super().__init__()
        self.setWindowTitle("FRECCIA_XAE - MAP VIEWER")
        self.resize(1200, 800)

        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)
        self.layout = QVBoxLayout(self.central_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)

        # Ensure profile exists
        self.profile = QWebEngineProfile.defaultProfile()

        # Setup custom scheme handler
        self.app_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "cesium_app"))
        self.handler = MBTilesHandler(mbtiles_path, self.app_dir, self)
        self.profile.installUrlSchemeHandler(b"mbtiles", self.handler)

        self.browser = QWebEngineView(self.central_widget)
        self.layout.addWidget(self.browser)

        # Enable settings for Cesium
        settings = self.browser.settings()
        settings.setAttribute(QWebEngineSettings.WebAttribute.LocalContentCanAccessRemoteUrls, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.Accelerated2dCanvasEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.WebGLEnabled, True)
        settings.setAttribute(QWebEngineSettings.WebAttribute.JavascriptEnabled, True)

        # Load index.html via the custom scheme
        self.browser.setUrl(QUrl("mbtiles://local/index.html"))

        # Telemetry signals
        self.signals = TelemetrySignals()
        self.signals.update_gps.connect(self.update_map)

        # Start TCP client thread
        self.tcp_thread = threading.Thread(target=self.tcp_worker, daemon=True)
        self.tcp_thread.start()

    @pyqtSlot(float, float, float)
    def update_map(self, lat, lon, alt):
        # Update Cesium position
        js = f"if(window.updatePosition) {{ window.updatePosition({lat}, {lon}, {alt}); }}"
        self.browser.page().runJavaScript(js)

    def tcp_worker(self):
        host = "127.0.0.1"
        port = 5000
        while True:
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.settimeout(5.0)
                    s.connect((host, port))
                    print(f"Connected to telemetry on {host}:{port}")
                    s.settimeout(None)
                    while True:
                        data = s.recv(4096)
                        if not data: break
                        lines = data.decode("utf-8", errors="ignore").splitlines()
                        for line in lines:
                            self.parse_line(line)
            except Exception as e:
                print(f"TCP Error: {e}. Retrying in 5s...")
                import time
                time.sleep(5.0)

    def parse_line(self, line):
        line = line.strip()
        if not line: return

        # Clean ID prefix if present (e.g. #6456: ...)
        if ":" in line and "," in line and line.find(":") < line.find(","):
             line = line.split(":", 1)[1].strip()

        parts = line.split(",")
        if len(parts) >= 15:
            try:
                # Format: lat, lon, date, time, secs, sats, hdop, roll, pitch, yaw, s1, s2, s3, s4, alt_diff, ...
                lat = float(parts[0])
                lon = float(parts[1])
                alt = float(parts[14])
                self.signals.update_gps.emit(lat, lon, alt)
            except ValueError:
                pass

if __name__ == "__main__":
    # Disable sandbox for WebEngine on some Linux environments
    os.environ["QTWEBENGINE_DISABLE_SANDBOX"] = "1"

    app = QApplication(sys.argv)

    # Path to the planet mbtiles file
    mbtiles_path = os.path.join(os.path.dirname(__file__), "maptiler-osm-2020-02-10-v3.11-planet.mbtiles")

    window = MapWindow(mbtiles_path)
    window.show()
    sys.exit(app.exec())
