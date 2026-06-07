# Subsistema de Control por Aprendizaje Automático (ML Control Subsystem)

Este módulo proporciona una arquitectura completa para el control inteligente de superficies aerodinámicas (aletas) de un cohete utilizando Aprendizaje por Refuerzo (Reinforcement Learning) y simulaciones de física avanzada.

## Arquitectura del Sistema

La arquitectura se divide en cuatro capas principales diseñadas para el aprendizaje continuo y la ejecución en tiempo real:

1.  **Capa de Simulación (`simulation/`)**:
    *   Motor de física 6-DOF (6 grados de libertad).
    *   Modelado de fuerzas: Empuje, Gravedad, y Aerodinámica (basada en coeficientes CL/CD predichos por modelos Random Forest existentes).
    *   Entorno de entrenamiento compatible con Gymnasium.

2.  **Capa de Aprendizaje (`training/` & `models/`)**:
    *   Algoritmo: **REINFORCE (con Baseline)** para control continuo (escalable a PPO).
    *   **Estado (State)**: [Roll, Pitch, Yaw, dRoll, dPitch, dYaw, Velocidad, Altitud].
    *   **Acciones (Actions)**: Ángulos de los 4 servos de las superficies de control.
    *   **Función de Recompensa (Reward)**: Penalización por desviación de la trayectoria deseada y uso excesivo de energía en los servos; bonificación por estabilidad (bajas velocidades angulares).

3.  **Capa de Inferencia en Tiempo Real (`inference/`)**:
    *   Puente C++ (`MLControlBridge.h`) para integrar con la GUI.
    *   Conversión de `SensorData` al espacio de estados del modelo.
    *   Ejecución de baja latencia para corrección de superficies en cada ciclo de telemetría.

4.  **Bucle de Mejora Continua (`data/`)**:
    *   Procesamiento de logs de vuelo reales (`.csv`).
    *   Ajuste de parámetros de la simulación (System Identification) basado en datos reales.
    *   Re-entrenamiento del modelo para mejorar el control en cada lanzamiento sucesivo.

## Flujo de Datos

1.  El cohete transmite datos de sensores (`SensorData`).
2.  La GUI recibe los datos y los pasa al `MLControlBridge`.
3.  El modelo de ML predice los ángulos óptimos para los servos para estabilizar el vuelo.
4.  La GUI visualiza las acciones y (opcionalmente) las envía de vuelta al hardware.
5.  Al final del vuelo, los datos se guardan y se usan para mejorar la simulación y el modelo.

## Física del Modelo

Se utiliza el formalismo de Newton-Euler para representar la dinámica del cuerpo rígido:
*   **Traslación**: $\sum \vec{F} = m \dot{\vec{v}}$
*   **Rotación**: $\sum \vec{M} = I \dot{\vec{\omega}} + \vec{\omega} \times (I \vec{\omega})$

Donde las fuerzas aerodinámicas dependen dinámicamente del Ángulo de Ataque ($\alpha$) y el Número de Reynolds, utilizando los modelos `IA/rf_model.h` ya entrenados.

## Análisis de Complejidad Algorítmica (Big O)

El sistema ha sido optimizado para garantizar el rendimiento en tiempo real y la escalabilidad del entrenamiento:

| Módulo | Algoritmo | Complejidad Temporal | Complejidad Espacial | Notas |
| :--- | :--- | :--- | :--- | :--- |
| **Inferencia** | Red Neuronal MLP | $O(L \cdot N^2)$ | $O(N_{params})$ | Ejecución en tiempo real con latencia < 1ms. |
| **Simulación** | RK4 Vectorizado | $O(B)$ | $O(B)$ | $B$ es el tamaño del lote (Batch). Optimizado con NumPy. |
| **Entrenamiento** | PPO/REINFORCE | $O(I \cdot B \cdot T)$ | $O(B \cdot T)$ | $I$: Iteraciones, $T$: Pasos de tiempo. |
| **Bridge C++** | Pre-procesamiento | $O(1)$ | $O(1)$ | Sin asignaciones dinámicas de memoria (Heap) en el bucle principal. |

### Optimizaciones Implementadas:
1. **Vectorización**: El motor de física procesa múltiples estados simultáneamente mediante operaciones matriciales (NumPy/BLAS), permitiendo que el entrenamiento sea $O(1)$ respecto al tiempo de reloj por cada agente en el lote.
2. **Memoria Estática**: El puente C++ utiliza arreglos en el stack para evitar la fragmentación de memoria y el overhead del recolector de basura o asignadores dinámicos.
