import socket
import time

def verify_subscriber():
    host = '127.0.0.1'
    port = 5000

    print(f"Intentando conectar a {host}:{port}...")
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5)
            s.connect((host, port))
            print("Conectado exitosamente al tópico de datos.")
            print("Esperando datos (Presiona Ctrl+C para salir)...")

            while True:
                data = s.recv(1024)
                if not data:
                    print("Servidor cerró la conexión.")
                    break
                print(f"Recibido: {data.decode('utf-8').strip()}")
    except ConnectionRefusedError:
        print("Error: Conexión rechazada. Asegúrate de que la aplicación C++ esté corriendo.")
    except socket.timeout:
        print("Error: Tiempo de espera agotado al intentar conectar.")
    except Exception as e:
        print(f"Ocurrió un error: {e}")

if __name__ == "__main__":
    verify_subscriber()
