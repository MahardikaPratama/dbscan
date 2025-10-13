"""
plot_clusters_2d.py
-------------------
Read ClusteredOutput.csv and plot a 2D scatter of Longitude vs Latitude
colored by cluster id. Saves `clusters_2d.png` by default.

Usage:
    python3 tools/plot_clusters_3d.py --file data/ClusteredOutput.csv
"""

import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt


def plot_clusters_2d(file_path: str, out_png: str = "clusters_2d.png"):
    df = pd.read_csv(file_path, header=None)

    # file format: object_id, lat, lon, alt, clusterId
    if df.shape[1] < 4:
        raise ValueError("Clustered CSV must have at least 4 columns: object_id, lat, lon, clusterId")

    # Keep first 5 columns if present, otherwise adapt
    df = df.iloc[:, 0:5]
    # If alt missing, insert NaN
    if df.shape[1] == 4:
        df.columns = ["object_id", "lat", "lon", "clusterId"]
        df["alt"] = np.nan
    else:
        df.columns = ["object_id", "lat", "lon", "alt", "clusterId"]

    # Ensure numeric
    for col in ["lat", "lon", "clusterId"]:
        df[col] = pd.to_numeric(df[col], errors="coerce")

    before = len(df)
    df = df.dropna(subset=["lat", "lon", "clusterId"])
    after = len(df)
    dropped = before - after
    if dropped > 0:
        print(f"Dropped {dropped} rows with missing numeric values.")

    # Normalize colors for cluster ids
    clusters = df["clusterId"].astype(int)
    unique_clusters = sorted(clusters.unique())

    cmap = plt.get_cmap("tab20")
    colors = {cid: cmap(i % 20) for i, cid in enumerate(unique_clusters)}

    plt.figure(figsize=(10, 7))

    for cid in unique_clusters:
        subset = df[df["clusterId"] == cid]
        plt.scatter(subset["lon"], subset["lat"], c=[colors[cid]], label=str(cid), s=10)

    plt.xlabel("Longitude")
    plt.ylabel("Latitude")
    plt.title("2D Cluster Plot")
    plt.legend(title="clusterId", bbox_to_anchor=(1.05, 1), loc='upper left')
    plt.tight_layout()
    plt.savefig(out_png, dpi=200)
    print(f"Saved 2D cluster plot to {out_png}")
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot 2D clusters from clustered CSV.")
    parser.add_argument("--file", type=str, default="data/ClusteredOutput.csv", help="Path to clustered CSV")
    parser.add_argument("--out", type=str, default="clusters_2d.png", help="Output PNG file")
    args = parser.parse_args()
    plot_clusters_2d(args.file, args.out)
