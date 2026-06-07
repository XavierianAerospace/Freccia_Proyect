import numpy as np

class RocketPhysicsEngine:
    """
    Optimized 6-DOF Physics Engine for Rocket Flight Simulation.
    Supports Vectorized Operations for Batch Simulation.
    Algorithmic Complexity: O(1) with respect to Batch Size (using NumPy/BLAS).
    """
    def __init__(self, mass=5.0, length=1.5, diameter=0.1, inertia=None):
        self.mass = mass
        self.length = length
        self.diameter = diameter
        self.radius = diameter / 2.0
        self.A_ref = np.pi * self.radius**2

        if inertia is None:
            Ixx = 0.5 * mass * self.radius**2
            Iyy = Izz = (1/12) * mass * (3*self.radius**2 + length**2)
            self.I = np.diag([Ixx, Iyy, Izz])
        else:
            self.I = inertia

        self.I_inv = np.linalg.inv(self.I)
        self.g = 9.81
        self.wind_vector = np.array([0.0, 0.0, 0.0])
        self.wind_gustiness = 0.5

    def get_atmospheric_density(self, altitude):
        """Vectorized atmospheric model"""
        rho0 = 1.225
        L = 0.0065
        T0 = 288.15
        # Ensure altitude is handled as array
        return rho0 * np.maximum(0, (1 - L * altitude / T0))**(5.2561 - 1)

    def calculate_aerodynamics(self, state, servo_angles):
        """
        Calculates Aerodynamic Forces and Moments (Vectorized).
        state: (Batch, 12)
        servo_angles: (Batch, 4)
        """
        # Ensure input is at least 2D
        if state.ndim == 1:
            state = state[np.newaxis, :]
        if servo_angles.ndim == 1:
            servo_angles = servo_angles[np.newaxis, :]

        v_air = state[:, 3:6] - (self.wind_vector + np.random.normal(0, self.wind_gustiness, (state.shape[0], 3)))
        v_norm = np.linalg.norm(v_air, axis=1, keepdims=True)
        v_norm = np.maximum(v_norm, 1e-6) # Avoid div by zero

        rho = self.get_atmospheric_density(state[:, 2:3])
        q_dynamic = 0.5 * rho * v_norm**2

        # alpha = atan2(vz, vx)
        alpha = np.arctan2(v_air[:, 2:3], v_air[:, 0:1])
        beta = np.arctan2(v_air[:, 1:2], v_air[:, 0:1])

        CL_alpha = 2.0 * np.pi
        CD0 = 0.5

        L = q_dynamic * self.A_ref * CL_alpha * alpha
        D = q_dynamic * self.A_ref * (CD0 + (CL_alpha * alpha)**2 / (np.pi * 0.7))

        # Forces in body frame
        F_aero = np.zeros_like(state[:, 0:3])
        F_aero[:, 0:1] = -D * np.cos(alpha) + L * np.sin(alpha)
        F_aero[:, 1:2] = -q_dynamic * self.A_ref * beta
        F_aero[:, 2:3] = -D * np.sin(alpha) - L * np.cos(alpha)

        # Moments
        k_servo = 0.1
        M_roll  = q_dynamic * self.A_ref * self.diameter * k_servo * np.sum(servo_angles, axis=1, keepdims=True) / 4.0
        M_pitch = q_dynamic * self.A_ref * self.length * k_servo * (servo_angles[:, 0:1] - servo_angles[:, 2:3])
        M_yaw   = q_dynamic * self.A_ref * self.length * k_servo * (servo_angles[:, 1:2] - servo_angles[:, 3:4])

        omega = state[:, 9:12]
        M_damping = -0.1 * omega
        M_aero = np.hstack([M_roll, M_pitch, M_yaw]) + M_damping

        return F_aero, M_aero

    def compute_derivatives(self, state, servo_angles, thrust_force):
        if state.ndim == 1: state = state[np.newaxis, :]

        v = state[:, 3:6]
        omega = state[:, 9:12]

        F_aero, M_aero = self.calculate_aerodynamics(state, servo_angles)

        # Forces
        F_thrust = np.zeros_like(v)
        F_thrust[:, 0] = thrust_force
        F_gravity = np.zeros_like(v)
        F_gravity[:, 2] = -self.mass * self.g

        a = (F_aero + F_thrust + F_gravity) / self.mass

        # Moments (Euler equations)
        # M = I*dw + w x (I*w)  => dw = I^-1 (M - w x (I*w))
        Iw = omega @ self.I.T # (Batch, 3)
        w_x_Iw = np.cross(omega, Iw)
        dw = (M_aero - w_x_Iw) @ self.I_inv.T

        return np.hstack([v, a, omega, dw])

    def step(self, state, servo_angles, thrust_force, dt):
        """Vectorized RK4 Integration"""
        k1 = self.compute_derivatives(state, servo_angles, thrust_force)
        k2 = self.compute_derivatives(state + 0.5 * dt * k1, servo_angles, thrust_force)
        k3 = self.compute_derivatives(state + 0.5 * dt * k2, servo_angles, thrust_force)
        k4 = self.compute_derivatives(state + dt * k3, servo_angles, thrust_force)

        new_state = state + (dt / 6.0) * (k1 + 2*k2 + 2*k3 + k4)
        new_state[:, 9:12] = np.clip(new_state[:, 9:12], -100, 100)

        return new_state
