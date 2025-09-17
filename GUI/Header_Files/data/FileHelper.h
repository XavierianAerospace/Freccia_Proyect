#ifndef FILEHELPER_H
#define FILEHELPER_H

#include "SensorData.h"

#include <string>
#include <vector>
#include <fstream>
#include <QTime>

class FileHelper {
public:
    // ===== Utilidades de archivos (estáticas) =====
    static bool exists(const std::string& path);
    static void ensureExists(const std::string& path);
    static void writeHeaderIfNew(const std::string& path, const std::string& header);
    static void createDataDirectoryIfNeeded();

    // ===== Persistencia por evento (estáticas) =====
    static void appendRawData(const std::string& path, const SensorData& data);
    static void appendCleanData(const std::string& path, const std::vector<SensorData>& data);
    static void appendErrorData(const std::string& path, const SensorData& data, const std::string& errorDetail);

    // ===== Grabación por sesión (REC) =====
    void iniciarGrabacion();
    void escribirDuranteGrabacion(const SensorData& d);
    void detenerGrabacion();

    // ===== Sesión de datos limpios (CLEAN por sesión) =====
    // Crea archivo de sesión con encabezado usando la fecha/hora absolutas de inicio.
    void iniciarSesionLimpios(const std::string& fechaISO, const std::string& horaISO);
    // Escribe una fila: Hora(tRel), Lat, Lon, FechaInicio, HoraInicio, ...
    // (firma usada por DataCleaner.cpp)
    void escribirLimpioDuranteSesion(double tRel,
                                     const SensorData& d,
                                     bool primerRegistro,
                                     const std::string& fechaISO,
                                     const std::string& horaISO);
    // Cierra archivo de sesión CLEAN
    void cerrarSesionLimpios();

    // Tiempo acumulado de la sesión de grabación (REC)
    QTime tiempoGrabado;

private:
    // Estado de la grabación por sesión (REC)
    std::ofstream archivoGrabacion;
    std::string   rutaArchivoActual;

    // Estado de la sesión CLEAN
    std::ofstream archivoCleanSesion;
    std::string   rutaCleanSesion;
    std::string   fechaPrimeraSesionISO;
    std::string   horaPrimeraSesionISO;
};

#endif // FILEHELPER_H
