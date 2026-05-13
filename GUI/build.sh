#!/bin/bash

# Directorio donde se ejecuta el script
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Crear carpeta build si no existe
mkdir -p "$PROJECT_DIR/build"

# Entrar a la carpeta build
cd "$PROJECT_DIR/build"

# Ejecutar CMake para configurar el proyecto
cmake ..

# Compilar el proyecto
cmake --build .

# Mensaje de éxito
echo "✅ Compilación de CMake completa."
