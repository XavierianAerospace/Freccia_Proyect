import torch
import torch.nn as nn
import torch.optim as optim
from torch.distributions import Normal
import numpy as np
import sys
import os

# Add parent directory to path to import simulation module
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from simulation.environment import RocketControlEnv

class PolicyNetwork(nn.Module):
    def __init__(self, state_dim, action_dim):
        super(PolicyNetwork, self).__init__()
        self.fc = nn.Sequential(
            nn.Linear(state_dim, 64),
            nn.ReLU(),
            nn.Linear(64, 64),
            nn.ReLU()
        )
        self.mu = nn.Linear(64, action_dim)
        self.sigma = nn.Parameter(torch.zeros(action_dim))

    def forward(self, state):
        x = self.fc(state)
        mu = torch.tanh(self.mu(x))
        sigma = torch.exp(self.sigma)
        return mu, sigma

class ValueNetwork(nn.Module):
    def __init__(self, state_dim):
        super(ValueNetwork, self).__init__()
        self.fc = nn.Sequential(
            nn.Linear(state_dim, 64),
            nn.ReLU(),
            nn.Linear(64, 64),
            nn.ReLU(),
            nn.Linear(64, 1)
        )

    def forward(self, state):
        return self.fc(state)

def train():
    env = RocketControlEnv()
    state_dim = 8
    action_dim = 4

    actor = PolicyNetwork(state_dim, action_dim)
    critic = ValueNetwork(state_dim)

    actor_opt = optim.Adam(actor.parameters(), lr=3e-4)
    critic_opt = optim.Adam(critic.parameters(), lr=1e-3)

    num_episodes = 50
    gamma = 0.99

    print(f"Starting training for {num_episodes} episodes...")

    for ep in range(num_episodes):
        state = env.reset()
        log_probs = []
        values = []
        rewards = []
        masks = []

        # Rollout
        for t in range(400):
            state_t = torch.FloatTensor(state)
            mu, sigma = actor(state_t)
            dist = Normal(mu, sigma)
            action = dist.sample()

            next_state, reward, done, _ = env.step(action.numpy())

            log_prob = dist.log_prob(action).sum()
            value = critic(state_t)

            log_probs.append(log_prob)
            values.append(value)
            rewards.append(torch.FloatTensor([reward]))
            masks.append(torch.FloatTensor([1 - done]))

            state = next_state
            if done:
                break

        # Compute Returns and Advantages
        returns = []
        R = 0
        for r, m in zip(reversed(rewards), reversed(masks)):
            R = r + gamma * R * m
            returns.insert(0, R)

        returns = torch.stack(returns).detach()
        log_probs = torch.stack(log_probs)
        values = torch.stack(values)

        advantages = returns - values

        # Actor Loss (REINFORCE with Baseline)
        actor_loss = -(log_probs * advantages.detach()).mean()

        # Critic Loss
        critic_loss = advantages.pow(2).mean()

        # Optimization
        actor_opt.zero_grad()
        actor_loss.backward()
        actor_opt.step()

        critic_opt.zero_grad()
        critic_loss.backward()
        critic_opt.step()

        if (ep + 1) % 10 == 0:
            total_reward = sum(rewards).item()
            print(f"Episode {ep+1} | Total Reward: {total_reward:.2f} | Loss: {actor_loss.item():.4f}")

    # Save models
    torch.save(actor.state_dict(), "IA/ControlSubsystem/models/actor_stable.pth")
    print("Training complete. Model saved.")

if __name__ == "__main__":
    train()
