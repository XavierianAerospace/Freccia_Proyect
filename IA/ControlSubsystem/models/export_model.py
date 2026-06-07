import torch
import sys
import os

# Add training directory to path to import models
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from training.train_control import PolicyNetwork

def export_to_torchscript(checkpoint_path, output_path):
    """
    Exports a trained PyTorch model to TorchScript for C++ deployment.
    """
    state_dim = 8
    action_dim = 4

    model = PolicyNetwork(state_dim, action_dim)

    if os.path.exists(checkpoint_path):
        model.load_state_dict(torch.load(checkpoint_path))
        model.eval()

        # Create a dummy input for tracing
        example_input = torch.randn(1, state_dim)

        # Use tracing to create a TorchScript model
        traced_script_module = torch.jit.trace(model, example_input)
        traced_script_module.save(output_path)

        print(f"Model successfully exported to {output_path}")
    else:
        print(f"Checkpoint {checkpoint_path} not found.")

if __name__ == "__main__":
    export_to_torchscript(
        "IA/ControlSubsystem/models/actor_stable.pth",
        "IA/ControlSubsystem/models/actor_traced.pt"
    )
