from flask import Flask, render_template, send_from_directory, Response
import os
import sys

# Ensure backend can be imported
sys.path.append(os.path.dirname(os.path.dirname(__file__)))
from backend.mbtiles_server import MBTilesReader

def create_app(mbtiles_path, static_folder, template_folder):
    app = Flask(__name__, static_folder=static_folder, template_folder=template_folder)
    reader = MBTilesReader(mbtiles_path)

    @app.route('/')
    def index():
        return render_template('index.html')

    @app.route('/tile/<int:z>/<int:x>/<int:y>')
    def get_tile(z, x, y):
        tile_data = reader.get_tile(z, x, y)
        if tile_data:
            return Response(tile_data, mimetype='image/png')
        else:
            return "Tile not found", 404

    @app.route('/static/Cesium/<path:path>')
    def serve_cesium(path):
        # Allow serving Cesium from a standard location if it exists
        cesium_path = os.path.join(static_folder, 'Cesium')
        return send_from_directory(cesium_path, path)

    return app
