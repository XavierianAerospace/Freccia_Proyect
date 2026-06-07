import numpy as np
import sys
import os

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from environment import RocketControlEnv

def test_env():
    env = RocketControlEnv()
    obs = env.reset()
    print(f"Initial obs: {obs}")

    for i in range(10):
        action = np.random.uniform(-1, 1, 4)
        obs, reward, done, _ = env.step(action)
        print(f"Step {i}: obs={obs}, reward={reward}")
        if np.isnan(obs).any():
            print("NaN detected in observations!")
            break

if __name__ == "__main__":
    test_env()
