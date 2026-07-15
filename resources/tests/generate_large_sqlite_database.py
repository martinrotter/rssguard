#!/usr/bin/env python3
"""Create a large, realistic SQLite database for RSS Guard performance testing.

Usage:
    python resources/tests/generate_large_sqlite_database.py [path/to/database.db]

The target must not exist or must be an empty SQLite database. The script downloads
curated OPML catalogues to obtain 600 unique feed URLs, then creates one standard
RSS account with folders, feeds, filters, articles, labels and label assignments.
"""

from __future__ import annotations

import argparse
import base64
import json
import random
import sqlite3
import struct
import sys
import time
import urllib.request
import xml.etree.ElementTree as element_tree
import zlib
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
from urllib.parse import unquote, urlparse


ACCOUNT_ID = 1
CATEGORY_COUNT = 30
FEED_COUNT = 600
ARTICLE_COUNT = 300_000
LABEL_COUNT = 10
LABELLED_ARTICLE_COUNT = 40_000
FILTER_COUNT = 10
FILTER_FEED_ASSIGNMENT_COUNT = 30
FILTER_ASSIGNMENT_SEED = 20260715
ARTICLE_BATCH_SIZE = 5_000
SCHEMA_VERSION = "103"
CATALOGUE_DOWNLOAD_TIMEOUT_SECONDS = 20
FEED_DOWNLOAD_TIMEOUT_SECONDS = 8
FEED_DOWNLOAD_MAXIMUM_BYTES = 512 * 1024
FEED_DOWNLOAD_WORKERS = 12
FEED_CLASSIFICATION_BATCH_SIZE = 60
MAXIMUM_FEED_CANDIDATES = 1_200

RSS_0X = 0
RSS_2X = 1
RDF = 2
ATOM_10 = 3
JSON_FEED = 4
SITEMAP = 5
ICALENDAR = 6

# These projects publish curated OPML catalogues. The Plenary project moved from
# plenaryapp to spians, but its catalogue URLs remain publicly available.
CATALOGUE_URLS = (
    "https://raw.githubusercontent.com/JackyST0/awesome-rsshub-routes/main/feeds.opml",
    "https://raw.githubusercontent.com/spians/awesome-RSS-feeds/master/recommended/with_category/Tech.opml",
    "https://raw.githubusercontent.com/spians/awesome-RSS-feeds/master/recommended/with_category/Programming.opml",
    "https://raw.githubusercontent.com/spians/awesome-RSS-feeds/master/recommended/with_category/News.opml",
    "https://raw.githubusercontent.com/spians/awesome-RSS-feeds/master/recommended/with_category/Science.opml",
    "https://raw.githubusercontent.com/RSS-Renaissance/awesome-AI-feeds/master/feedlist.opml",
    "https://raw.githubusercontent.com/simevidas/web-dev-feeds/master/feeds.opml",
    "https://raw.githubusercontent.com/zer0yu/CyberSecurityRSS/master/CyberSecurityRSS.opml",
    "https://raw.githubusercontent.com/mrtouch93/awesome-security-feed/main/security_feeds.opml",
    "https://raw.githubusercontent.com/zhaoolee/garss/main/zhaoolee_github_garss_subscription_list_v2.opml",
)

TOPICS = (
    "engineering",
    "science",
    "security",
    "design",
    "open source",
    "data analysis",
    "climate",
    "space exploration",
    "software releases",
    "internet culture",
)

AUTHORS = (
    "Alex Morgan",
    "Dana Novak",
    "Jordan Lee",
    "Marta Silva",
    "Noah Patel",
    "Riley Chen",
    "Sam Taylor",
    "Tomas Kral",
)

FILTERS = (
    (
        "Prioritize security updates",
        """function filterMessage() {
  if (/security/i.test(msg.title)) {
    msg.isImportant = true;
    msg.score = Math.max(msg.score, 90);
    var label = acc.findLabel("Load test label 01");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Tag science articles",
        """function filterMessage() {
  if (/science|space exploration/i.test(msg.title)) {
    msg.score = Math.max(msg.score, 70);
    var label = acc.findLabel("Load test label 02");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Highlight open-source news",
        """function filterMessage() {
  if (/open source|software releases/i.test(msg.title)) {
    msg.isImportant = true;
    var label = acc.findLabel("Load test label 03");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Tag image-rich articles",
        """function filterMessage() {
  if (msg.contents.indexOf("<img") >= 0) {
    var label = acc.findLabel("Load test label 04");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Rank engineering articles",
        """function filterMessage() {
  if (/engineering|data analysis/i.test(msg.title)) {
    msg.score = Math.max(msg.score, 65);
    var label = acc.findLabel("Load test label 05");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Boost weekend articles",
        """function filterMessage() {
  var day = msg.created.getDay();
  if (day === 0 || day === 6) {
    msg.score = Math.min(msg.score + 15, 100);
    var label = acc.findLabel("Load test label 06");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Flag selected authors",
        """function filterMessage() {
  if (/Alex Morgan|Marta Silva/.test(msg.author)) {
    var label = acc.findLabel("Load test label 07");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Emphasize internet culture",
        """function filterMessage() {
  if (/internet culture|design/i.test(msg.title)) {
    msg.score = Math.max(msg.score, 55);
    var label = acc.findLabel("Load test label 08");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Annotate generated HTML",
        """function filterMessage() {
  if (msg.contents.indexOf("<article>") >= 0) {
    msg.contents = msg.contents.replace("<article>", "<article data-load-test=\\\"true\\\">");
    var label = acc.findLabel("Load test label 09");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
    (
        "Review unread security articles",
        """function filterMessage() {
  if (!msg.isRead && /security|climate/i.test(msg.title)) {
    msg.isImportant = true;
    msg.score = Math.max(msg.score, 85);
    var label = acc.findLabel("Load test label 10");
    if (label) {
      msg.assignLabel(label);
    }
  }
  return Msg.Accept;
}""",
    ),
)


@dataclass(frozen=True)
class FeedInfo:
    source_url: str
    feed_type: int
    title: str

ICON_COLORS = (
    (49, 130, 206),
    (56, 161, 105),
    (214, 93, 14),
    (128, 90, 213),
    (213, 63, 140),
    (45, 55, 72),
    (49, 151, 149),
    (192, 86, 33),
    (49, 151, 149),
    (113, 128, 150),
)


def png_chunk(kind: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)


def generated_icon(seed: int) -> bytes:
    """Create a small transparent PNG without requiring Pillow or network access."""
    size = 32
    red, green, blue = ICON_COLORS[seed % len(ICON_COLORS)]
    scanlines = bytearray()

    for y in range(size):
        scanlines.append(0)  # PNG filter type: None.

        for x in range(size):
            distance_x = x - (size - 1) / 2
            distance_y = y - (size - 1) / 2
            inside_circle = distance_x * distance_x + distance_y * distance_y <= 14 * 14
            stripe = (x + y + seed) % 7 == 0

            if inside_circle:
                brightness = 1.18 if stripe else 1.0
                scanlines.extend(
                    (
                        min(255, int(red * brightness)),
                        min(255, int(green * brightness)),
                        min(255, int(blue * brightness)),
                        255,
                    )
                )
            else:
                scanlines.extend((0, 0, 0, 0))

    png = b"\x89PNG\r\n\x1a\n"
    png += png_chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
    png += png_chunk(b"IDAT", zlib.compress(bytes(scanlines), level=9))
    png += png_chunk(b"IEND", b"")
    return base64.b64encode(png)


def feed_title_from_url(source_url: str) -> str:
    parsed_url = urlparse(source_url)
    host = parsed_url.netloc.removeprefix("www.") or "RSS source"
    path_parts = [unquote(part).replace("-", " ").replace("_", " ") for part in parsed_url.path.split("/") if part]
    description = " ".join(path_parts[-2:]).strip() or "feed"

    return f"{host}: {description.title()}"


def repository_root() -> Path:
    return Path(__file__).resolve().parents[2]


def sqlite_schema() -> str:
    """Expand the SQLite-specific placeholders in the canonical schema."""
    schema_path = repository_root() / "resources" / "sql" / "db_init.sql"
    schema = schema_path.read_text(encoding="utf-8")

    return (
        schema.replace("$$", "INTEGER PRIMARY KEY")
        .replace("^^", "BLOB")
        .replace("**", "TEXT")
        .replace("~~", "COLLATE NOCASE")
        .replace("##", "")
        .replace("\u00a7\u00a71", "PRAGMA foreign_keys=ON;")
        .replace("\u00a7\u00a70", "PRAGMA foreign_keys=OFF;")
    )


def download_document(url: str, timeout_seconds: int, maximum_bytes: int) -> bytes:
    request = urllib.request.Request(url, headers={"User-Agent": "RSSGuard-load-test-generator/1.0"})

    with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
        return response.read(maximum_bytes)


def download_opml(url: str) -> list[str]:
    document = download_document(url, CATALOGUE_DOWNLOAD_TIMEOUT_SECONDS, 4 * 1024 * 1024)

    root = element_tree.fromstring(document)
    urls: list[str] = []

    for outline in root.iter("outline"):
        feed_url = outline.attrib.get("xmlUrl") or outline.attrib.get("xmlurl")

        if feed_url and feed_url.startswith(("https://", "http://")):
            urls.append(feed_url.strip())

    return urls


def local_name(element: element_tree.Element) -> str:
    return element.tag.rsplit("}", maxsplit=1)[-1].lower()


def child_text(element: element_tree.Element, name: str) -> str:
    for child in element:
        if local_name(child) == name:
            return " ".join(child.itertext()).strip()

    return ""


def classify_feed(source_url: str) -> FeedInfo | None:
    try:
        document = download_document(source_url, FEED_DOWNLOAD_TIMEOUT_SECONDS, FEED_DOWNLOAD_MAXIMUM_BYTES)
    except OSError:
        return None

    stripped_document = document.lstrip()

    try:
        json_document = json.loads(stripped_document)
    except (UnicodeDecodeError, json.JSONDecodeError):
        json_document = None

    if isinstance(json_document, dict) and "jsonfeed.org" in str(json_document.get("version", "")):
        return FeedInfo(source_url, JSON_FEED, str(json_document.get("title") or feed_title_from_url(source_url)))

    if stripped_document.decode("utf-8", errors="ignore").upper().startswith("BEGIN:VCALENDAR"):
        return FeedInfo(source_url, ICALENDAR, feed_title_from_url(source_url))

    try:
        root = element_tree.fromstring(document)
    except element_tree.ParseError:
        return None

    root_name = local_name(root)

    if root_name == "rss":
        channel = next((child for child in root if local_name(child) == "channel"), None)
        title = child_text(channel, "title") if channel is not None else ""
        version = root.attrib.get("version", "2.0")
        feed_type = RSS_0X if version in {"0.91", "0.92", "0.93"} else RSS_2X
        return FeedInfo(source_url, feed_type, title or feed_title_from_url(source_url))

    if root_name == "rdf":
        channel = next((child for child in root if local_name(child) == "channel"), None)
        title = child_text(channel, "title") if channel is not None else ""
        return FeedInfo(source_url, RDF, title or feed_title_from_url(source_url))

    if root_name == "feed":
        return FeedInfo(source_url, ATOM_10, child_text(root, "title") or feed_title_from_url(source_url))

    if root_name == "urlset":
        return FeedInfo(source_url, SITEMAP, feed_title_from_url(source_url))

    return None


def collect_feeds() -> list[FeedInfo]:
    unique_urls: dict[str, None] = {}
    errors: list[str] = []

    print(f"Downloading {len(CATALOGUE_URLS)} curated OPML catalogues...", flush=True)

    for catalogue_index, catalogue_url in enumerate(CATALOGUE_URLS, start=1):
        print(f"  Catalogue {catalogue_index}/{len(CATALOGUE_URLS)}", flush=True)

        try:
            for feed_url in download_opml(catalogue_url):
                unique_urls.setdefault(feed_url, None)
        except (OSError, element_tree.ParseError) as error:
            errors.append(f"{catalogue_url}: {error}")

    candidate_urls = list(unique_urls)[:MAXIMUM_FEED_CANDIDATES]
    feeds: list[FeedInfo] = []

    print(
        f"Found {len(unique_urls)} unique URLs. Inspecting up to {len(candidate_urls)} feeds "
        f"with {FEED_DOWNLOAD_WORKERS} concurrent downloads...",
        flush=True,
    )

    with ThreadPoolExecutor(max_workers=FEED_DOWNLOAD_WORKERS) as executor:
        for batch_start in range(0, len(candidate_urls), FEED_CLASSIFICATION_BATCH_SIZE):
            batch = candidate_urls[batch_start : batch_start + FEED_CLASSIFICATION_BATCH_SIZE]
            batch_end = batch_start + len(batch)
            print(
                f"  Inspecting candidates {batch_start + 1}-{batch_end}/{len(candidate_urls)} "
                f"({len(feeds)}/{FEED_COUNT} recognized)",
                flush=True,
            )

            for feed in executor.map(classify_feed, batch):
                if feed is not None:
                    feeds.append(feed)

            if len(feeds) >= FEED_COUNT:
                print(f"Selected {FEED_COUNT} classified feeds.", flush=True)
                return feeds[:FEED_COUNT]

    if len(feeds) < FEED_COUNT:
        details = "\n".join(errors) if errors else "No usable feed URLs were found."
        raise RuntimeError(
            f"Only identified {len(feeds)} usable feeds after inspecting {len(candidate_urls)} candidates; "
            f"{FEED_COUNT} are required.\n{details}"
        )


def ensure_empty_database(connection: sqlite3.Connection) -> None:
    tables = connection.execute(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%'"
    ).fetchall()

    if tables:
        raise RuntimeError("The target database is not empty. Use a new empty SQLite database file.")


def category_rows(now: int) -> Iterable[tuple[int, int, int, str, str, int, str]]:
    for category_id in range(1, CATEGORY_COUNT + 1):
        if category_id <= 10:
            parent_id = -1
        elif category_id <= 20:
            parent_id = category_id - 10
        else:
            parent_id = category_id - 10

        yield (
            category_id,
            category_id - 1,
            parent_id,
            f"Load test folder {category_id:02d}",
            f"Generated folder {category_id} for article-list and feed-tree performance tests.",
            now - category_id * 86_400,
            f"load-test-category-{category_id}",
        )


def feed_type_name(feed_type: int) -> str:
    return {
        RSS_0X: "RSS 0.x",
        RSS_2X: "RSS 2.x",
        RDF: "RDF/RSS 1.0",
        ATOM_10: "Atom 1.0",
        JSON_FEED: "JSON Feed",
        SITEMAP: "sitemap",
        ICALENDAR: "iCalendar",
    }[feed_type]


def feed_rows(feeds: list[FeedInfo], now: int) -> Iterable[tuple[object, ...]]:
    for feed_id, feed in enumerate(feeds, start=1):
        category_id = -1 if feed_id % 11 == 0 else ((feed_id - 1) % CATEGORY_COUNT) + 1
        topic = TOPICS[(feed_id - 1) % len(TOPICS)]

        yield (
            feed_id,
            feed_id - 1,
            f"{feed.title[:360]} ({feed_id:03d})",
            f"Curated {feed_type_name(feed.feed_type)} source used by the RSS Guard large-database load test ({topic}).",
            now - feed_id * 3_600,
            generated_icon(feed_id),
            category_id,
            feed.source_url,
            0,  # Feed::AutoUpdateType::DontAutoUpdate.
            900,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            1,
            1,
            0,
            ACCOUNT_ID,
            f"load-test-feed-{feed_id}",
            json.dumps({"source_type": 0, "type": feed.feed_type, "encoding": "UTF-8"}),
            0,
        )


def label_rows() -> Iterable[tuple[int, str, bytes, str, int]]:
    for label_id in range(1, LABEL_COUNT + 1):
        yield (
            label_id,
            f"Load test label {label_id:02d}",
            generated_icon(FEED_COUNT + label_id),
            f"load-test-label-{label_id}",
            ACCOUNT_ID,
        )


def filter_rows() -> Iterable[tuple[int, str, str, int, int]]:
    for filter_id, (name, script) in enumerate(FILTERS, start=1):
        yield filter_id, name, script, 1, filter_id - 1


def filter_assignment_rows() -> Iterable[tuple[int, int]]:
    random_generator = random.Random(FILTER_ASSIGNMENT_SEED)

    for filter_id in range(1, len(FILTERS) + 1):
        feed_ids = random_generator.sample(
            range(1, FEED_COUNT + 1), FILTER_FEED_ASSIGNMENT_COUNT
        )

        for feed_id in sorted(feed_ids):
            yield filter_id, feed_id


def article_contents(article_id: int, topic: str) -> str:
    if article_id % 3 == 0:
        return (
            f"Load test report {article_id} covers {topic}. It contains enough natural language text "
            "to exercise sorting, filtering, searching, previews and article extraction without relying "
            "on a live web page."
        )

    image_url = f"https://picsum.photos/seed/rssguard-load-test-{article_id}/640/360"
    return (
        "<article>"
        f"<h2>{topic.title()} field notes {article_id}</h2>"
        f"<p>This generated article discusses {topic}, practical tools and recent developments. "
        "Its intentionally varied text is useful for testing searches, filters and rendering.</p>"
        f"<figure><img src=\"{image_url}\" alt=\"Illustration for {topic}\"></figure>"
        "<p>Teams can use this entry to verify article previews, image handling, labels, read states "
        "and lazy loading with a realistically sized database.</p>"
        "</article>"
    )


def article_rows(now: int) -> Iterable[tuple[object, ...]]:
    for article_id in range(1, ARTICLE_COUNT + 1):
        feed_id = ((article_id - 1) % FEED_COUNT) + 1
        topic = TOPICS[(feed_id - 1) % len(TOPICS)]
        author = AUTHORS[(article_id - 1) % len(AUTHORS)]
        created = now - (ARTICLE_COUNT - article_id) * 300

        yield (
            article_id,
            0 if article_id % 7 == 0 else 1,
            1 if article_id % 31 == 0 else 0,
            1 if article_id % 97 == 0 else 0,
            0,
            feed_id,
            f"{topic.title()} update {article_id:06d}: practical notes and analysis",
            f"https://example.invalid/rssguard-load-test/{feed_id}/{article_id}",
            author,
            created,
            article_contents(article_id, topic),
            None,
            float((article_id * 13) % 101),
            ACCOUNT_ID,
            f"load-test-message-{article_id}",
            json.dumps({"generated": True, "topic": topic, "sequence": article_id}),
            created + 60,
        )


def insert_in_batches(connection: sqlite3.Connection, statement: str, rows: Iterable[tuple[object, ...]]) -> None:
    batch: list[tuple[object, ...]] = []

    for row in rows:
        batch.append(row)

        if len(batch) == ARTICLE_BATCH_SIZE:
            connection.executemany(statement, batch)
            batch.clear()

    if batch:
        connection.executemany(statement, batch)


def create_database(database_path: Path, feeds: list[FeedInfo], include_articles: bool) -> None:
    now = int(time.time())
    connection = sqlite3.connect(database_path)

    try:
        ensure_empty_database(connection)
        connection.executescript(sqlite_schema())
        connection.execute("PRAGMA foreign_keys=OFF")
        connection.execute("PRAGMA journal_mode=MEMORY")
        connection.execute("PRAGMA synchronous=OFF")
        connection.execute("PRAGMA temp_store=MEMORY")
        connection.execute("PRAGMA cache_size=-200000")

        with connection:
            connection.execute(
                "INSERT INTO Information (inf_key, inf_value) VALUES ('schema_version', ?)", (SCHEMA_VERSION,)
            )
            connection.execute(
                "INSERT INTO Accounts (id, ordr, type, custom_data) VALUES (?, ?, ?, ?)",
                (
                    ACCOUNT_ID,
                    0,
                    "std-rss",
                    json.dumps(
                        {
                            "title": "Large SQLite load-test account",
                            "show_node_unread": True,
                            "show_node_important": True,
                            "show_node_labels": True,
                            "show_node_probes": True,
                        }
                    ),
                ),
            )
            connection.executemany(
                """
                INSERT INTO Categories
                (id, ordr, parent_id, title, description, date_created, account_id, custom_id)
                VALUES (?, ?, ?, ?, ?, ?, 1, ?)
                """,
                category_rows(now),
            )
            connection.executemany(
                """
                INSERT INTO Feeds
                (id, ordr, title, description, date_created, icon, category, source, update_type,
                 update_interval, is_off, is_quiet, is_rtl, add_any_datetime_articles,
                 datetime_to_avoid, keep_article_customize, keep_article_count,
                 keep_unread_articles, keep_starred_articles, recycle_articles, account_id,
                 custom_id, custom_data, open_articles)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                """,
                feed_rows(feeds, now),
            )
            connection.executemany(
                "INSERT INTO Labels (id, name, icon, custom_id, account_id) VALUES (?, ?, ?, ?, ?)",
                label_rows(),
            )
            connection.executemany(
                """
                INSERT INTO MessageFilters (id, name, script, is_enabled, ordr)
                VALUES (?, ?, ?, ?, ?)
                """,
                filter_rows(),
            )
            connection.executemany(
                "INSERT INTO MessageFiltersInFeeds (filter, feed) VALUES (?, ?)",
                filter_assignment_rows(),
            )
            if include_articles:
                insert_in_batches(
                    connection,
                    """
                    INSERT INTO Messages
                    (id, is_read, is_important, is_deleted, is_pdeleted, feed, title, url, author,
                     date_created, contents, enclosures, score, account_id, custom_id, custom_data,
                     date_retrieved)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
                    """,
                    article_rows(now),
                )
                connection.executemany(
                    "INSERT INTO LabelsInMessages (message, label) VALUES (?, ?)",
                    (
                        (1 + (position * ARTICLE_COUNT // LABELLED_ARTICLE_COUNT), (position % LABEL_COUNT) + 1)
                        for position in range(LABELLED_ARTICLE_COUNT)
                    ),
                )

        connection.execute("PRAGMA foreign_keys=ON")
        connection.execute("ANALYZE")
        connection.execute("PRAGMA journal_mode=WAL")
        connection.execute("PRAGMA synchronous=NORMAL")
        connection.commit()
    finally:
        connection.close()


def verify_database(database_path: Path, include_articles: bool) -> None:
    connection = sqlite3.connect(database_path)

    try:
        counts = {
            "accounts": connection.execute("SELECT COUNT(*) FROM Accounts").fetchone()[0],
            "folders": connection.execute("SELECT COUNT(*) FROM Categories").fetchone()[0],
            "feeds": connection.execute("SELECT COUNT(*) FROM Feeds").fetchone()[0],
            "articles": connection.execute("SELECT COUNT(*) FROM Messages").fetchone()[0],
            "labels": connection.execute("SELECT COUNT(*) FROM Labels").fetchone()[0],
            "filters": connection.execute("SELECT COUNT(*) FROM MessageFilters").fetchone()[0],
            "filter assignments": connection.execute(
                "SELECT COUNT(*) FROM MessageFiltersInFeeds"
            ).fetchone()[0],
            "labelled articles": connection.execute("SELECT COUNT(*) FROM LabelsInMessages").fetchone()[0],
        }
    finally:
        connection.close()

    expected = {
        "accounts": 1,
        "folders": CATEGORY_COUNT,
        "feeds": FEED_COUNT,
        "articles": ARTICLE_COUNT if include_articles else 0,
        "labels": LABEL_COUNT,
        "filters": FILTER_COUNT,
        "filter assignments": FILTER_COUNT * FILTER_FEED_ASSIGNMENT_COUNT,
        "labelled articles": LABELLED_ARTICLE_COUNT if include_articles else 0,
    }

    if counts != expected:
        raise RuntimeError(f"Database verification failed: expected {expected}, got {counts}")

    print("Created large RSS Guard SQLite test database:")
    for name, count in counts.items():
        print(f"  {name}: {count:,}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "database",
        type=Path,
        nargs="?",
        default=Path("database.db"),
        help="new or empty SQLite database file (default: ./database.db)",
    )
    parser.add_argument(
        "--no-articles",
        action="store_true",
        help="create only the account tree, feeds, labels and icons without articles",
    )
    arguments = parser.parse_args()
    database_path = arguments.database.resolve()

    database_path.parent.mkdir(parents=True, exist_ok=True)
    feeds = collect_feeds()

    try:
        create_database(database_path, feeds, not arguments.no_articles)
        verify_database(database_path, not arguments.no_articles)
    except Exception:
        if database_path.exists() and database_path.stat().st_size == 0:
            database_path.unlink()
        raise

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1)
