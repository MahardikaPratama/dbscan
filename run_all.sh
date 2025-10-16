#!/bin/bash

DBSCAN_EXEC="./build/dbscan"

for scenario in scenario-1 scenario-2 scenario-3; do
    for csvfile in Dataset/$scenario/*.csv; do
        if [ -f "$csvfile" ]; then
            echo "Running: $DBSCAN_EXEC $csvfile"
            $DBSCAN_EXEC "$csvfile"
        fi
    done
done
