import sqlite3
import os

class MBTilesReader:
    def __init__(self, mbtiles_path):
        self.mbtiles_path = mbtiles_path
        self._conn = None
        if os.path.exists(mbtiles_path):
            try:
                self._conn = sqlite3.connect(mbtiles_path, check_same_thread=False)
            except sqlite3.Error as e:
                print(f"Error opening MBTiles: {e}")
        else:
            print(f"MBTiles file not found at: {mbtiles_path}")

    def get_tile(self, z, x, y):
        """
        Gets tile data for XYZ coordinates.
        MBTiles stores tiles in TMS coordinates, so we convert y.
        """
        if not self._conn:
            return None

        # Convert XYZ y to TMS y
        tms_y = (1 << z) - 1 - y

        try:
            cursor = self._conn.cursor()
            cursor.execute(
                "SELECT tile_data FROM tiles WHERE zoom_level = ? AND tile_column = ? AND tile_row = ?",
                (z, x, tms_y)
            )
            row = cursor.fetchone()
            if row:
                return row[0]
        except sqlite3.Error as e:
            print(f"Error reading tile (z={z}, x={x}, y={y}): {e}")

        return None

    def close(self):
        if self._conn:
            self._conn.close()
            self._conn = None
