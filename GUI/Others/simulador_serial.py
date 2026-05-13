import serial
import time
import math
import random

puerto = 'COM5'  # Cambia si usas otro puerto
baudrate = 115200

# Abrir conexión serial
ser = serial.Serial(puerto, baudrate)
time.sleep(2)

# Centro: Bogotá
lat_centro = 4.65
lon_centro = -74.1

# Radio grande para simular todo Colombia (~5 grados = ~500 km)
radio = 5.0
pasos = 360  # Número de puntos

# Variables para controlar las rotaciones de manera continua
t = 0
fase_roll = 0
fase_pitch = 0
fase_yaw = 0

while True:
    for i in range(pasos):
        angulo_rad = math.radians(i)

        # Movimiento semicircular: sube hacia la costa, baja hacia el sur
        latitude = lat_centro + radio * math.sin(angulo_rad)
        longitude = lon_centro + radio * math.cos(angulo_rad)

        # Rotaciones controladas y realistas para un vuelo
        # Roll: oscilación suave entre -30 y 30 grados (como alas balanceándose)
        roll = 25 * math.sin(math.radians(fase_roll))

        # Pitch: variación más lenta entre -15 y 15 grados (ascenso/descenso)
        pitch = 12 * math.sin(math.radians(fase_pitch * 0.7))

        # Yaw: sigue la dirección del movimiento circular + pequeñas variaciones
        yaw_direccion = (i + 90) % 360  # 90° de compensación para alinear con el movimiento
        yaw_variacion = 8 * math.sin(math.radians(fase_yaw * 0.5))
        yaw = (yaw_direccion + yaw_variacion) % 360

        # Incrementar fases para animación continua
        fase_roll += 3.0  # Roll más rápido
        fase_pitch += 1.5  # Pitch más lento
        fase_yaw += 2.0  # Yaw medio

        # Mantener las fases en rango razonable
        if fase_roll > 360:
            fase_roll -= 360
        if fase_pitch > 360:
            fase_pitch -= 360
        if fase_yaw > 360:
            fase_yaw -= 360

        # Incrementar tiempo general
        t += 1

        # Datos simulados
        fecha = "2025-06-24"
        hora = "12:34:56"
        secs = round((i * 0.1) % 60, 2)
        satelites = random.randint(6, 12)
        hdop = round(random.uniform(0.5, 2.0), 2)
        servo1 = round(random.uniform(0, 180), 2)
        servo2 = round(random.uniform(0, 180), 2)
        servo3 = round(random.uniform(0, 180), 2)
        servo4 = round(random.uniform(0, 180), 2)
        alt_diff = round(random.uniform(-5, 5), 2)
        pressure = round(random.uniform(950, 1050), 2)
        temperature = round(random.uniform(20, 35), 2)

        # Línea completa con 17 campos
        linea = f"{latitude},{longitude},{fecha},{hora},{secs},{satelites},{hdop},{roll:.2f},{pitch:.2f},{yaw:.2f},{servo1},{servo2},{servo3},{servo4},{alt_diff},{pressure},{temperature}\r\n"

        # Enviar al COM
        ser.write(linea.encode())
        print(f"Enviado: Lat:{latitude:.4f}, Lon:{longitude:.4f}, Roll:{roll:.2f}, Pitch:{pitch:.2f}, Yaw:{yaw:.2f}")

        # Espera para simular tiempo real
        time.sleep(0.2)