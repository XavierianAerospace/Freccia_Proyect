# 🚀 Proyecto Freccia | Semillero XAE

![XAE Banner](https://img.shields.io/badge/Semillero-Xavierian_Aerospace_Engineering-0033A0?style=for-the-badge)
![Javeriana](https://img.shields.io/badge/Pontificia_Universidad-Javeriana-F2A900?style=for-the-badge)
![Status](https://img.shields.io/badge/Estado-En_Desarrollo-28A745?style=for-the-badge)

## 📖 Descripción General

El **Proyecto Freccia** es una iniciativa de desarrollo e investigación en el marco del **Semillero Xavierian Aerospace Engineering (XAE)** de la Facultad de Ingeniería en la Pontificia Universidad Javeriana. 

Este repositorio centraliza el código fuente, la simulación y la documentación técnica del proyecto. Dado el carácter multidisciplinario del sistema, **la implementación no se encuentra en una única línea de código**, sino que está dividida modularmente a través de diferentes ramas (*branches*), permitiendo el desarrollo en paralelo de múltiples subsistemas.

## 🌿 Arquitectura del Repositorio (Estructura de Ramas)

Este repositorio utiliza un enfoque basado en ramas para separar las fases de implementación y los diferentes dominios del proyecto. 

**La rama `main` actúa únicamente como punto de entrada, índice y documentación global.** Para explorar las implementaciones específicas, debes cambiar a la rama correspondiente según el área de trabajo. Las ramas están organizadas bajo las siguientes categorías de desarrollo:

* 💻 **Software y Algoritmos:** Desarrollo de interfaces de usuario web, control de rutas, dashboards de telemetría y algoritmos de machine learning.
* ⚙️ **Simulación y Análisis:** Scripts matemáticos, simulaciones cinemáticas, análisis de trayectorias y dinámica de sistemas.
* 🛠️ **Diseño y Estructuras:** Archivos de diseño CAD, análisis de esfuerzos (FEA) y configuraciones mecánicas.
* 📄 **Documentación:** Formulación de proyectos, reportes de investigación, manuales y derivaciones matemáticas compiladas en LaTeX.

### 💡 ¿Cómo navegar entre las implementaciones?

**Desde la interfaz de GitHub:**
Haz clic en el botón desplegable que dice `Branch: main ▾` (en la parte superior izquierda de la lista de archivos) y selecciona la rama del subsistema que deseas explorar.

**Desde la terminal (Git):**
Para listar todas las ramas disponibles y cambiar a una de ellas, utiliza los siguientes comandos:
` ` `bash
git fetch --all
git branch -a               # Lista todas las ramas
git checkout <nombre-rama>  # Cambia a la rama de la implementación deseada
` ` `

## ⚙️ Requisitos y Entorno de Trabajo

Dado que cada rama contiene implementaciones en diferentes lenguajes y herramientas, los requisitos técnicos varían significativamente. Dependiendo de la rama en la que te encuentres, el stack tecnológico puede requerir:

* **MATLAB / Simulink:** Para la ejecución de simulaciones y modelado de mecanismos.
* **Python 3.x:** (Librerías como Flask, NumPy, etc.) para levantar las interfaces web o ejecutar modelos algorítmicos.
* **LaTeX:** Distribución local (TeX Live/MiKTeX) o plataforma online (Overleaf) para la lectura y compilación de la documentación técnica.

> ⚠️ **Importante:** Te invitamos a revisar el archivo `README.md` secundario que se encuentra *dentro* de cada rama específica, ya que allí se detallan las instrucciones exactas de instalación, dependencias y ejecución de ese módulo en particular.

## 🤝 Flujo de Trabajo para el Equipo

Para los investigadores y miembros del equipo trabajando en el **Laboratorio Ingenia**, se deben seguir estas normativas de control de versiones:

1.  **No realizar *commits* directos a `main`.**
2.  Ubícate siempre en la rama del subsistema en el que estás trabajando. Si vas a crear una nueva funcionalidad, hazlo creando una sub-rama (ej. `feature/nombre-de-la-mejora`).
3.  Mantén el código comentado y los archivos de configuración actualizados.
4.  Utiliza *Pull Requests* (PR) para consolidar cambios importantes o integrar módulos.

## 🏛️ Reconocimientos

* **Organización:** Semillero Xavierian Aerospace Engineering (XAE)
* **Institución:** Facultad de Ingeniería, Pontificia Universidad Javeriana (Bogotá).
* **Equipo Base:** Desarrollado por los estudiantes e investigadores del equipo XAE.

---
*Repositorio mantenido por el semillero Xavierian Aerospace Engineering.*
