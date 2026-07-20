import socket
import time
import random

def mock_telemetry_server():
    host = '127.0.0.1'
    port = 5000

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen()
        print(f"Mock Telemetry Server listening on {host}:{port}")

        while True:
            conn, addr = s.accept()
            with conn:
                print(f"Connected by {addr}")
                lat, lon, alt = 4.6286, -74.0647, 2600.0
                while True:
                    try:
                        # Format: lat, lon, date, time, secs, sats, hdop, roll, pitch, yaw, s1, s2, s3, s4, alt_diff
                        data = f"{lat:.8f},{lon:.8f},2023-10-27,12:00:00,1.0,10,1.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,{alt:.2f}\n"
                        conn.sendall(data.encode('utf-8'))
                        print(f"Sent: {data.strip()}")

                        # Simulate movement
                        lat += 0.0001
                        lon += 0.0001
                        alt += 0.1

                        time.sleep(1.0)
                    except (BrokenPipeError, ConnectionResetError):
                        print("Client disconnected")
                        break

if __name__ == "__main__":
    mock_telemetry_server()
