import re
import matplotlib.pyplot as plt

# Lists to store the parsed data
time_vals = []
cwnd_vals = []

# Open and read the log file
with open("/Users/debar/Desktop/CN_LAB_PROJECT/cwnd.txt", "r") as file:
    ind = 0
    for line in file:
        match = re.search(r"CWnd:\s*(\d+)\s*KB", line)
        if match:
            ind += 1
            if ind % 10 == 0:  
                cwnd_vals.append(int(match.group(1)))
                time_vals.append(ind * 0.05) 


plt.figure(figsize=(10, 5))
plt.plot(time_vals, cwnd_vals, label="Congestion Window", color="blue", linewidth=2)
plt.xlabel("Time (s)", fontsize=12)
plt.ylabel("cwnd (KB)", fontsize=12)
plt.title("Congestion Window vs Time", fontsize=14)
plt.grid(True)
plt.legend()
plt.tight_layout()
plt.show()
