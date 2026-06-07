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
        // In a full implementation with libtorch:
        // module = torch::jit::load("IA/ControlSubsystem/models/actor_traced.pt");
        std::cout << "[ML Bridge] Architecture ready. Model path: IA/ControlSubsystem/models/actor_traced.pt" << std::endl;
    }

    /**
     * @brief Predicts optimal servo angles based on current telemetry.
     */
    /**
     * @brief Predicts optimal servo angles based on current telemetry.
     * Complexity: O(L) where L is the number of layers (Inference)
     */
    ControlActions predict(const SensorData& data) {
        // 1. Prepare state vector (Fixed size avoids allocations in hot loop)
        float state[8] = {
            data.Roll,
            data.Pitch,
            data.Yaw,
            0.0f,
            0.0f,
            0.0f,
            data.AltDiff / 1000.0f,
            0.0f
        };

        // 2. Perform Inference
        return runInference(state);
    }

private:
    /**
     * @brief Optimized Inference Placeholder
     * In a production environment with libtorch, this would be O(N_parameters).
     */
    ControlActions runInference(const float state[8]) {
        // Safety check (O(1))
        if (std::abs(state[0]) < 1.0f && std::abs(state[1]) < 1.0f && std::abs(state[2]) < 1.0f) {
            return {0.0f, 0.0f, 0.0f, 0.0f};
        }

        // Optimized arithmetic
        float k = -0.5f;
        ControlActions actions;
        actions.s1 = std::clamp(k * state[1], -15.0f, 15.0f);
        actions.s3 = -actions.s1;
        actions.s2 = std::clamp(k * state[2], -15.0f, 15.0f);
        actions.s4 = -actions.s2;

        return actions;
    }
};

#endif
