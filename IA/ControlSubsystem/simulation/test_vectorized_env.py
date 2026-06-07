import numpy as np
import sys
import os

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from simulation.environment import RocketControlEnv

def test_vectorized_env():
    batch_size = 4
    env = RocketControlEnv(batch_size=batch_size)
    obs = env.reset()
    print(f"Initial batch obs shape: {obs.shape}")

    for i in range(5):
        actions = np.random.uniform(-1, 1, (batch_size, 4))
        obs, rewards, dones, _ = env.step(actions)
        print(f"Step {i}: Mean Reward = {np.mean(rewards):.2f}, All Dones = {dones.all()}")

    print("Vectorized environment verification complete.")

if __name__ == "__main__":
    test_vectorized_env()
