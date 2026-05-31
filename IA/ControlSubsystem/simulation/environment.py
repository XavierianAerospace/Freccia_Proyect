import numpy as np
try:
    from .physics_engine import RocketPhysicsEngine
except ImportError:
    from physics_engine import RocketPhysicsEngine

class RocketControlEnv:
    """
    Gym-like environment for Reinforcement Learning training.
    """
    def __init__(self):
        self.physics = RocketPhysicsEngine()
        self.dt = 0.05 # 20Hz control loop
        self.max_steps = 400 # 20 seconds of flight

        # Action space: 4 servos, range [-15, 15] degrees
        self.action_low = -15.0
        self.action_high = 15.0

        # State space: [roll, pitch, yaw, p, q, r, alt, vel]
        # Normalized for better training performance
        self.reset()

    def reset(self):
        # Initial state [x, y, z, vx, vy, vz, phi, theta, psi, p, q, r]
        # Start with some random attitude perturbation
        self.state = np.zeros(12)
        self.state[2] = 100.0 # Start at 100m altitude
        self.state[3] = 150.0 # 150 m/s vertical velocity
        self.state[6:9] = np.random.uniform(-5, 5, 3) # Small initial tilt

        self.steps = 0
        return self._get_obs()

    def _get_obs(self):
        # Return normalized observation for RL
        obs = np.array([
            self.state[6], # Roll
            self.state[7], # Pitch
            self.state[8], # Yaw
            self.state[9], # p
            self.state[10],# q
            self.state[11],# r
            self.state[2] / 1000.0, # Altitude (km)
            self.state[3] / 300.0   # Velocity (scaled)
        ])
        return obs

    def step(self, action):
        """
        Apply servo angles and propagate simulation.
        action: [s1, s2, s3, s4] in range [-1, 1]
        """
        # 1. Denormalize actions to degrees
        servo_angles = np.clip(action, -1, 1) * 15.0

        # 2. Physics step
        thrust = 100.0 if self.steps < 200 else 0 # 10s of thrust
        self.state = self.physics.step(self.state, servo_angles, thrust, self.dt)

        # 3. Calculate reward
        # Goal: Keep Roll, Pitch, Yaw at 0.
        attitude_error = np.sum(np.square(self.state[6:9]))
        angular_rate_penalty = 0.1 * np.sum(np.square(self.state[9:12]))
        control_effort = 0.01 * np.sum(np.square(action))

        reward = -(attitude_error + angular_rate_penalty + control_effort)

        # 4. Check if done
        self.steps += 1
        done = (self.steps >= self.max_steps) or (self.state[2] < 0)

        # 5. Survival bonus
        if not done:
            reward += 1.0

        return self._get_obs(), reward, done, {}

    def render(self):
        # Optional: Print state for debugging
        print(f"Step: {self.steps} | Alt: {self.state[2]:.1f} | Att: {self.state[6:9]}")
