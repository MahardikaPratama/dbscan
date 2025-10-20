#!/bin/bash

DBSCAN_EXEC="./build/dbscan"

for scenario in scenario-1 scenario-2 scenario-3; do
    for csvfile in input/$scenario/*.csv; do
        if [ -f "$csvfile" ]; then
            echo "Running: $DBSCAN_EXEC $csvfile"
            $DBSCAN_EXEC "$csvfile" output/$(basename "$scenario")/$(basename "$csvfile" .csv)_output.csv --metrics
        fi
    done
done
