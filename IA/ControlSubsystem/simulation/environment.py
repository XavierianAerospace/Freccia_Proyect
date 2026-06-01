import numpy as np
try:
    from .physics_engine import RocketPhysicsEngine
except ImportError:
    from physics_engine import RocketPhysicsEngine

class RocketControlEnv:
    """
    Optimized environment supporting batch observations and vectorized steps.
    """
    def __init__(self, batch_size=1):
        self.batch_size = batch_size
        self.physics = RocketPhysicsEngine()
        self.dt = 0.05
        self.max_steps = 400
        self.reset()

    def reset(self):
        # Initial state (Batch, 12)
        self.state = np.zeros((self.batch_size, 12))
        self.state[:, 2] = 100.0 # Altitude
        self.state[:, 3] = 150.0 # Velocity
        self.state[:, 6:9] = np.random.uniform(-5, 5, (self.batch_size, 3))

        self.steps = 0
        self.dones = np.zeros(self.batch_size, dtype=bool)
        return self._get_obs()

    def _get_obs(self):
        obs = np.zeros((self.batch_size, 8))
        obs[:, 0:3] = self.state[:, 6:9]  # RPY
        obs[:, 3:6] = self.state[:, 9:12] # pqr
        obs[:, 6] = self.state[:, 2] / 1000.0
        obs[:, 7] = self.state[:, 3] / 300.0
        return obs

    def step(self, actions):
        """
        actions: (Batch, 4) in range [-1, 1]
        """
        servo_angles = np.clip(actions, -1, 1) * 15.0

        thrust = 100.0 if self.steps < 200 else 0.0
        self.state = self.physics.step(self.state, servo_angles, thrust, self.dt)

        # Rewards
        attitude_error = np.sum(np.square(self.state[:, 6:9]), axis=1)
        angular_rate_penalty = 0.1 * np.sum(np.square(self.state[:, 9:12]), axis=1)
        control_penalty = 0.01 * np.sum(np.square(actions), axis=1)

        rewards = -(attitude_error + angular_rate_penalty + control_penalty)

        self.steps += 1

        # Check terminations
        step_done = self.steps >= self.max_steps
        crashed = self.state[:, 2] < 0

        new_dones = step_done | crashed
        self.dones = self.dones | new_dones

        # Survival bonus for active agents
        rewards[~self.dones] += 1.0

        return self._get_obs(), rewards, self.dones, {}
