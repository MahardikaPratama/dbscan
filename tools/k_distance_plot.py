"""
k_distance_plot.py
-----------------------------------
Utility script for DBSCAN parameter tuning (epsilon estimation).

This script loads TrackDataset.csv, computes the distance to the
k-th nearest neighbor for each point (k = MinPts), and plots the
sorted k-distances to help identify the "elbow" point, which corresponds
to a good epsilon value.

This version uses simple Euclidean distance in (lat, lon, alt) space
— consistent with C++ Utils::euclideanDistance().
"""

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.neighbors import NearestNeighbors
import argparse


def generate_k_distance_plot(file_path: str, min_pts: int):
    """
    Compute and plot the k-distance graph for DBSCAN epsilon estimation.
    Uses simple Euclidean distance over (lat, lon, alt).
    """
    # Load dataset
    df = pd.read_csv(file_path)

    # Expect columns: Latitude, Longitude, Altitude
    if not {'Latitude', 'Longitude', 'Altitude'}.issubset(df.columns):
        raise ValueError("CSV must contain 'Latitude', 'Longitude', and 'Altitude' columns.")

    # Convert to numeric and drop invalid values
    for col in ['Latitude', 'Longitude', 'Altitude']:
        df[col] = pd.to_numeric(df[col], errors='coerce')

    before = len(df)
    df = df.dropna(subset=['Latitude', 'Longitude', 'Altitude'])
    after = len(df)
    if before - after > 0:
        print(f"Dropped {before - after} rows with missing values.")

    if len(df) < min_pts:
        raise ValueError(f"Not enough valid points ({len(df)}) for min_pts={min_pts}.")

    # Prepare data: use raw lat/lon/alt (same as C++ Utils::euclideanDistance)
    X = df[['Latitude', 'Longitude', 'Altitude']].to_numpy()

    # Fit nearest neighbors using Euclidean metric
    nbrs = NearestNeighbors(n_neighbors=min_pts, algorithm='auto', metric='euclidean')
    nbrs.fit(X)
    distances, _ = nbrs.kneighbors(X)

    # Take distance to k-th nearest neighbor
    k_distances = np.sort(distances[:, min_pts - 1])

    # Print distance percentiles
    percentiles = [50, 75, 90, 95, 99]
    pct_values = np.percentile(k_distances, percentiles)
    print("k-distance percentiles (Euclidean units):")
    for p, v in zip(percentiles, pct_values):
        print(f"  {p}th: {v:.6f}")

    # Suggest epsilon candidates (in same units as your dataset)
    suggestions = {
        'median': float(np.median(k_distances)),
        '75pct': float(np.percentile(k_distances, 75)),
        '90pct': float(np.percentile(k_distances, 90)),
    }
    print("Suggested epsilon candidates:", suggestions)

    # Plot
    plt.figure(figsize=(8, 5))
    plt.plot(k_distances)
    plt.title(f"k-distance Graph (k = {min_pts})")
    plt.xlabel("Points sorted by distance")
    plt.ylabel(f"Distance to {min_pts}-th nearest neighbor")
    plt.grid(True)
    out_png = 'k_distance.png'
    plt.savefig(out_png, dpi=150)
    print(f"Saved k-distance plot to {out_png}")
    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate k-distance plot for DBSCAN epsilon estimation.")
    parser.add_argument("--file", type=str, required=True, help="Path to input CSV file.")
    parser.add_argument("--minpts", type=int, default=5, help="Number of neighbors (MinPts).")

    args = parser.parse_args()
    generate_k_distance_plot(args.file, args.minpts)
