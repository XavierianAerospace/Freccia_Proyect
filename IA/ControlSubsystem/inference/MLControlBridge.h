#ifndef ML_CONTROL_BRIDGE_H
#define ML_CONTROL_BRIDGE_H

#include "../../../GUI/Header_Files/data/SensorData.h"
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

/**
 * @brief Real-Time Bridge for ML Control Inference.
 * This class translates GUI sensor data into AI control actions.
 */
class MLControlBridge {
public:
    struct ControlActions {
        float s1, s2, s3, s4;
    };

    MLControlBridge() {
        // In a full implementation, this would load the TorchScript model
        std::cout << "[ML Bridge] Initializing Neural Network Inference..." << std::endl;
    }

    /**
     * @brief Predicts optimal servo angles based on current telemetry.
     */
    ControlActions predict(const SensorData& data) {
        // 1. Prepare normalized state vector
        // State: [Roll, Pitch, Yaw, p, q, r, Alt, Vel]
        std::vector<float> state = {
            data.Roll,
            data.Pitch,
            data.Yaw,
            0.0f, // Angular rates would need differentiation or extra sensors
            0.0f,
            0.0f,
            data.AltDiff / 1000.0f,
            0.0f  // Velocity would need differentiation
        };

        // 2. Perform Inference
        // Since we cannot run full libtorch in this environment easily,
        // we provide a placeholder for the logic that will call the model.
        ControlActions actions = runInference(state);

        return actions;
    }

private:
    ControlActions runInference(const std::vector<float>& state) {
        // Placeholder for Neural Network forward pass
        // Logic: Apply weights and biases from actor_stable.pth

        // Safety check: if attitude is stable, keep servos at 0
        if (std::abs(state[0]) < 1.0f && std::abs(state[1]) < 1.0f && std::abs(state[2]) < 1.0f) {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        // Simple proportional response for demonstration (ML would be non-linear)
        float k = -0.5f;
        ControlActions actions;
        actions.s1 = std::max(-15.0f, std::min(15.0f, k * state[1])); // Pitch correction
        actions.s3 = -actions.s1;
        actions.s2 = std::max(-15.0f, std::min(15.0f, k * state[2])); // Yaw correction
        actions.s4 = -actions.s2;

        return actions;
    }
};

#endif
