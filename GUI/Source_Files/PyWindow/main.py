import webview
import threading
import socket
import os
import sys
import time
from backend.server import create_app

def tcp_worker(window):
    host = "127.0.0.1"
    port = 5000
    print(f"TCP Worker: Connecting to {host}:{port}...")
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(5.0)
                s.connect((host, port))
                print(f"TCP Worker: Connected to {host}:{port}")
                s.settimeout(None)
                while True:
                    data = s.recv(4096)
                    if not data: break
                    lines = data.decode("utf-8", errors="ignore").splitlines()
                    for line in lines:
                        parse_and_update(line, window)
        except Exception as e:
            print(f"TCP Worker Error: {e}. Retrying in 5s...")
            time.sleep(5.0)

def parse_and_update(line, window):
    line = line.strip()
    if not line: return

    # Clean ID prefix if present
    if ":" in line and "," in line and line.find(":") < line.find(","):
         line = line.split(":", 1)[1].strip()

    parts = line.split(",")
    if len(parts) >= 15:
        try:
            lat = float(parts[0])
            lon = float(parts[1])
            alt = float(parts[14])
            # Call JavaScript function in Cesium
            window.evaluate_js(f"if(window.updatePosition) {{ window.updatePosition({lat}, {lon}, {alt}); }}")
        except Exception as e:
            # Silently fail on parse errors
            pass

def run_flask(app):
    app.run(port=5001, threaded=True)

if __name__ == "__main__":
    # Base paths
    base_dir = os.path.dirname(os.path.abspath(__file__))
    mbtiles_path = os.path.join(base_dir, "maptiler-osm-2020-02-10-v3.11-planet.mbtiles")
    static_folder = os.path.join(base_dir, "cesium_app", "static")
    template_folder = os.path.join(base_dir, "cesium_app", "templates")

    # Initialize Flask app
    app = create_app(mbtiles_path, static_folder, template_folder)

    # Start Flask in a background thread
    server_thread = threading.Thread(target=run_flask, args=(app,), daemon=True)
    server_thread.start()

    # Wait a bit for the server to start
    time.sleep(1)

    # Create pywebview window
    window = webview.create_window('FRECCIA_XAE - MAP VIEWER (Cesium 3D)', 'http://127.0.0.1:5001')

    # Start TCP worker in a background thread
    telemetry_thread = threading.Thread(target=tcp_worker, args=(window,), daemon=True)
    telemetry_thread.start()

    # Start pywebview
    webview.start()
