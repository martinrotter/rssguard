#!/usr/bin/env python3
"""Create an offline, deterministic, aged RSS Guard SQLite performance fixture.

The canonical application SQL files and SqliteDriver PRAGMAs are used unchanged.
The destination must not exist. This script never reads a user's RSS database.
No application queries are benchmarked here: checks only establish fixture validity.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import platform
import random
import re
import shutil
import sqlite3
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SEED = 20261014
NOW_MS = int(datetime(2026, 10, 14, tzinfo=timezone.utc).timestamp() * 1000)
START_MS = int(datetime(2016, 1, 1, tzinfo=timezone.utc).timestamp() * 1000)
ACCOUNTS = 8
FEEDS = 6000
FEEDS_PER_ACCOUNT = FEEDS // ACCOUNTS
PURGED_DURING_HISTORY = 8000
PURGED_AT_END = 42000
PRAGMAS = {
    "foreign_keys": "ON", "encoding": "'UTF-8'", "page_size": 32768,
    "cache_size": -65536, "mmap_size": 100000000, "synchronous": "OFF",
    "temp_store": "MEMORY", "journal_mode": "WAL", "busy_timeout": 5000,
}
TOPICS = ("engineering", "science", "security", "design", "open source",
          "data analysis", "climate", "space exploration", "software releases", "culture")
AUTHORS = ("Ada Novak", "Grace Kim", "Alan Chen", "Marta Svoboda", "Research Desk",
           "Science Bulletin", "Open Source Team", "Nora Evans", "Staff", "")
# IconFactory::fromByteArray accepts base64 PNG via its legacy pixmap fallback.
ICON = b"iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aF1sAAAAASUVORK5CYII="
INSERT_MESSAGE = """INSERT INTO Messages
    (id,is_read,is_important,is_deleted,is_pdeleted,feed,title,url,author,
     date_created,contents,enclosures,score,account_id,custom_id,custom_data,date_retrieved)
    VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"""


def app_schema() -> tuple[str, str, dict[str, str]]:
    """Apply DatabaseDriver::prepareScript inclusion and SQLite substitutions."""
    definitions = (ROOT / "src/librssguard/definitions/definitions.h").read_text(encoding="utf-8")
    version = re.search(r'#define\s+APP_DB_SCHEMA_VERSION\s+"(\d+)"', definitions).group(1)
    sql_dir = ROOT / "resources/sql"
    hashes = {}

    def read_statements(name: str) -> list[str]:
        source = (sql_dir / name).read_text(encoding="utf-8")
        hashes[name] = hashlib.sha256(source.encode("utf-8")).hexdigest()
        statements = []
        for statement in re.split(r"-- !\r?\n", source):
            if not statement.strip():
                continue
            if statement.startswith("!!"):
                statements.extend(read_statements(statement[3:].strip()))
            else:
                statements.append(statement)
        return statements

    script = "\n".join(read_statements("db_init_sqlite.sql"))
    for placeholder, replacement in {
        "$$": "INTEGER PRIMARY KEY", "^^": "BLOB", "**": "TEXT",
        "~~": "COLLATE NOCASE", "##": "",
        "\u00a7\u00a71": "PRAGMA foreign_keys=ON;", "\u00a7\u00a70": "PRAGMA foreign_keys=OFF;",
    }.items():
        script = script.replace(placeholder, replacement)
    return script, version, hashes


def configure(connection: sqlite3.Connection) -> None:
    for name, value in PRAGMAS.items():
        connection.execute(f"PRAGMA {name}={value}")


def memory_status() -> dict:
    if sys.platform != "win32":
        return {}
    class MemoryStatus(ctypes.Structure):
        _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong)] + [
            (name, ctypes.c_ulonglong) for name in
            ("total", "available", "pagefile", "availpage", "virtual", "availvirtual", "extra")]
    memory = MemoryStatus()
    memory.length = ctypes.sizeof(memory)
    ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(memory))
    return {"physical_total_bytes": memory.total, "physical_available_bytes": memory.available}


def canonical_schema(connection: sqlite3.Connection) -> list:
    return connection.execute("SELECT type,name,tbl_name,sql FROM sqlite_master "
                              "WHERE name NOT LIKE 'sqlite_%' ORDER BY type,name").fetchall()


def populate_tree(connection: sqlite3.Connection, version: str) -> None:
    with connection:
        connection.execute("INSERT INTO Information VALUES ('schema_version',?)", (version,))
        for account in range(1, ACCOUNTS + 1):
            connection.execute("INSERT INTO Accounts (id,ordr,type,custom_data) VALUES (?,?,?,?)", (
                account, account - 1, "std-rss", json.dumps({
                    "title": f"Historical test account {account}", "show_node_unread": True,
                    "show_node_important": True, "show_node_labels": True, "show_node_probes": True})))
            for local in range(1, 41):
                category = (account - 1) * 40 + local
                parent = -1 if local <= 10 else (account - 1) * 40 + ((local - 1) % 10) + 1
                connection.execute("INSERT INTO Categories "
                    "(id,ordr,parent_id,title,description,date_created,icon,account_id,custom_id,custom_data) "
                    "VALUES (?,?,?,?,?,?,?,?,?,?)", (category, (local - 1) % 10, parent,
                    f"{TOPICS[(local - 1) % 10].title()} folder {local}", "Offline synthetic historical folder",
                    START_MS + local * 86400000, ICON, account, f"cat-{category}", "{}"))
            for local in range(1, 25):
                label = (account - 1) * 24 + local
                connection.execute("INSERT INTO Labels (id,name,icon,custom_id,account_id) VALUES (?,?,?,?,?)",
                    (label, f"{TOPICS[(local - 1) % 10].title()} label {local}", ICON, f"label-{label}", account))
            for local in range(1, 9):
                probe = (account - 1) * 8 + local
                probe_type = int(local > 4)
                expression = ("security|release|analysis" if not probe_type else
                              f"Messages.score >= {local * 10} AND Messages.is_read = 0")
                connection.execute("INSERT INTO Probes (id,name,icon,type,fltr,account_id) VALUES (?,?,?,?,?,?)",
                    (probe, f"Saved search {local}", ICON, probe_type, expression, account))
        for feed in range(1, FEEDS + 1):
            account = 1 + (feed - 1) // FEEDS_PER_ACCOUNT
            category = -1 if feed % 13 == 0 else (account - 1) * 40 + (feed - 1) % 40 + 1
            connection.execute("""INSERT INTO Feeds
                (id,ordr,title,description,date_created,icon,category,source,update_type,
                 update_interval,is_off,is_quiet,is_rtl,add_any_datetime_articles,datetime_to_avoid,
                 keep_article_customize,keep_article_count,keep_unread_articles,keep_starred_articles,
                 recycle_articles,account_id,custom_id,custom_data,open_articles,exclude_global_unread)
                VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)""", (
                feed, (feed - 1) % 19, f"{TOPICS[feed % 10].title()} publication {feed}",
                f"Synthetic history for offline feed {feed} (never fetched)", START_MS + (feed % 365) * 86400000,
                ICON, category, f"https://publisher-{feed}.example.invalid/feed", 0, (900, 3600, 86400)[feed % 3],
                int(feed % 31 == 0), int(feed % 19 == 0), 0, int(feed % 17 == 0), 0,
                int(feed % 7 == 0), 2000 if feed % 7 == 0 else 0, 1, 1, int(feed % 9 == 0),
                account, f"feed-{feed}", json.dumps({"source_type": 0, "type": feed % 5, "encoding": "UTF-8"}),
                int(feed % 23 == 0), int(feed % 29 == 0)))
        for filter_id in range(1, 41):
            connection.execute("INSERT INTO MessageFilters (id,name,script,is_enabled,ordr) VALUES (?,?,?,?,?)",
                (filter_id, f"Historical article filter {filter_id}",
                 "function filterMessage() { return msg.title.length > 0 ? Msg.Accept : Msg.Ignore; }", int(filter_id % 9 != 0),
                 filter_id - 1))
            connection.executemany("INSERT INTO MessageFiltersInFeeds (filter,feed) VALUES (?,?)",
                ((filter_id, 1 + ((filter_id - 1) * 149 + local * 71) % FEEDS) for local in range(45)))
    connection.execute("PRAGMA wal_checkpoint(TRUNCATE)")


def body_pool() -> dict[int, list[str]]:
    rng = random.Random(SEED + 1)
    vocabulary = ("archive event practical analysis climate software release security science design "
                  "reading community framework data experiment network investigation development "
                  "engineering collection publication historical article discovery space culture ").split()
    pool = {}
    for length in (1024, 1536, 2048, 4096, 16384):
        pool[length] = []
        for variant in range(32):
            words = " ".join(rng.choices(vocabulary, k=length // 4))
            # ASCII bodies ensure these are actual byte lengths, not character estimates.
            pool[length].append(("<p>" + words)[:length - 4] + "</p>")
    return pool


def message_rows(start: int, end: int, total: int, rng: random.Random, bodies: dict):
    for message in range(start, end):
        skew = message % 100
        if skew < 10:
            feed = 1
        elif skew < 30:
            feed = 1 + ((message // 100) % ACCOUNTS) * FEEDS_PER_ACCOUNT
        elif skew < 50:
            account = (message // 100) % ACCOUNTS
            feed = 1 + account * FEEDS_PER_ACCOUNT + rng.randrange(1, 65)
        else:
            # Leave 32 genuinely empty feeds to cover newly added / dormant sources.
            feed = rng.randrange(1, FEEDS - 31)
        account = 1 + (feed - 1) // FEEDS_PER_ACCOUNT
        published = min(NOW_MS - 300000, START_MS + (NOW_MS - START_MS) * message // total
                        - rng.randrange(0, 7 * 86400000))
        retrieved = min(NOW_MS, published + rng.randrange(60000, 3600000))
        length_choice = rng.randrange(1000)
        length = (1024 if length_choice < 740 else 1536 if length_choice < 880 else
                  2048 if length_choice < 960 else 4096 if length_choice < 995 else 16384)
        contents = f"<article data-id='{message}'>{bodies[length][message % 32]}</article>"
        deleted = int(message % 17 == 0)
        pdeleted = int(deleted and message % 3 == 0)
        read = int(rng.random() > (0.30 if message > total * 0.9 else 0.06))
        enclosures = (json.dumps([{"mime": "audio/mpeg", "url":
            f"https://publisher-{feed}.example.invalid/media/{message}.mp3"}], separators=(",", ":"))
            if message % 13 == 0 else "[]")
        topic = TOPICS[feed % 10]
        custom_data = (json.dumps({"summary_version": message % 4, "history": message // 200000,
                                  "language": "en", "thread": message % 37}, separators=(",", ":"))
                       if message % 9 == 0 else "{}")
        yield (message, read, int(message % 31 == 0), deleted, pdeleted, feed,
               f"{topic.title()} report {message}: {('Release', 'Research', 'Analysis')[message % 3]}",
               f"https://publisher-{feed}.example.invalid/articles/{message}", AUTHORS[message % len(AUTHORS)],
               published, contents, enclosures, float((message * 13) % 101), account,
               f"guid-{feed}-{message}", custom_data, retrieved)


def generate(arguments) -> dict:
    destination = arguments.database.resolve()
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Exclusive filesystem create protects existing data, including empty DB files.
    with destination.open("xb"):
        pass
    started = time.monotonic()
    initial_free = shutil.disk_usage(destination.parent).free
    initial_memory = memory_status()
    script, version, hashes = app_schema()
    reference = sqlite3.connect(":memory:")
    reference.executescript(script)
    expected_schema = canonical_schema(reference)
    reference.close()
    connection = sqlite3.connect(destination, timeout=5)
    max_wal = 0
    progress_path = destination.with_name("fixture_progress.json")
    try:
        configure(connection)
        connection.executescript(script)
        populate_tree(connection, version)
        rng = random.Random(SEED)
        bodies = body_pool()
        total = arguments.messages + PURGED_DURING_HISTORY + PURGED_AT_END
        history_purge_interval = (arguments.messages + PURGED_DURING_HISTORY) // 8
        deleted_in_history = 0
        modified = 0
        for start in range(1, total + 1, arguments.batch_size):
            end = min(total + 1, start + arguments.batch_size)
            rows = list(message_rows(start, end, total, rng, bodies))
            labels = []
            for row in rows:
                message, account = row[0], row[13]
                if message % 8 == 0:
                    label = (account - 1) * 24 + 1 + (message // 8) % 24
                    labels.append((message, label))
                    if message % 40 == 0:
                        labels.append((message, (account - 1) * 24 + 1 + (message // 8 + 7) % 24))
            with connection:
                connection.executemany(INSERT_MESSAGE, rows)
                connection.executemany("INSERT INTO LabelsInMessages (message,label) VALUES (?,?)", labels)
                if end - 1 >= (deleted_in_history // 1000 + 1) * history_purge_interval and deleted_in_history < 8000:
                    old_start = deleted_in_history + 1
                    connection.execute("DELETE FROM Messages WHERE id BETWEEN ? AND ?", (old_start, old_start + 999))
                    deleted_in_history += 1000
                # Bounded old-article edits and read/star changes model usage over time.
                if start > 30000:
                    edit_start = start - 20000
                    connection.execute("UPDATE Messages SET contents=contents || ?,is_read=1,is_important=1 "
                        "WHERE id BETWEEN ? AND ?", ("<p>Updated after publication.</p>", edit_start, edit_start + 24))
                    modified += 25
            wal_path = Path(str(destination) + "-wal")
            max_wal = max(max_wal, wal_path.stat().st_size if wal_path.exists() else 0)
            connection.execute("PRAGMA wal_checkpoint(TRUNCATE)")
            free = shutil.disk_usage(destination.parent).free
            if free < arguments.reserve_bytes or destination.stat().st_size > arguments.max_db_bytes:
                raise RuntimeError(f"Fixture stopped to preserve disk reserve: DB={destination.stat().st_size}, free={free}")
            if (start - 1) % (arguments.batch_size * 25) == 0 or end == total + 1:
                progress = {"inserted": end - 1, "target_inserted": total,
                    "surviving": end - 1 - deleted_in_history,
                    "database_bytes": destination.stat().st_size, "free_disk_bytes": free,
                    "elapsed_seconds": round(time.monotonic() - started, 2)}
                progress_path.write_text(json.dumps(progress, indent=2) + "\n", encoding="utf-8")
                print(json.dumps(progress), flush=True)
        # An end-of-history cleanup leaves freed pages instead of compacting the DB.
        for start in range(total - PURGED_AT_END + 1, total + 1, arguments.batch_size):
            with connection:
                connection.execute("DELETE FROM Messages WHERE id BETWEEN ? AND ?",
                    (start, min(total, start + arguments.batch_size - 1)))
            connection.execute("PRAGMA wal_checkpoint(TRUNCATE)")
        print("Validating canonical schema, foreign keys and SQLite integrity...", flush=True)
        actual_schema = canonical_schema(connection)
        if actual_schema != expected_schema:
            raise RuntimeError("Fixture schema differs from current canonical SQLite initialization")
        fk_errors = connection.execute("PRAGMA foreign_key_check").fetchall()
        integrity = connection.execute("PRAGMA integrity_check").fetchall()
        if fk_errors or integrity != [("ok",)]:
            raise RuntimeError(f"Invalid fixture: foreign keys={fk_errors[:10]}, integrity={integrity[:10]}")
        tables = [row[1] for row in actual_schema if row[0] == "table"]
        counts = {table: connection.execute(f'SELECT COUNT(*) FROM "{table}"').fetchone()[0] for table in tables}
        if counts["Messages"] != arguments.messages:
            raise RuntimeError(f"Unexpected surviving message count: {counts['Messages']}")
        states = connection.execute("SELECT is_read,is_important,is_deleted,is_pdeleted,COUNT(*) "
                                    "FROM Messages GROUP BY 1,2,3,4").fetchall()
        account_counts = connection.execute("SELECT account_id,COUNT(*) FROM Messages GROUP BY account_id").fetchall()
        hot_feeds = connection.execute("SELECT feed,COUNT(*) FROM Messages GROUP BY feed "
                                       "ORDER BY COUNT(*) DESC LIMIT 20").fetchall()
        actual_pragmas = {name: connection.execute(f"PRAGMA {name}").fetchone()[0] for name in PRAGMAS}
        metadata = {"status": "complete", "database": str(destination), "seed": SEED,
            "fixture_generator_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "schema_version": version, "schema_sources_sha256": hashes, "canonical_schema_matches": True,
            "sqlite_version": sqlite3.sqlite_version, "python": sys.version, "platform": platform.platform(),
            "connection_pragmas": actual_pragmas, "foreign_key_check": fk_errors, "integrity_check": "ok",
            "counts": counts, "account_message_counts": account_counts, "hot_feed_counts": hot_feeds,
            "state_counts_columns": ["is_read", "is_important", "is_deleted", "is_pdeleted", "count"],
            "state_counts": states, "history": {"start_utc": "2016-01-01", "end_utc": "2026-10-14",
                "date_unit": "milliseconds_since_epoch", "inserted_messages": total,
                "physically_purged_messages": PURGED_DURING_HISTORY + PURGED_AT_END,
                "edited_old_messages": modified, "batch_size": arguments.batch_size,
                "vacuum_performed": False, "analyze_performed": False},
            "bodies": {"weights_per_1000": {"1024": 740, "1536": 140, "2048": 80, "4096": 35, "16384": 5},
                       "description": "HTML body lengths plus unique article ID wrapper and occasional edits"},
            "schema": actual_schema, "page_count": connection.execute("PRAGMA page_count").fetchone()[0],
            "freelist_count": connection.execute("PRAGMA freelist_count").fetchone()[0],
            "initial_free_disk_bytes": initial_free, "free_disk_bytes": shutil.disk_usage(destination.parent).free,
            "initial_memory": initial_memory, "max_observed_wal_bytes": max_wal,
            "wal_autocheckpoint": connection.execute("PRAGMA wal_autocheckpoint").fetchone()[0],
            "database_bytes": destination.stat().st_size, "elapsed_seconds": round(time.monotonic() - started, 2),
            "limitations": ["Synthetic offline fixture; it is not a captured user database.",
                "Python SQLite runtime may differ from Qt QSQLITE runtime; connection settings match source.",
                "All synthetic accounts use std-rss; service-specific blobs are not simulated.",
                "No extra indexes, ANALYZE, VACUUM or application query benchmarks were applied."]}
        destination.with_name("fixture_metadata.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        progress_path.write_text(json.dumps({"status": "complete", "messages": counts["Messages"]}, indent=2) + "\n",
                                 encoding="utf-8")
        print(json.dumps({key: metadata[key] for key in ("counts", "database_bytes", "freelist_count", "elapsed_seconds")}), flush=True)
        return metadata
    finally:
        connection.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path)
    parser.add_argument("--messages", type=int, default=2000000)
    parser.add_argument("--batch-size", type=int, default=4000)
    parser.add_argument("--reserve-bytes", type=int, default=3000000000)
    parser.add_argument("--max-db-bytes", type=int, default=4500000000)
    arguments = parser.parse_args()
    if arguments.messages < 250000 or arguments.batch_size < 1:
        parser.error("messages must be >=250000; batch size must be positive")
    generate(arguments)


if __name__ == "__main__":
    main()
