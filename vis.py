import pandas as pd
import matplotlib.pyplot as plt

memory_df = pd.read_csv("memory_log.csv")
cache_df = pd.read_csv("cache_log.csv")

def plot_memory_layout(memory_df):
    snapshots = memory_df[memory_df['Address'] == 'END'].index
    fig, ax = plt.subplots(len(snapshots), 1, figsize=(10, 8), sharex=True)
    for i, snapshot in enumerate(snapshots):
        ax[i].scatter(memory_df['Address'][:snapshot], memory_df['Size'][:snapshot], c=memory_df['Is_Free'][:snapshot], cmap='coolwarm')
        ax[i].set_title(f"Memory Snapshot {i + 1}")
        ax[i].set_ylabel("Size")
    plt.xlabel("Memory Address")
    plt.show()

def plot_cache_stats(cache_df):
    plt.figure(figsize=(10, 5))
    plt.plot(cache_df.index, cache_df["Cache_Hits"], label="Cache Hits", marker="o")
    plt.plot(cache_df.index, cache_df["Cache_Misses"], label="Cache Misses", marker="x")
    plt.plot(cache_df.index, cache_df["Cache_Evictions"], label="Cache Evictions", marker="s")
    plt.xlabel("Operation")
    plt.ylabel("Count")
    plt.title("Cache Performance Over Time")
    plt.legend()
    plt.show()

plot_memory_layout(memory_df)
plot_cache_stats(cache_df)
