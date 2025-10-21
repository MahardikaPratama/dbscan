# DBSCAN Clustering — C++ Implementation & Experiments

![C++](https://img.shields.io/badge/language-C%2B%2B-00599C?logo=c%2B%2B&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![Status](https://img.shields.io/badge/status-active-brightgreen)
![License](https://img.shields.io/badge/license-See%20repo-lightgrey)

This repository provides a C++ implementation of **DBSCAN-based clustering algorithms** designed for multi-sensor tracking data.  
The project includes **three DBSCAN variants** — *Original DBSCAN*, *Dynamic Epsilon DBSCAN*, and *KDBSCAN* — along with a unified experiment runner and result visualizer.

---

## 📂 Project Structure

```
src/
 ├── lib/
 │   ├── data_handler/         # Input parsing and dataset loader
 │   ├── dbscan/
 │   │   ├── dbscan_regular/         # Original DBSCAN implementation
 │   │   ├── dbscan_dynamic_epsilon/ # Dynamic epsilon (adaptive) DBSCAN
 │   │   ├── kdbscan/                # Core-point optimized DBSCAN
 │   │   ├── object/                 # Object representation for clustering
 │   │   ├── util/                   # Shared utility functions
 │   │   ├── dbscan.cpp / .h         # Common interfaces for all variants
 │   ├── haversine/           # Distance calculation between coordinates
 │   └── metrics/             # Metrics recorder (execution time, RMSE, etc.)
 ├── main.cpp                 # Entry point
 ├── run_all.sh               # Shell script to run all experiments
 ├── CMakeLists.txt           # Build configuration
 └── README.md
```

---

## ⚙️ DBSCAN Variants Implemented

### 1. Original DBSCAN Clustering  
A **standard density-based algorithm** that groups data points based on neighborhood density.  
It requires two manually defined parameters:
- `Eps` — the neighborhood radius.
- `MinPts` — the minimum number of points required to form a dense cluster.  

However, this method has a major limitation: it determines only **one global `Eps` value**, which can be ineffective for datasets with **varying densities** — possibly **missing sparse clusters** or **merging dense ones incorrectly**.  
📁 *Source:* `src/lib/dbscan/dbscan_regular/`

---

### 2. Dynamic Epsilon DBSCAN Clustering  
An **adaptive DBSCAN** variant that automatically determines the optimal `Eps` value.  
It uses the **k-distance graph** and finds the **inflection point (or “knee”)** of the sorted k-distance curve to calculate an adaptive threshold for each local region.  
This makes it more robust for **non-uniform density** datasets.  
📁 *Source:* `src/lib/dbscan/dbscan_dynamic_epsilon/`

---

### 3. KDBSCAN Clustering  
An **efficiency-oriented version** of DBSCAN.  
Instead of processing all points, KDBSCAN:
1. Identifies **core points** first.  
2. Removes **non-core points** to reduce dataset size.  
3. Merges clusters that share overlapping core regions.  

This approach significantly **reduces computation time** while maintaining clustering accuracy.  
📁 *Source:* `src/lib/dbscan/kdbscan/`

---

## 🧪 Running Experiments

### 🔧 Build the project
```bash
cmake ..
cmake --build .
```

### ▶️ Run all experiments
```bash
./run_all.sh
```

This script will:
- Build the executable (if not already built)
- Run all DBSCAN variants on datasets under `/input`
- Save results and metrics under `/output`

---

## 📊 Metrics Collected
Each DBSCAN variant produces performance and clustering metrics such as:
- **Execution Time** per algorithm phase
- **Memory Usage (peak RSS)**
- **RMSE** for spatial error
- **Number of Clusters / Noise Points**
- **Distance Calls** and density statistics

All results can be aggregated and visualized through Python scripts (planned in `scripts/`).

---

## 🛰️ Dataset Description
The dataset simulates **multi-sensor object tracking** (e.g., ships, airplanes).  
Each `.csv` input contains:
- Object ID  
- Latitude & Longitude  
- Sensor ID (optional)  
- Timestamp or sequence index  

Different scenarios are generated to evaluate DBSCAN’s performance on **static, moving, and overlapping tracks**.

---

## 🧠 Future Work
- Add real-time data streaming input  
- Integrate interactive Python visualizer (`matplotlib` / `plotly`)  
- Compare with HDBSCAN for high-density data  

---

**Status:** Active development  
**Maintainer:** Mahardika  
**License:** See `LICENSE` file in repository.
