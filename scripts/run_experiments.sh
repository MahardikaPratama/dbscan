#!/usr/bin/env bash
set -euo pipefail

# Usage:
# ./scripts/run_experiments.sh /path/to/input.csv /path/to/outroot [--binary ./build/kmeans] [--iters 100] [--restarts 5] [--perf-events "cycles,instructions,cache-misses"]

ARGS=()
for a in "$@"; do
  ARGS+=("$a")
done
if [ ${#ARGS[@]} -lt 2 ]; then
  echo "Usage: $0 <input.csv|input_dir> <out_root> [--binary <path>] [--iters N] [--restarts N] [--perf-events \"events\"]"
  exit 1
fi

INPUT=${ARGS[0]}
OUTROOT=${ARGS[1]}
BINARY=./build/kmeans
ITERS=100
RESTARTS=5
PERF_EVENTS="cycles,instructions,cache-misses"

shift 2 || true
while (("$#")); do
  case "$1" in
    --binary) BINARY="$2"; shift 2;;
    --iters) ITERS="$2"; shift 2;;
    --restarts) RESTARTS="$2"; shift 2;;
    --perf-events) PERF_EVENTS="$2"; shift 2;;
    *) echo "Unknown arg: $1"; exit 1;;
  esac
done

mkdir -p "$OUTROOT"

# Build the project before running to ensure binary is up-to-date.
# If the build directory isn't configured, run cmake to configure it.
if [ ! -d build ] || [ ! -f build/CMakeCache.txt ]; then
  echo "Configuring build directory..."
  cmake -S . -B build
fi

echo "Building project..."
cmake --build build -j "$(nproc)"

# Clear existing files/folders inside OUTROOT (preserve the directory itself)
if [ -d "$OUTROOT" ]; then
  # safety: avoid accidental massive deletes
  if [ "$OUTROOT" = "/" ] || [ -z "$OUTROOT" ]; then
    echo "Refusing to clear unsafe OUTROOT: $OUTROOT"
    exit 1
  fi
  if [ "$(ls -A "$OUTROOT")" ]; then
    echo "Clearing existing files in $OUTROOT (recursive)"
    # remove EVERYTHING under OUTROOT (preserve OUTROOT dir itself)
    # handle dotfiles and nested directories
    find "$OUTROOT" -mindepth 1 -exec rm -rf {} +
  fi
fi

shopt -s nullglob
FILES=()
if [ -d "$INPUT" ]; then
  # deep recursive search for CSV files, handle spaces
  while IFS= read -r -d $'\0' f; do
    FILES+=("$f")
  done < <(find "$INPUT" -type f \( -iname '*.csv' \) -print0)
else
  FILES+=("$INPUT")
fi

if [ ${#FILES[@]} -eq 0 ]; then
  echo "No CSV files found in input: $INPUT"
  exit 1
fi

echo "Found ${#FILES[@]} CSV(s). Binary=$BINARY iters=$ITERS restarts=$RESTARTS"

for csv in "${FILES[@]}"; do
  if [ -d "$INPUT" ]; then
    # preserve relative path under OUTROOT (remove INPUT prefix)
    rel=${csv#"$INPUT"/}
    name_no_ext=${rel%.csv}
    runroot="$OUTROOT/$name_no_ext"
    display_name="$name_no_ext"
  else
    base=$(basename "$csv" .csv)
    runroot="$OUTROOT/$base"
    display_name="$base"
  fi

  echo "\n=== Running $display_name ==="

  # Remove existing output dir to ensure clean run
  if [ -d "$runroot" ]; then
    echo "Removing existing output dir: $runroot"
    rm -rf "$runroot"
  fi

  mkdir -p "$runroot"

  # 1) Run with algorithm metrics enabled (will produce metrics.json inside subdirs)
  echo "Running with algorithm metrics..."
  "$BINARY" "$csv" "$runroot" "$RESTARTS" --metrics > "$runroot/run_with_metrics.stdout" 2> "$runroot/run_with_metrics.stderr" || true

  # 2) Run without algorithm metrics and collect system-level metrics (time + perf)
  echo "Running system-level measurement (no algorithm metrics)..."
  sysdir="$runroot/system_run"
  mkdir -p "$sysdir"

  # collect /usr/bin/time verbose output and perf stat
  /usr/bin/time -v -o "$sysdir/time.txt" perf stat -e $PERF_EVENTS -o "$sysdir/perf.txt" -- \
    $BINARY "$csv" "$sysdir" "$RESTARTS" 1>"$sysdir/stdout.log" 2>"$sysdir/stderr.log" || true

  echo "Completed $display_name. Artifacts in $runroot"
done

echo "All runs finished. Output root: $OUTROOT"

# Setup python venv and run aggregator
VENV_DIR="$(pwd)/scripts/.venv_metrics"
if [ ! -d "$VENV_DIR" ]; then
  echo "Creating venv for plotting at $VENV_DIR"
  python3 -m venv "$VENV_DIR"
fi
source "$VENV_DIR/bin/activate"
pip install --upgrade pip
pip install -r scripts/requirements.txt

echo "Running Python aggregator..."
python3 scripts/aggregate_and_plot.py "$OUTROOT" --out "$OUTROOT/plots"
deactivate