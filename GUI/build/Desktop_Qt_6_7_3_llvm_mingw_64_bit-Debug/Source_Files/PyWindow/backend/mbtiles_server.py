import sqlite3
import os

class MBTilesReader:
    def __init__(self, mbtiles_path):
        self.mbtiles_path = mbtiles_path
        self._conn = None
        print(f"DEBUG MBTiles: Intentando abrir {mbtiles_path}")
        if os.path.exists(mbtiles_path):
            try:
                self._conn = sqlite3.connect(mbtiles_path, check_same_thread=False)
                print(f"DEBUG MBTiles: Conectado exitosamente")
                # Verificar info de la BD
                cursor = self._conn.cursor()
                cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
                tables = cursor.fetchall()
                print(f"DEBUG MBTiles: Tablas encontradas: {tables}")
                cursor.execute("SELECT min(zoom_level), max(zoom_level) FROM tiles;")
                zoom_range = cursor.fetchone()
                print(f"DEBUG MBTiles: Rango de zoom: {zoom_range}")
            except sqlite3.Error as e:
                print(f"DEBUG MBTiles: Error opening MBTiles: {e}")
        else:
            print(f"DEBUG MBTiles: Archivo NO encontrado en {mbtiles_path}")

    def get_tile(self, z, x, y):
        """
        Gets tile data for XYZ coordinates.
        MBTiles stores tiles in TMS coordinates, so we convert y.
        """
        print(f"DEBUG get_tile: Solicitando tile z={z}, x={x}, y={y}")
        
        if not self._conn:
            print(f"DEBUG get_tile: No hay conexión a BD")
            return None

        # Convert XYZ y to TMS y
        tms_y = (1 << z) - 1 - y
        print(f"DEBUG get_tile: Convertido a TMS - tms_y={tms_y}")

        try:
            cursor = self._conn.cursor()
            cursor.execute(
                "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?",
                (z, x, tms_y)
            )
            row = cursor.fetchone()
            if row:
                print(f"DEBUG get_tile: ✅ Tile encontrado, tamaño: {len(row[0])} bytes")
                return row[0]
            else:
                print(f"DEBUG get_tile: ❌ Tile NO encontrado en BD para z={z}, x={x}, tms_y={tms_y}")
        except sqlite3.Error as e:
            print(f"DEBUG get_tile: Error reading tile (z={z}, x={x}, y={y}): {e}")

        return None

    def close(self):
        if self._conn:
            self._conn.close()
            self._conn = None
