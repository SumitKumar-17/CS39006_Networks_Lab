import matplotlib.pyplot as plt

# Example Data (Replace with actual observations)
file_sizes = [1.1, 2.3, 6.8, 20.4, 40.8]  # File sizes in KB
packet_counts = [0.001412, 0.001566426, 0.009805439, 0.01224731, 0.07499217]  # Time (s)") observed

# Plotting the data
plt.plot(file_sizes, packet_counts, marker='o', linestyle='-', color='b')  # Added color for clarity
plt.xlabel("File Size (KB)")
plt.ylabel("Time (s)")
plt.title("File Size vs. Time (s)")
plt.grid(True)

# Save the plot to a file
plt.savefig("/home/sumitk/Desktop/Semester-6-IIT-KGP/Semester-6/cs39006_networks_lab/week_4/Sumit_Kumar_22CS30056/time.png")  # Replace with your desired file path

# Optionally, you can close the plot to free memory
plt.close()
