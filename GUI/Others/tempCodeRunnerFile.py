import serial
import time
import math
import random

puerto = 'COM7'  # Cambia si usas otro puerto
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

while True:
    for i in range(pasos):
        angulo_rad = math.radians(i)

        # Movimiento semicircular: sube hacia la costa, baja hacia el sur
        latitude = lat_centro + radio * math.sin(angulo_rad)
        longitude = lon_centro + radio * math.cos(angulo_rad)

        # Datos simulados
        fecha = "2025-06-24"
        hora = "12:34:56"
        secs = round((i * 0.1) % 60, 2)
        satelites = random.randint(6, 12)
        hdop = round(random.uniform(0.5, 2.0), 2)
        roll = round(random.uniform(-30, 30), 2)
        pitch = round(random.uniform(-30, 30), 2)
        yaw = round(random.uniform(0, 360), 2)
        servo1 = round(random.uniform(0, 180), 2)
        servo2 = round(random.uniform(0, 180), 2)
        servo3 = round(random.uniform(0, 180), 2)
        servo4 = round(random.uniform(0, 180), 2)
        alt_diff = round(random.uniform(-5, 5), 2)
        pressure = round(random.uniform(950, 1050), 2)
        temperature = round(random.uniform(20, 35), 2)

        # Línea completa con 17 campos
        linea = f"{latitude},{longitude},{fecha},{hora},{secs},{satelites},{hdop},{roll},{pitch},{yaw},{servo1},{servo2},{servo3},{servo4},{alt_diff},{pressure},{temperature}\r\n"

        # Enviar al COM
        ser.write(linea.encode())
        print("Enviado:", linea.strip())

        # Espera para simular tiempo real
        time.sleep(0.2)
