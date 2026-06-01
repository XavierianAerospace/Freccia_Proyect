import torch
import torch.nn as nn
import torch.optim as optim
from torch.distributions import Normal
import numpy as np
import sys
import os

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
    batch_size = 16 # Vectorized training
    env = RocketControlEnv(batch_size=batch_size)
    state_dim = 8
    action_dim = 4

    actor = PolicyNetwork(state_dim, action_dim)
    critic = ValueNetwork(state_dim)

    actor_opt = optim.Adam(actor.parameters(), lr=3e-4)
    critic_opt = optim.Adam(critic.parameters(), lr=1e-3)

    num_iterations = 20
    gamma = 0.99

    print(f"Starting Vectorized Training: {batch_size} agents in parallel.")

    for itr in range(num_iterations):
        states = env.reset() # (Batch, State)

        all_log_probs = []
        all_values = []
        all_rewards = []
        all_masks = []

        for t in range(400):
            states_pt = torch.FloatTensor(states)
            mu, sigma = actor(states_pt)
            dist = Normal(mu, sigma)
            actions = dist.sample()

            next_states, rewards, dones, _ = env.step(actions.numpy())

            log_prob = dist.log_prob(actions).sum(dim=1)
            value = critic(states_pt)

            all_log_probs.append(log_prob)
            all_values.append(value.squeeze())
            all_rewards.append(torch.FloatTensor(rewards))
            all_masks.append(torch.FloatTensor(1 - dones))

            states = next_states
            if dones.all(): break

        # Compute Returns and Advantages
        returns = []
        R = torch.zeros(batch_size)
        for r, m in zip(reversed(all_rewards), reversed(all_masks)):
            R = r + gamma * R * m
            returns.insert(0, R.clone())

        returns = torch.stack(returns).detach() # (T, Batch)
        log_probs = torch.stack(all_log_probs)   # (T, Batch)
        values = torch.stack(all_values)       # (T, Batch)

        advantages = returns - values

        # Optimize
        actor_loss = -(log_probs * advantages.detach()).mean()
        critic_loss = advantages.pow(2).mean()

        actor_opt.zero_grad()
        actor_loss.backward()
        actor_opt.step()

        critic_opt.zero_grad()
        critic_loss.backward()
        critic_opt.step()

        avg_reward = torch.stack(all_rewards).sum(dim=0).mean().item()
        print(f"Iteration {itr+1} | Mean Reward: {avg_reward:.2f} | Loss: {actor_loss.item():.4f}")

    torch.save(actor.state_dict(), "IA/ControlSubsystem/models/actor_stable.pth")
    print("Training complete.")

if __name__ == "__main__":
    train()
