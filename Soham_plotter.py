import matplotlib.pyplot as plt
import re

# Lists to store the parsed data
time_vals = []
throughput_vals = []
cwnd_vals = []

# Open and read the log file
with open("/Users/debar/Desktop/CN_LAB_PROJECT/log_stats.txt", "r") as file:
    for line in file:
        # Match the pattern using regular expressions
        match = re.search(r"Time:\s*([\d.]+)\s*s,\s*Throughput:\s*([\d.]+)\s*Mbps,\s*cwnd:\s*(\d+)\s*bytes", line)
        if match:
            time = float(match.group(1))
            throughput = float(match.group(2))
            cwnd = int(match.group(3)) / 1024  # Convert bytes to KB

            time_vals.append(time)
            throughput_vals.append(throughput)
            cwnd_vals.append(cwnd)

# Plot cwnd vs Time
plt.figure(figsize=(12, 5))

plt.subplot(1, 2, 1)
plt.plot(time_vals, cwnd_vals, label="cwnd", color="blue")
plt.xlabel("Time (s)")
plt.ylabel("cwnd (KB)")
plt.title("cwnd vs Time")
plt.grid(True)

# Plot Throughput vs Time
plt.subplot(1, 2, 2)
plt.plot(time_vals, throughput_vals, label="Throughput", color="green")
plt.xlabel("Time (s)")
plt.ylabel("Throughput (Mbps)")
plt.title("Throughput vs Time")
plt.grid(True)

# Show both plots
plt.tight_layout()
plt.show()
