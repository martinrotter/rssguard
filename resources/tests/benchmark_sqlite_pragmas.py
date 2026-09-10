#!/usr/bin/env python3
"""Create a reproducible SQLite PRAGMA benchmark matrix.

This is deliberately independent of RSS Guard. It uses only generated data in
the output directory and compares page size, journal/synchronous mode, cache,
mmap and temporary-store configurations.

    python resources/tests/benchmark_sqlite_pragmas.py
    python resources/tests/benchmark_sqlite_pragmas.py --messages 100000
    python resources/tests/benchmark_sqlite_pragmas.py --output D:/bench/sqlite-pragmas
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import os
import platform
import random
import sqlite3
import statistics
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


SEED = 20260910
PAGE_SIZES = (4096, 8192, 16384, 32768)
WARMUPS = 2
READ_RUNS = 15
WRITE_RUNS = 10


@dataclass(frozen=True)
class Profile:
    name: str
    journal_mode: str
    synchronous: str
    cache_size: int
    mmap_size: int
    temp_store: str
    busy_timeout: int = 5000


# The focused default matrix isolates one factor at a time. It runs the
# baseline at every page size, then runs each PRAGMA variation at 32 KiB.
# ``--full-matrix`` remains available for exhaustive cross-products.
BASELINE = Profile("wal_normal_cache64m_mmap100m_temp_memory", "WAL", "NORMAL", -65536, 100_000_000, "MEMORY")
VARIATIONS = (
    Profile("wal_normal_cache8m", "WAL", "NORMAL", -8192, 100_000_000, "MEMORY"),
    Profile("wal_normal_cache256m", "WAL", "NORMAL", -262144, 100_000_000, "MEMORY"),
    Profile("wal_normal_mmap_off", "WAL", "NORMAL", -65536, 0, "MEMORY"),
    Profile("wal_normal_temp_file", "WAL", "NORMAL", -65536, 100_000_000, "FILE"),
    Profile("wal_full_cache64m", "WAL", "FULL", -65536, 100_000_000, "MEMORY"),
    Profile("delete_full_cache64m", "DELETE", "FULL", -65536, 0, "FILE"),
)


def timer_ns() -> int:
    return time.perf_counter_ns()


def milliseconds(start: int) -> float:
    return (time.perf_counter_ns() - start) / 1_000_000


def p95(values: list[float]) -> float:
    return sorted(values)[max(0, math.ceil(len(values) * .95) - 1)]


def stats(values: list[float]) -> dict[str, float]:
    return {"min_ms": min(values), "median_ms": statistics.median(values),
            "mean_ms": statistics.fmean(values), "p95_ms": p95(values), "max_ms": max(values)}


def body(message_id: int) -> str:
    """A stable mix of small, medium and large text records."""
    rng = random.Random(SEED + message_id)
    size = rng.randint(300, 1500) if message_id % 10 < 7 else rng.randint(2_000, 12_000)
    if message_id % 100 == 0:
        size = rng.randint(20_000, 60_000)
    line = f"SQLite benchmark message {message_id}: indexed reads, transactions and page-cache behaviour. "
    return (line * (size // len(line) + 1))[:size]


def configure(connection: sqlite3.Connection, profile: Profile) -> None:
    connection.execute(f"PRAGMA cache_size={profile.cache_size}")
    connection.execute(f"PRAGMA mmap_size={profile.mmap_size}")
    connection.execute(f"PRAGMA synchronous={profile.synchronous}")
    connection.execute(f"PRAGMA temp_store={profile.temp_store}")
    connection.execute(f"PRAGMA busy_timeout={profile.busy_timeout}")


def create_database(path: Path, page_size: int, profile: Profile, messages: int) -> tuple[float, float]:
    connection = sqlite3.connect(path, isolation_level=None)
    try:
        # page_size must be the first storage-affecting setting on a new file.
        connection.execute(f"PRAGMA page_size={page_size}")
        connection.execute(f"PRAGMA journal_mode={profile.journal_mode}")
        configure(connection, profile)
        start = timer_ns()
        connection.executescript("""
            CREATE TABLE messages (
              id INTEGER PRIMARY KEY,
              feed INTEGER NOT NULL,
              created INTEGER NOT NULL,
              is_read INTEGER NOT NULL,
              is_starred INTEGER NOT NULL,
              title TEXT NOT NULL,
              body TEXT NOT NULL
            );
            CREATE INDEX messages_feed_created ON messages(feed, created DESC);
            CREATE INDEX messages_read_created ON messages(is_read, created DESC);
            CREATE INDEX messages_starred_created ON messages(is_starred, created DESC);
        """)
        schema_ms = milliseconds(start)
        start = timer_ns()
        connection.execute("BEGIN")
        rows: list[tuple[Any, ...]] = []
        for message_id in range(1, messages + 1):
            rows.append((message_id, message_id % 1000, 1_700_000_000_000 + message_id * 60_000,
                         int(message_id % 7 == 0), int(message_id % 31 == 0),
                         f"Topic {message_id % 20} message {message_id}", body(message_id)))
            if len(rows) == 1000:
                connection.executemany("INSERT INTO messages VALUES (?, ?, ?, ?, ?, ?, ?)", rows)
                rows.clear()
        if rows:
            connection.executemany("INSERT INTO messages VALUES (?, ?, ?, ?, ?, ?, ?)", rows)
        connection.execute("COMMIT")
        return schema_ms, milliseconds(start)
    finally:
        connection.close()


def open_database(path: Path, profile: Profile) -> sqlite3.Connection:
    connection = sqlite3.connect(path, timeout=profile.busy_timeout / 1000, isolation_level=None)
    configure(connection, profile)
    return connection


def sample_query(connection: sqlite3.Connection, sql: str, params: tuple[Any, ...], page_size: int,
                 profile: Profile, test: str, mode: str, runs: int = READ_RUNS) -> list[dict[str, Any]]:
    for _ in range(WARMUPS):
        connection.execute(sql, params).fetchall()
    rows = []
    for run in range(1, runs + 1):
        start = timer_ns()
        connection.execute(sql, params).fetchall()
        rows.append(result_row(page_size, profile, test, mode, run, milliseconds(start)))
    return rows


def result_row(page_size: int, profile: Profile, test: str, mode: str, run: int, elapsed: float, **extra: Any) -> dict[str, Any]:
    return {"page_size": page_size, "profile": profile.name, "test": test, "mode": mode,
            "run": run, "elapsed_ms": elapsed, **extra}


def benchmark_database(path: Path, page_size: int, profile: Profile, messages: int) -> list[dict[str, Any]]:
    connection = open_database(path, profile)
    try:
        rows: list[dict[str, Any]] = []
        rows += sample_query(connection, "SELECT id,feed,created,is_read,is_starred,title FROM messages "
                             "ORDER BY created DESC LIMIT 500", (), page_size, profile, "message_list_500", "warm")
        rows += sample_query(connection, "SELECT id,title FROM messages WHERE is_read=0 ORDER BY created DESC LIMIT 500", (),
                             page_size, profile, "unread_list_500", "warm")
        rows += sample_query(connection, "SELECT id,title FROM messages WHERE feed=? ORDER BY created DESC LIMIT 100", (17,),
                             page_size, profile, "feed_list_100", "warm")
        rows += sample_query(connection, "SELECT feed,COUNT(*),SUM(NOT is_read) FROM messages GROUP BY feed", (),
                             page_size, profile, "feed_counts", "warm")
        # No supporting index: this deliberately exercises SQLite's temporary
        # sort storage, making temp_store variations observable.
        rows += sample_query(connection, "SELECT id,title FROM messages ORDER BY title DESC LIMIT 500", (),
                             page_size, profile, "title_sort_500", "warm")
        for test, sql in (("point_lookup_metadata", "SELECT id,title,created FROM messages WHERE id=?"),
                          ("point_lookup_body", "SELECT id,title,body FROM messages WHERE id=?")):
            rng = random.Random(SEED + len(test))
            ids = [rng.randint(1, messages) for _ in range(WARMUPS + READ_RUNS)]
            for _ in range(WARMUPS):
                connection.execute(sql, (ids.pop(),)).fetchone()
            for run in range(1, READ_RUNS + 1):
                start = timer_ns()
                connection.execute(sql, (ids.pop(),)).fetchone()
                rows.append(result_row(page_size, profile, test, "warm", run, milliseconds(start)))
        next_id = messages + 1
        for batch_size in (20, 100, 500):
            for _ in range(WARMUPS):
                next_id = insert_batch(connection, next_id, batch_size)
            for run in range(1, WRITE_RUNS + 1):
                start = timer_ns()
                next_id = insert_batch(connection, next_id, batch_size)
                rows.append(result_row(page_size, profile, f"insert_{batch_size}", "warm", run, milliseconds(start)))
        for count in (1, 100, 1000):
            sql = "UPDATE messages SET is_read=NOT is_read WHERE id IN (%s)" % ",".join("?" * count)
            parameters = tuple(range(1, count + 1))
            for _ in range(WARMUPS):
                connection.execute("BEGIN"); connection.execute(sql, parameters); connection.execute("COMMIT")
            for run in range(1, WRITE_RUNS + 1):
                start = timer_ns()
                connection.execute("BEGIN"); connection.execute(sql, parameters); connection.execute("COMMIT")
                rows.append(result_row(page_size, profile, f"update_read_{count}", "warm", run, milliseconds(start)))
        if profile.journal_mode == "WAL":
            connection.execute("PRAGMA wal_checkpoint(TRUNCATE)").fetchall()
        return rows
    finally:
        connection.close()


def insert_batch(connection: sqlite3.Connection, start_id: int, count: int) -> int:
    connection.execute("BEGIN")
    try:
        connection.executemany("INSERT INTO messages VALUES (?, ?, ?, 0, 0, ?, ?)",
                               ((message_id, message_id % 1000, 1_800_000_000_000 + message_id,
                                 f"Refresh message {message_id}", body(message_id))
                                for message_id in range(start_id, start_id + count)))
        connection.execute("COMMIT")
    except Exception:
        connection.execute("ROLLBACK")
        raise
    return start_id + count


def fresh_connection_samples(path: Path, page_size: int, profile: Profile) -> list[dict[str, Any]]:
    sql = "SELECT id,title FROM messages WHERE is_read=0 ORDER BY created DESC LIMIT 500"
    rows = []
    for run in range(1, READ_RUNS + 1):
        start = timer_ns()
        connection = open_database(path, profile)
        try:
            connection.execute(sql).fetchall()
        finally:
            connection.close()
        rows.append(result_row(page_size, profile, "unread_list_500", "fresh_connection", run, milliseconds(start)))
    return rows


def database_info(path: Path, profile: Profile) -> dict[str, Any]:
    connection = open_database(path, profile)
    try:
        values = {name: connection.execute(f"PRAGMA {name}").fetchone()[0]
                  for name in ("page_size", "page_count", "freelist_count", "journal_mode", "cache_size", "mmap_size")}
        values["logical_size_bytes"] = int(values["page_size"]) * int(values["page_count"])
    finally:
        connection.close()
    values["db_size_bytes"] = path.stat().st_size
    values["wal_size_bytes"] = Path(str(path) + "-wal").stat().st_size if Path(str(path) + "-wal").exists() else 0
    return values


def summarize(rows: list[dict[str, Any]], info: dict[tuple[int, str], dict[str, Any]]) -> list[dict[str, Any]]:
    groups: dict[tuple[int, str, str, str], list[float]] = {}
    for row in rows:
        groups.setdefault((row["page_size"], row["profile"], row["test"], row["mode"]), []).append(row["elapsed_ms"])
    result = []
    for (page_size, profile, test, mode), values in sorted(groups.items()):
        result.append({"page_size": page_size, "profile": profile, "test": test, "mode": mode,
                       "runs": len(values), **stats(values), **info[(page_size, profile)]})
    return result


def write_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    fields = sorted({key for row in rows for key in row})
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def write_report(output: Path, summary: list[dict[str, Any]], profiles: tuple[Profile, ...], full_matrix: bool) -> None:
    lines = ["# SQLite PRAGMA benchmark", "", "Generated deterministic data only; no RSS Guard database or application code is used.",
             "", "## Matrix", "", "| Profile | Journal | Synchronous | Cache | mmap | temp store |", "|---|---|---|---:|---:|---|"]
    for profile in profiles:
        lines.append(f"| {profile.name} | {profile.journal_mode} | {profile.synchronous} | {profile.cache_size} | {profile.mmap_size} | {profile.temp_store} |")
    coverage = "Every profile is tested at every page size." if full_matrix else "The baseline is tested at 4K, 8K, 16K and 32K; each variation is tested at 32K."
    lines.extend(["", f"{coverage} Timings use `perf_counter_ns`, {WARMUPS} warmups, {READ_RUNS} read samples and {WRITE_RUNS} write samples.",
                  "", "## Results", "", "| Profile | Page size | Test | Mode | Median ms | p95 ms | DB size |", "|---|---:|---|---|---:|---:|---:|"])
    for row in summary:
        lines.append(f"| {row['profile']} | {row['page_size']} | {row['test']} | {row['mode']} | {row['median_ms']:.3f} | {row['p95_ms']:.3f} | {row['db_size_bytes']:,} |")
    lines.extend(["", "## Notes", "", "`fresh_connection` is not a true cold-cache test: the operating-system cache is intentionally not flushed.",
                  "Database creation and bulk import appear as one sample because each matrix member is generated once. Repeat the command on an empty output directory to compare creation variability.",
                  "`title_sort_500` deliberately has no supporting index, so it is the useful comparison for `temp_store`.", ""])
    (output / "report.md").write_text("\n".join(lines), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output", type=Path, default=Path("benchmark-results"))
    parser.add_argument("--messages", type=int, default=25_000, help="generated rows per matrix member (default: 25000)")
    parser.add_argument("--full-matrix", action="store_true", help="test every PRAGMA profile at every page size; slower and more disk-intensive")
    arguments = parser.parse_args()
    if arguments.messages < 1_000:
        raise SystemExit("--messages must be at least 1000")
    output = arguments.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise SystemExit(f"Output directory must be empty: {output}")
    output.mkdir(parents=True, exist_ok=True)
    environment = {"platform": platform.platform(), "machine": platform.machine(), "processor": platform.processor(),
                   "cpu_count": os.cpu_count(), "python": sys.version, "sqlite_version": sqlite3.sqlite_version,
                   "seed": SEED, "messages": arguments.messages, "baseline": asdict(BASELINE),
                   "variations": [asdict(profile) for profile in VARIATIONS], "full_matrix": arguments.full_matrix}
    (output / "environment.json").write_text(json.dumps(environment, indent=2) + "\n", encoding="utf-8")
    detailed: list[dict[str, Any]] = []
    info: dict[tuple[int, str], dict[str, Any]] = {}
    matrix: list[tuple[Profile, int]] = [(BASELINE, page_size) for page_size in PAGE_SIZES]
    if arguments.full_matrix:
        matrix.extend((profile, page_size) for profile in VARIATIONS for page_size in PAGE_SIZES)
    else:
        matrix.extend((profile, 32768) for profile in VARIATIONS)
    active_profiles = (BASELINE, *VARIATIONS)
    for profile, page_size in matrix:
        name = f"{profile.name}-p{page_size // 1024}k"
        database = output / f"{name}.db"
        print(f"{name}: creating {arguments.messages:,} messages...", flush=True)
        schema_ms, import_ms = create_database(database, page_size, profile, arguments.messages)
        detailed.append(result_row(page_size, profile, "schema_create", "creation", 1, schema_ms))
        detailed.append(result_row(page_size, profile, "bulk_import", "creation", 1, import_ms))
        detailed.extend(benchmark_database(database, page_size, profile, arguments.messages))
        detailed.extend(fresh_connection_samples(database, page_size, profile))
        info[(page_size, profile.name)] = database_info(database, profile)
    summary = summarize(detailed, info)
    write_csv(output / "detailed-results.csv", detailed)
    write_csv(output / "summary.csv", summary)
    write_report(output, summary, active_profiles, arguments.full_matrix)
    print(f"Finished. Results are in: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
