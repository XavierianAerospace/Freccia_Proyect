import argparse
import json
import os
import shutil
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

TILESERVER_HOST = os.environ.get("FRECCIA_TILESERVER_HOST", "127.0.0.1")
TILESERVER_PORT = int(os.environ.get("FRECCIA_TILESERVER_PORT", "8080"))
TILESERVER_STYLE = os.environ.get("FRECCIA_TILESERVER_STYLE", "basic-preview")
FLASK_PORT = int(os.environ.get("FRECCIA_FLASK_PORT", "5001"))
TELEMETRY_HOST = os.environ.get("FRECCIA_TCP_HOST", "127.0.0.1")
TELEMETRY_PORT = int(os.environ.get("FRECCIA_TCP_PORT", "5000"))

DEFAULT_MBTILES_NAME = "maptiler-osm-2020-02-10-v3.11-planet.mbtiles"
TILESERVER_STARTUP_TIMEOUT_SECONDS = 120
FLASK_STARTUP_TIMEOUT_SECONDS = 30


def run_flask(app):
    app.run(host=TILESERVER_HOST, port=FLASK_PORT, threaded=True, debug=False, use_reloader=False)


def parse_args():
    parser = argparse.ArgumentParser(description="Freccia offline Cesium map viewer")
    parser.add_argument(
        "--headless",
        action="store_true",
        help="Run Flask + TileServer-GL without opening the pywebview window.",
    )
    return parser.parse_args()


def is_port_open(host, port):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(1)
        return sock.connect_ex((host, port)) == 0


def wait_for_port(host, port, timeout_seconds, service_name):
    deadline = time.time() + timeout_seconds
    while time.time() < deadline:
        if is_port_open(host, port):
            return
        time.sleep(0.5)

    raise TimeoutError(f"{service_name} no quedo listo en {host}:{port}.")


def read_log_tail(log_path, max_lines=20):
    if not log_path.exists():
        return "No se genero archivo de log."

    try:
        lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError as exc:
        return f"No se pudo leer el log: {exc}"

    if not lines:
        return "El log esta vacio."

    return "\n".join(lines[-max_lines:])


def resolve_mbtiles_path(base_dir):
    maps_dir = Path(base_dir) / "maps"
    configured_path = os.environ.get("FRECCIA_MBTILES_PATH")

    candidates = []
    if configured_path:
        candidates.append(Path(configured_path).expanduser())

    candidates.append(maps_dir / DEFAULT_MBTILES_NAME)
    candidates.extend(sorted(maps_dir.glob("*.mbtiles")))

    seen = set()
    for candidate in candidates:
        candidate = candidate.expanduser()
        if candidate in seen:
            continue
        seen.add(candidate)

        if candidate.is_file():
            return candidate.resolve()

    searched = "\n  - ".join(str(candidate) for candidate in seen)
    raise FileNotFoundError(
        "No se encontro un archivo .mbtiles valido.\n"
        f"Coloca el archivo dentro de: {maps_dir}\n"
        "Tambien puedes definir FRECCIA_MBTILES_PATH con una ruta completa.\n"
        f"Rutas revisadas:\n  - {searched}"
    )


def resolve_tileserver_command():
    configured_path = os.environ.get("TILESERVER_GL_BIN")
    candidate_commands = []

    if configured_path:
        candidate_commands.append([configured_path])

    for command_name in ("tileserver-gl.cmd", "tileserver-gl", "tileserver-gl.ps1"):
        command_path = shutil.which(command_name)
        if command_path:
            candidate_commands.append([command_path])

    appdata = os.environ.get("APPDATA")
    if appdata:
        npm_dir = Path(appdata) / "npm"
        for command_name in ("tileserver-gl.cmd", "tileserver-gl", "tileserver-gl.ps1"):
            command_path = npm_dir / command_name
            if command_path.exists():
                candidate_commands.append([str(command_path)])

    npx_path = shutil.which("npx.cmd") or shutil.which("npx")
    if npx_path:
        candidate_commands.append([npx_path, "tileserver-gl"])

    if appdata:
        npm_npx = Path(appdata) / "npm" / "npx.cmd"
        if npm_npx.exists():
            candidate_commands.append([str(npm_npx), "tileserver-gl"])

    powershell_path = shutil.which("powershell.exe") or shutil.which("pwsh.exe")

    deduped_commands = []
    seen = set()
    for command in candidate_commands:
        normalized = tuple(command)
        if normalized in seen:
            continue
        seen.add(normalized)

        executable = Path(command[0])
        if executable.suffix.lower() == ".ps1" and powershell_path:
            deduped_commands.append(
                [powershell_path, "-ExecutionPolicy", "Bypass", "-File", str(executable)]
            )
        else:
            deduped_commands.append(command)

    if deduped_commands:
        return deduped_commands[0]

    searched_paths = []
    if appdata:
        npm_dir = Path(appdata) / "npm"
        searched_paths.extend(
            [
                npm_dir / "tileserver-gl.cmd",
                npm_dir / "tileserver-gl.ps1",
                npm_dir / "npx.cmd",
            ]
        )

    searched_text = "\n  - ".join(str(path) for path in searched_paths) or "No hubo rutas adicionales."
    raise FileNotFoundError(
        "No se encontro TileServer-GL.\n"
        "Instalalo con: npm install -g tileserver-gl\n"
        "Opcionalmente define TILESERVER_GL_BIN con la ruta exacta al ejecutable.\n"
        f"Rutas revisadas:\n  - {searched_text}"
    )


def build_tileserver_env():
    env = os.environ.copy()
    extra_paths = []

    appdata = env.get("APPDATA")
    if appdata:
        npm_dir = Path(appdata) / "npm"
        if npm_dir.exists():
            extra_paths.append(str(npm_dir))

    node_path = shutil.which("node.exe") or shutil.which("node")
    if node_path:
        extra_paths.append(str(Path(node_path).parent))

    if extra_paths:
        current_path = env.get("PATH", "")
        env["PATH"] = os.pathsep.join(extra_paths + [current_path])

    env["FRECCIA_TILESERVER_HOST"] = TILESERVER_HOST
    env["FRECCIA_TILESERVER_PORT"] = str(TILESERVER_PORT)
    env["FRECCIA_TILESERVER_STYLE"] = TILESERVER_STYLE
    return env


def wait_for_tileserver(process, log_path):
    deadline = time.time() + TILESERVER_STARTUP_TIMEOUT_SECONDS
    while time.time() < deadline:
        if is_port_open(TILESERVER_HOST, TILESERVER_PORT):
            return

        exit_code = process.poll()
        if exit_code is not None:
            log_tail = read_log_tail(log_path)
            raise RuntimeError(
                "TileServer-GL termino antes de abrir el puerto configurado.\n"
                f"Codigo de salida: {exit_code}\n"
                f"Log:\n{log_tail}"
            )

        time.sleep(1)

    raise TimeoutError(
        "TileServer-GL no quedo listo a tiempo.\n"
        f"Revisa el log en: {log_path}"
    )


def run_tileserver(base_dir, mbtiles_path):
    maps_dir = Path(base_dir) / "maps"
    log_path = Path(base_dir) / "tileserver-gl.log"
    endpoint = f"http://{TILESERVER_HOST}:{TILESERVER_PORT}"

    print("\n" + "=" * 60)
    print("INICIANDO TILESERVER-GL")
    print("=" * 60)
    print(f"Archivo MBTiles: {mbtiles_path}")
    print(f"Carpeta maps: {maps_dir}")
    print(f"Endpoint: {endpoint}")
    print(f"Estilo raster: {TILESERVER_STYLE}")
    print("=" * 60 + "\n")

    if is_port_open(TILESERVER_HOST, TILESERVER_PORT):
        print(
            f"TileServer-GL ya estaba respondiendo en {endpoint}. "
            "Se reutilizara esa instancia.\n"
        )
        return None

    command = resolve_tileserver_command()
    command_line = command + [
        str(mbtiles_path),
        "--bind",
        TILESERVER_HOST,
        "--port",
        str(TILESERVER_PORT),
    ]

    creationflags = 0
    if os.name == "nt":
        creationflags = subprocess.CREATE_NO_WINDOW

    with log_path.open("w", encoding="utf-8") as log_file:
        process = subprocess.Popen(
            command_line,
            cwd=base_dir,
            stdout=log_file,
            stderr=subprocess.STDOUT,
            env=build_tileserver_env(),
            creationflags=creationflags,
        )

    wait_for_tileserver(process, log_path)
    print(f"TileServer-GL iniciado en background. Log: {log_path}\n")
    return process


def normalize_telemetry_line(raw_line):
    line = raw_line.strip()
    if not line:
        return ""

    if ":" in line and "," in line and line.find(":") < line.find(","):
        line = line.split(":", 1)[1].strip()

    return line


def parse_telemetry_line(raw_line):
    line = normalize_telemetry_line(raw_line)
    if not line:
        return None

    parts = line.split(",")
    if len(parts) < 15:
        return None

    try:
        payload = {
            "latitude": float(parts[0]),
            "longitude": float(parts[1]),
            "date": parts[2],
            "utc_time": parts[3],
            "seconds": float(parts[4]),
            "satellites": int(float(parts[5])),
            "hdop": float(parts[6]),
            "roll": float(parts[7]),
            "pitch": float(parts[8]),
            "yaw": float(parts[9]),
            "servo1": float(parts[10]),
            "servo2": float(parts[11]),
            "servo3": float(parts[12]),
            "servo4": float(parts[13]),
            "altitude": float(parts[14]),
        }
    except (TypeError, ValueError):
        return None

    if len(parts) >= 17:
        try:
            payload["pressure"] = float(parts[15])
            payload["temperature"] = float(parts[16])
        except (TypeError, ValueError):
            pass

    return payload


def push_telemetry_to_window(window, payload):
    payload_json = json.dumps(payload, ensure_ascii=True, separators=(",", ":"))
    script = (
        f"if (window.updateTelemetryFromPython) {{ "
        f"window.updateTelemetryFromPython({payload_json}); "
        f"}} else if (window.updatePosition) {{ "
        f"window.updatePosition({payload['latitude']}, {payload['longitude']}, {payload['altitude']}); "
        f"}}"
    )

    try:
        window.evaluate_js(script)
    except Exception:
        # pywebview may still be loading; ignore until the next telemetry frame.
        pass


def tcp_worker(window):
    print(f"TCP Worker: preparando conexion a {TELEMETRY_HOST}:{TELEMETRY_PORT}...")
    time.sleep(2)

    while True:
        try:
            with socket.create_connection((TELEMETRY_HOST, TELEMETRY_PORT), timeout=5.0) as sock:
                print(f"TCP Worker: conectado a {TELEMETRY_HOST}:{TELEMETRY_PORT}")
                sock.settimeout(5.0)
                buffer = ""

                while True:
                    try:
                        chunk = sock.recv(4096)
                    except socket.timeout:
                        continue

                    if not chunk:
                        print("TCP Worker: conexion cerrada por el emisor. Reintentando...")
                        break

                    buffer += chunk.decode("utf-8", errors="ignore")

                    while "\n" in buffer:
                        line, buffer = buffer.split("\n", 1)
                        payload = parse_telemetry_line(line)
                        if payload:
                            push_telemetry_to_window(window, payload)

                if buffer.strip():
                    payload = parse_telemetry_line(buffer)
                    if payload:
                        push_telemetry_to_window(window, payload)

        except Exception as exc:
            print(f"TCP Worker Error: {exc}. Reintentando en 2s...")
            time.sleep(2)


def stop_background_process(process, label):
    if process is None:
        return

    if process.poll() is not None:
        return

    print(f"Cerrando {label}...")
    process.terminate()
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()


def main():
    from backend.server import create_app

    args = parse_args()
    headless_mode = args.headless or os.environ.get("FRECCIA_HEADLESS") == "1"
    base_dir = Path(__file__).resolve().parent
    tileserver_process = None

    print("\n" + "=" * 60)
    print("FRECCIA XAE - MAPA 3D CESIUM + OPENFREEMAP")
    print("=" * 60)

    try:
        mbtiles_path = resolve_mbtiles_path(base_dir)
        tileserver_process = run_tileserver(base_dir, mbtiles_path)

        static_folder = base_dir / "cesium_app" / "static"
        template_folder = base_dir / "cesium_app" / "templates"
        app = create_app(str(mbtiles_path), str(static_folder), str(template_folder))

        server_thread = threading.Thread(target=run_flask, args=(app,), daemon=True)
        server_thread.start()
        wait_for_port(TILESERVER_HOST, FLASK_PORT, FLASK_STARTUP_TIMEOUT_SECONDS, "Flask")

        print(f"Flask iniciado en http://{TILESERVER_HOST}:{FLASK_PORT}")
        print("\n" + "=" * 60)
        print("ARQUITECTURA ACTIVA:")
        print("=" * 60)
        print("Vector MBTiles")
        print("         ->")
        print(f"TileServer-GL ({TILESERVER_HOST}:{TILESERVER_PORT})")
        print("         ->")
        print("Raster PNG Tiles")
        print("         ->")
        print(f"Cesium Globe ({TILESERVER_HOST}:{FLASK_PORT})")
        print("         ->")
        print(f"TCP Telemetria ({TELEMETRY_HOST}:{TELEMETRY_PORT})")
        print("=" * 60 + "\n")

        if headless_mode:
            print("Modo headless activo. Backend listo sin abrir pywebview.\n")
            try:
                while True:
                    time.sleep(1)
            except KeyboardInterrupt:
                print("Cerrando backend headless...")
            return

        import webview

        print("Abriendo ventana PyWindow...\n")
        window = webview.create_window(
            "FRECCIA_XAE - MAPA 3D CESIUM",
            f"http://{TILESERVER_HOST}:{FLASK_PORT}",
            width=1400,
            height=900,
            background_color="#1e1e1e",
        )

        telemetry_thread = threading.Thread(target=tcp_worker, args=(window,), daemon=True)
        telemetry_thread.start()

        webview.start(debug=False)
    except Exception as exc:
        print(f"\nERROR: {exc}")
        sys.exit(1)
    finally:
        stop_background_process(tileserver_process, "TileServer-GL")


if __name__ == "__main__":
    main()
