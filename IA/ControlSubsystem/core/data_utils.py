import numpy as np

def normalize_state(raw_state):
    """
    Normalizes raw sensor data into the range expected by the ML model.
    Input raw_state: [Roll, Pitch, Yaw, p, q, r, Alt, Vel]
    """
    norm_state = np.array(raw_state, dtype=np.float32)

    # Scaling factors (should match environment.py)
    norm_state[6] /= 1000.0 # Alt to km
    norm_state[7] /= 300.0  # Vel scaling

    return norm_state

def denormalize_action(ml_action, limit=15.0):
    """
    Converts ML model output [-1, 1] to servo angles in degrees.
    """
    return np.clip(ml_action, -1.0, 1.0) * limit
