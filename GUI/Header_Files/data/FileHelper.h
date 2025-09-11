#ifndef FILEHELPER_H
#define FILEHELPER_H

#include "SensorData.h"

#include <string>
#include <vector>
#include <fstream>
#include <QTime>

// Nota: Los headers de Qt usados SOLO en el .cpp (QFile, QTextStream, QDateTime, QString, QIODevice, QDebug)
// NO son necesarios aquí. Inclúyelos en FileHelper.cpp.

class FileHelper {
public:
    // ===== Utilidades de archivos (estáticas) =====
    static bool exists(const std::string& path);
    static void ensureExists(const std::string& path);
    static void writeHeaderIfNew(const std::string& path, const std::string& header);
    static void createDataDirectoryIfNeeded();

    // ===== Persistencia por evento (estáticas) =====
    // Escribe EXACTAMENTE 1 registro en 'path' (p.ej. ../data/raw_data.csv).
    static void appendRawData(const std::string& path, const SensorData& data);

    // Agrega múltiples registros "limpios" en un solo batch.
    static void appendCleanData(const std::string& path, const std::vector<SensorData>& data);

    // Registra un dato con su detalle de error en el archivo de errores.
    static void appendErrorData(const std::string& path, const SensorData& data, const std::string& errorDetail);

    // ===== Grabación por sesión (de instancia) =====
    // Abre/crea un archivo de sesión con encabezado.
    void iniciarGrabacion();
    // Agrega un registro a la sesión abierta (si la hay).
    void escribirDuranteGrabacion(const SensorData& d);
    // Cierra la sesión y antepone la duración al archivo.
    void detenerGrabacion();

    // Exponemos el tiempo acumulado de la sesión (lo manipula el .cpp).
    QTime tiempoGrabado;

private:
    // Estado de la grabación por sesión (propio del objeto)
    std::ofstream archivoGrabacion;
    std::string   rutaArchivoActual;
};

#endif // FILEHELPER_H
