from flask import Flask, redirect, render_template, send_from_directory
import os


def create_app(mbtiles_path, static_folder, template_folder):
    del mbtiles_path

    app = Flask(__name__, static_folder=static_folder, template_folder=template_folder)
    tileserver_host = os.environ.get("FRECCIA_TILESERVER_HOST", "127.0.0.1")
    tileserver_port = os.environ.get("FRECCIA_TILESERVER_PORT", "8080")
    tileserver_style = os.environ.get("FRECCIA_TILESERVER_STYLE", "basic-preview")
    tileserver_base_url = f"http://{tileserver_host}:{tileserver_port}/styles/{tileserver_style}"
    cesium_ion_token = os.environ.get("CESIUM_ION_TOKEN", "")

    @app.route("/")
    def index():
        return render_template(
            "index.html",
            tileserver_base_url=tileserver_base_url,
            cesium_ion_token=cesium_ion_token
        )

    @app.route("/tile/<int:z>/<int:x>/<int:y>")
    def get_tile(z, x, y):
        return redirect(f"{tileserver_base_url}/{z}/{x}/{y}.png", code=302)

    @app.route("/static/Cesium/<path:path>")
    def serve_cesium(path):
        cesium_path = os.path.join(static_folder, "Cesium")
        return send_from_directory(cesium_path, path)

    return app
