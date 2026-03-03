#!/usr/bin/env python3

import sys
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np


def main():
    freq_file = sys.argv[1] if len(sys.argv) > 1 else "data/freq.tsv"
    out_path = sys.argv[2] if len(sys.argv) > 2 else "screenshots/zipf.png"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)

    freqs = []
    with open(freq_file, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.strip().split("\t")
            if len(parts) == 2:
                freqs.append(int(parts[1]))

    freqs.sort(reverse=True)
    ranks = np.arange(1, len(freqs) + 1)
    freqs_arr = np.array(freqs, dtype=float)

    C = freqs_arr[0]
    zipf_ideal = C / ranks

    fig, ax = plt.subplots(figsize=(10, 7))
    ax.loglog(ranks, freqs_arr, "b-", linewidth=0.8, label="Actual distribution")
    ax.loglog(ranks, zipf_ideal, "r--", linewidth=1.2, label=f"Zipf's law (C={C:.0f})")
    ax.set_xlabel("Rank (log scale)", fontsize=12)
    ax.set_ylabel("Frequency (log scale)", fontsize=12)
    ax.set_title("Zipf's Law: Term Frequency Distribution", fontsize=14)
    ax.legend(fontsize=11)
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_path, dpi=150)
    plt.close(fig)
    print(f"Saved Zipf plot to {out_path}")
    print(f"Total unique terms: {len(freqs)}")
    print(f"Top-10 frequencies: {freqs[:10]}")


if __name__ == "__main__":
    main()
