import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys

def plot_flight_analysis(log_path):
    """
    Plots flight dynamics and control performance.
    Works with both simulation logs and real flight telemetry.
    """
    try:
        data = pd.read_csv(log_path)
    except Exception as e:
        print(f"Error loading {log_path}: {e}")
        return

    fig, axs = plt.subplots(3, 1, figsize=(10, 12), sharex=True)

    # 1. Attitude Plot
    axs[0].plot(data['Roll'], label='Roll', color='r')
    axs[0].plot(data['Pitch'], label='Pitch', color='g')
    axs[0].plot(data['Yaw'], label='Yaw', color='b')
    axs[0].set_ylabel('Degrees')
    axs[0].set_title('Rocket Attitude (Degrees)')
    axs[0].legend()
    axs[0].grid(True)

    # 2. Control Action Plot
    if 'Servo1' in data.columns:
        axs[1].plot(data['Servo1'], label='Servo 1', alpha=0.7)
        axs[1].plot(data['Servo2'], label='Servo 2', alpha=0.7)
        axs[1].plot(data['Servo3'], label='Servo 3', alpha=0.7)
        axs[1].plot(data['Servo4'], label='Servo 4', alpha=0.7)
    axs[1].set_ylabel('Deflection (Deg)')
    axs[1].set_title('Control Surface Actions')
    axs[1].legend()
    axs[1].grid(True)

    # 3. Altitude/Velocity (Optional)
    if 'AltDiff' in data.columns:
        axs[2].plot(data['AltDiff'], label='Altitude', color='black')
        axs[2].set_ylabel('Meters')
        axs[2].set_title('Flight Profile')
        axs[2].grid(True)

    plt.xlabel('Time Step')
    plt.tight_layout()
    plt.show()
    print(f"Analysis plot generated for {log_path}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        plot_flight_analysis(sys.argv[1])
    else:
        print("Usage: python visualize_flight.py <path_to_log.csv>")
