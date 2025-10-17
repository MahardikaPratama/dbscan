#!/usr/bin/env python3
"""
Aggregate metrics.json files and produce summary CSV and plots.
Usage:
  python3 scripts/aggregate_and_plot.py <results_root>
  python3 scripts/aggregate_and_plot.py --file <metrics.json>
"""
import json
from pathlib import Path
import argparse
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
import os


def load_metrics(path: Path):
    with open(path, 'r') as f:
        return json.load(f)


def collect_from_root(root: Path):
    rows = []
    for p in root.rglob('metrics.json'):
        try:
            m = load_metrics(p)
        except Exception:
            continue
        algo = p.parent.name
        dataset = p.parents[1].name if len(p.parents) > 1 else p.parent.name
        outer = p.parents[2].name if len(p.parents) > 2 else 'root'
        final_sse = m.get('final_sse')
        num_points = m.get('num_points') or m.get('n') or None
        rmse = m.get('rmse_km')
        # If rmse not present, try to compute (safely)
        try:
            if (rmse is None or rmse == 0) and final_sse is not None and num_points:
                rmse = float(final_sse) / float(num_points)
                rmse = float(rmse) ** 0.5
        except Exception:
            rmse = None

        rows.append({
            'path': str(p),
            'outer': outer,
            'dataset': dataset,
            'algorithm': algo,
            'K': m.get('K'),
            'threads': m.get('threads'),
            'final_sse': final_sse,
            'rmse_km': rmse,
            'mean_dist_km': m.get('mean_dist_km'),
            'median_dist_km': m.get('median_dist_km'),
            'max_dist_km': m.get('max_dist_km'),
            'min_dist_km': m.get('min_dist_km'),
            'iterations': m.get('iterations'),
            'distance_calls': m.get('distance_calls'),
            'peak_rss_kb': m.get('peak_rss_kb'),
            'init_ms': m.get('phases_ms', {}).get('initialize', 0),
            'assign_ms': m.get('phases_ms', {}).get('assign', 0),
            'update_ms': m.get('phases_ms', {}).get('update', 0),
        })
    return pd.DataFrame(rows)


def single_file_report(path: Path, outdir: Path):
    m = load_metrics(path)
    outdir.mkdir(parents=True, exist_ok=True)
    # write JSON copy
    (outdir / 'metrics_copy.json').write_text(json.dumps(m, indent=2))
    # simple textual summary
    with open(outdir / 'summary.txt', 'w') as f:
        f.write(f"K: {m.get('K')}\n")
        f.write(f"threads: {m.get('threads')}\n")
        f.write(f"final_sse: {m.get('final_sse')}\n")
        f.write(f"iterations: {m.get('iterations')}\n")
        f.write(f"distance_calls: {m.get('distance_calls')}\n")
    print('Wrote single-file summary to', outdir)


def _coerce_numeric_and_log10(df: pd.DataFrame) -> pd.DataFrame:
    numeric_cols = [
        'final_sse', 'rmse_km', 'mean_dist_km', 'median_dist_km', 'max_dist_km',
        'min_dist_km', 'iterations', 'distance_calls', 'peak_rss_kb',
        'init_ms', 'assign_ms', 'update_ms'
    ]
    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
    df['final_sse_log10'] = df['final_sse'].replace(0, np.nan).apply(
        lambda x: np.log10(x) if pd.notnull(x) and x > 0 else np.nan)
    return df


def _save_outer_group_plots(outer: str, outer_df: pd.DataFrame, outdir: Path):
    outer_out = outdir / outer
    outer_out.mkdir(parents=True, exist_ok=True)
    outer_df.to_csv(outer_out / 'results_summary.csv', index=False)

    # per-dataset plots inside this outer group
    for ds, sub in outer_df.groupby('dataset'):
        safe_ds = ds.replace('/', '_')
        plt.figure(figsize=(8, 4))
        if sub['rmse_km'].notna().any():
            sns.barplot(data=sub, x='algorithm', y='rmse_km')
            plt.ylabel('RMSE (km)')
            plt.title(f'Final RMSE (km) — {outer}/{ds}')
            fname = f'{safe_ds}_final_rmse.png'
        else:
            sns.barplot(data=sub, x='algorithm', y='final_sse_log10')
            plt.ylabel('log10(Final SSE)')
            plt.title(f'Final SSE (log10) — {outer}/{ds}')
            fname = f'{safe_ds}_final_sse_log10.png'
        plt.tight_layout()
        plt.savefig(outer_out / fname)
        plt.close()

    # group-level plots: compare algorithms across all datasets in this outer group
    plt.figure(figsize=(8, 4))
    if outer_df['rmse_km'].notna().any():
        sns.boxplot(data=outer_df, x='algorithm', y='rmse_km')
        plt.ylabel('RMSE (km)')
        plt.title(f'RMSE by algorithm — {outer}')
        fname = f'{outer}_rmse_by_algorithm.png'
    else:
        sns.boxplot(data=outer_df, x='algorithm', y='final_sse_log10')
        plt.ylabel('log10(Final SSE)')
        plt.title(f'Final SSE (log10) by algorithm — {outer}')
        fname = f'{outer}_sse_log10_by_algorithm.png'
    plt.tight_layout()
    plt.savefig(outer_out / fname)
    plt.close()

    # additional stats plots: mean_dist_km distribution and peak RSS
    if 'mean_dist_km' in outer_df.columns:
        plt.figure(figsize=(8, 4))
        sns.boxplot(data=outer_df, x='algorithm', y='mean_dist_km')
        plt.ylabel('Mean dist (km)')
        plt.title(f'Mean point-to-centroid distance by algorithm — {outer}')
        plt.tight_layout()
        plt.savefig(outer_out / f'{outer}_mean_dist_by_algorithm.png')
        plt.close()

    plt.figure(figsize=(8, 4))
    sns.boxplot(data=outer_df, x='algorithm', y='peak_rss_kb')
    plt.ylabel('Peak RSS (KB)')
    plt.title(f'Peak RSS by algorithm — {outer}')
    plt.tight_layout()
    plt.savefig(outer_out / f'{outer}_peak_rss_by_algorithm.png')
    plt.close()


def _save_phase_times(outer: str, outer_df: pd.DataFrame, outdir: Path):
    outer_out = outdir / outer
    for ds, sub in outer_df.groupby('dataset'):
        safe_ds = ds.replace('/', '_')
        agg = sub.groupby('algorithm')[['init_ms', 'assign_ms', 'update_ms']].sum()
        if agg.empty:
            continue
        fig, ax = plt.subplots(figsize=(8, 4))
        agg.plot(kind='bar', stacked=True, ax=ax)
        ax.set_ylabel('ms')
        ax.set_title(f'Phase times — {outer}/{ds}')
        ax.legend(title='phase')
        fig.tight_layout()
        fig.savefig(outer_out / f'{safe_ds}_phase_times.png')
        plt.close(fig)


def _save_misc_plots(df: pd.DataFrame, outdir: Path):
    fig, ax = plt.subplots(figsize=(7, 5))
    sns.scatterplot(data=df, x='distance_calls', y='final_sse', hue='algorithm', ax=ax)
    ax.set_xscale('log')
    ax.set_yscale('log')
    fig.tight_layout()
    fig.savefig(outdir / 'distance_vs_sse.png')
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(7, 5))
    sns.boxplot(data=df, x='algorithm', y='iterations', ax=ax)
    ax.set_title('Iterations by algorithm')
    fig.tight_layout()
    fig.savefig(outdir / 'iterations_box.png')
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(7, 5))
    sns.histplot(data=df, x='peak_rss_kb', hue='algorithm', multiple='stack', ax=ax)
    ax.set_title('Peak RSS (KB)')
    fig.tight_layout()
    fig.savefig(outdir / 'peak_rss_hist.png')
    plt.close(fig)


def plot_aggregates(df: pd.DataFrame, outdir: Path):
    outdir.mkdir(parents=True, exist_ok=True)
    df.to_csv(outdir / 'results_summary.csv', index=False)
    sns.set(style='whitegrid')

    df = _coerce_numeric_and_log10(df)

    # Execute plotting tasks (delegated to helpers to keep this function simple)
    for outer, outer_df in df.groupby('outer'):
        _save_outer_group_plots(outer, outer_df, outdir)

    for outer, outer_df in df.groupby('outer'):
        _save_phase_times(outer, outer_df, outdir)

    _save_misc_plots(df, outdir)

    print('Wrote aggregate CSV and plots to', outdir)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('root', nargs='?', default='Results',
                        help='results root or single metrics.json when using --file')
    parser.add_argument('--file', help='single metrics.json file to summarize')
    parser.add_argument(
        '--out', help='output directory for plots/csv', default='plots')
    args = parser.parse_args()

    outdir = Path(args.out)

    if args.file:
        p = Path(args.file)
        if not p.exists():
            print('File not found:', p)
            return
        single_file_report(p, outdir)
        return

    root = Path(args.root)
    if not root.exists():
        print('Results root not found:', root)
        return
    df = collect_from_root(root)
    if df.empty:
        print('No metrics.json files found under', root)
        return
    plot_aggregates(df, outdir)


if __name__ == '__main__':
    main()