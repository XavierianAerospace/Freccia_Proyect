import numpy as np

class RocketPhysicsEngine:
    """
    6-DOF Physics Engine for Rocket Flight Simulation.
    Calculates forces and moments based on current state and control surface angles.
    """
    def __init__(self, mass=5.0, length=1.5, diameter=0.1, inertia=None):
        self.mass = mass
        self.length = length
        self.diameter = diameter
        self.radius = diameter / 2.0
        self.A_ref = np.pi * self.radius**2

        # Inertia matrix (simplified as a cylinder)
        if inertia is None:
            Ixx = 0.5 * mass * self.radius**2
            Iyy = Izz = (1/12) * mass * (3*self.radius**2 + length**2)
            self.I = np.diag([Ixx, Iyy, Izz])
        else:
            self.I = inertia

        self.I_inv = np.linalg.inv(self.I)
        self.g = 9.81 # m/s^2

    def get_atmospheric_density(self, altitude):
        """Standard atmosphere model (simplified)"""
        rho0 = 1.225 # kg/m^3
        L = 0.0065   # K/m
        T0 = 288.15  # K
        return rho0 * (1 - L * altitude / T0)**(5.2561 - 1)

    def calculate_aerodynamics(self, state, servo_angles, dt):
        """
        Calculates Aerodynamic Forces and Moments.
        State: [x, y, z, vx, vy, vz, phi, theta, psi, p, q, r]
        Servo_angles: [s1, s2, s3, s4] (in degrees)
        """
        v_body = state[3:6]
        omega = state[9:12]
        altitude = state[2]

        v_norm = np.linalg.norm(v_body)
        if v_norm < 0.1:
            return np.zeros(3), np.zeros(3)

        rho = self.get_atmospheric_density(altitude)
        q_dynamic = 0.5 * rho * v_norm**2

        # Angle of attack and sideslip (simplified)
        alpha = np.arctan2(v_body[2], v_body[0]) if v_body[0] != 0 else 0
        beta = np.arctan2(v_body[1], v_body[0]) if v_body[0] != 0 else 0

        # Aerodynamic coefficients (Base + Control Surface contribution)
        # In a real scenario, we would use the RF model here.
        # For this architecture, we use a linearized model as a baseline.
        CL_alpha = 2.0 * np.pi
        CD0 = 0.5

        # Lift and Drag
        L = q_dynamic * self.A_ref * CL_alpha * alpha
        D = q_dynamic * self.A_ref * (CD0 + (CL_alpha * alpha)**2 / (np.pi * 0.7))

        # Forces in body frame (Simplified)
        F_aero = np.array([
            -D * np.cos(alpha) + L * np.sin(alpha),
            -q_dynamic * self.A_ref * beta, # Side force
            -D * np.sin(alpha) - L * np.cos(alpha)
        ])

        # Moments from control surfaces (Servos)
        # Each servo affects Roll, Pitch, or Yaw depending on its position
        # s1, s3 affect Pitch; s2, s4 affect Yaw; All affect Roll if moved differentially
        k_servo = 0.1 # Moment coefficient per degree

        M_roll  = q_dynamic * self.A_ref * self.diameter * k_servo * np.sum(servo_angles) / 4.0
        M_pitch = q_dynamic * self.A_ref * self.length * k_servo * (servo_angles[0] - servo_angles[2])
        M_yaw   = q_dynamic * self.A_ref * self.length * k_servo * (servo_angles[1] - servo_angles[3])

        # Damping moments
        M_damping = -0.1 * omega

        M_aero = np.array([M_roll, M_pitch, M_yaw]) + M_damping

        return F_aero, M_aero

    def compute_derivatives(self, state, servo_angles, thrust_force):
        """
        Computes the state derivatives (equations of motion).
        """
        # state = [x, y, z, vx, vy, vz, phi, theta, psi, p, q, r]
        v = state[3:6]
        omega = state[9:12]

        # 1. Kinematics (Position)
        # (Assuming body frame and world frame alignment for simplification in this skeleton)
        d_pos = v

        # 2. Dynamics (Linear Velocity)
        F_aero, M_aero = self.calculate_aerodynamics(state, servo_angles, 0.01)
        F_thrust = np.array([thrust_force, 0, 0])
        F_gravity = np.array([0, 0, -self.mass * self.g])

        # Simplified rotation to body frame for gravity is omitted for brevity in the architecture skeleton
        a = (F_aero + F_thrust + F_gravity) / self.mass

        # 3. Kinematics (Attitude - Euler rates)
        # p, q, r are angular velocities in body frame
        d_attitude = omega # Simplified

        # 4. Dynamics (Angular Velocity)
        # M = I*alpha + omega x (I*omega)
        dw = self.I_inv @ (M_aero - np.cross(omega, self.I @ omega))

        return np.concatenate([d_pos, a, d_attitude, dw])

    def step(self, state, servo_angles, thrust_force, dt):
        """RK4 Integration step"""
        k1 = self.compute_derivatives(state, servo_angles, thrust_force)
        k2 = self.compute_derivatives(state + 0.5 * dt * k1, servo_angles, thrust_force)
        k3 = self.compute_derivatives(state + 0.5 * dt * k2, servo_angles, thrust_force)
        k4 = self.compute_derivatives(state + dt * k3, servo_angles, thrust_force)

        new_state = state + (dt / 6.0) * (k1 + 2*k2 + 2*k3 + k4)

        # Stability: limit angular velocities to prevent NaN overflow
        new_state[9:12] = np.clip(new_state[9:12], -100, 100)

        return new_state
