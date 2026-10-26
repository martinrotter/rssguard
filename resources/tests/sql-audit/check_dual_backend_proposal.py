#!/usr/bin/env python3
"""Independently check the review-only dual-backend schema proposal.

Default checks run entirely in memory on SQLite. --mariadb additionally creates
three uniquely named EMPTY integration schemas on the explicitly isolated local
MariaDB server and drops exactly those schemas in finally. It never opens or
modifies either huge fixture, applies a patch to production, or times SQL.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib
import json
import os
import re
import shutil
import sqlite3
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "benchmark-results/sql-audit-20261014"
INDEX = "idx_Messages_feed_display"


def quote(value: str) -> str:
    return "`" + value.replace("`", "``") + "`"


def patch_overlay(path: Path) -> dict[str, str]:
    """Read unified diff hunks without importing the proposal writer or applying it."""
    overlay = {}
    for block in path.read_text(encoding="utf-8").split("diff --git ")[1:]:
        lines = block.splitlines()
        header = re.fullmatch(r"a/(.+) b/(.+)", lines[0])
        if not header or header[1] != header[2]:
            raise RuntimeError("Unsupported patch file header")
        name = header[1]
        original_path = ROOT / name
        original = original_path.read_text(encoding="utf-8").splitlines() if original_path.exists() else []
        result = []
        cursor = 0
        i = 1
        while i < len(lines):
            hunk = re.match(r"@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@", lines[i])
            if not hunk:
                i += 1
                continue
            old_start = max(0, int(hunk[1]) - 1)
            if old_start < cursor:
                raise RuntimeError("Overlapping patch hunks")
            result.extend(original[cursor:old_start])
            cursor = old_start
            removed = added = 0
            i += 1
            while i < len(lines) and not lines[i].startswith("@@ "):
                record = lines[i]
                i += 1
                if record.startswith("\\ No newline"):
                    continue
                if not record or record[0] not in " +-":
                    raise RuntimeError("Unsupported patch hunk record")
                if record[0] in " -":
                    if cursor >= len(original) or original[cursor] != record[1:]:
                        raise RuntimeError("Review patch no longer matches application source: " + name)
                    cursor += 1
                    removed += 1
                if record[0] in " +":
                    result.append(record[1:])
                    added += 1
            if removed != int(hunk[2] or 1) or added != int(hunk[4] or 1):
                raise RuntimeError("Patch hunk line counts differ")
        result.extend(original[cursor:])
        overlay[name] = "\n".join(result) + "\n"
    if not overlay:
        raise RuntimeError("Empty review patch")
    return overlay


def source_text(name: str, overlay: dict) -> str:
    return overlay.get(name, (ROOT / name).read_text(encoding="utf-8") if (ROOT / name).exists() else "")


def app_statements(script: str, backend: str, overlay: dict, database: str = "") -> list[str]:
    """Mirror application include splitting and backend-specific placeholders."""
    result = []
    for chunk in re.split(r"-- !\r?\n", source_text("resources/sql/" + script, overlay)):
        if not chunk.strip():
            continue
        if chunk.startswith("!!"):
            included = source_text("resources/sql/" + chunk[3:].strip(), overlay)
            result.extend(part for part in re.split(r"-- !\r?\n", included) if part.strip())
        else:
            result.append(chunk)
    if any(part.startswith("!!") for part in result):
        raise RuntimeError("Nested includes do not match DatabaseDriver::prepareScript")
    replacements = {
        "##": quote(database),
        "$$": "INTEGER PRIMARY KEY" if backend == "sqlite" else "INTEGER AUTO_INCREMENT PRIMARY KEY",
        "^^": "BLOB" if backend == "sqlite" else "MEDIUMBLOB",
        "**": "TEXT" if backend == "sqlite" else "MEDIUMTEXT",
        "~~": "COLLATE NOCASE" if backend == "sqlite" else "",
        "\u00a7\u00a71": "PRAGMA foreign_keys=ON;" if backend == "sqlite" else "SET FOREIGN_KEY_CHECKS=1;",
        "\u00a7\u00a70": "PRAGMA foreign_keys=OFF;" if backend == "sqlite" else "SET FOREIGN_KEY_CHECKS=0;",
    }
    for placeholder, replacement in replacements.items():
        result = [part.replace(placeholder, replacement) for part in result]
    return result


def index_definition(sql: str) -> dict:
    pattern = (r"CREATE\s+INDEX(?:\s+IF\s+NOT\s+EXISTS)?\s+`?(\w+)`?\s+ON\s+`?(\w+)`?"
               r"\s*\(([^)]+)\)\s*(.*)")
    match = re.fullmatch(pattern, sql.strip().rstrip(";"), re.IGNORECASE | re.DOTALL)
    if not match or match[1].casefold() != INDEX.casefold() or match[2].casefold() != "messages":
        raise RuntimeError("Unexpected trial/proposal index DDL")
    columns = []
    for column in match[3].split(","):
        tokens = column.replace("`", "").strip().split()
        if len(tokens) not in (1, 2) or (len(tokens) == 2 and tokens[1].upper() not in ("ASC", "DESC")):
            raise RuntimeError("Unexpected index column or direction")
        columns.append((tokens[0].casefold(), tokens[1].upper() if len(tokens) == 2 else "ASC"))
    return {"columns": columns, "predicate": re.sub(r"\s+", "", match[4]).casefold()}


def proposed_index(backend: str, overlay: dict) -> dict:
    script = app_statements(f"db_update_{backend}_105_106.sql", backend, overlay)
    candidates = [index_definition(sql) for sql in script if re.search(r"CREATE\s+INDEX", sql, re.IGNORECASE)]
    if len(candidates) != 1:
        raise RuntimeError("Expected one measured index in each backend upgrade")
    return candidates[0]


def check_static(overlay: dict) -> dict:
    definitions = source_text("src/librssguard/definitions/definitions.h", overlay)
    version = re.search(r'#define\s+APP_DB_SCHEMA_VERSION\s+"(\d+)"', definitions).group(1)
    original = source_text("src/librssguard/definitions/definitions.h", {})
    old_version = re.search(r'#define\s+APP_DB_SCHEMA_VERSION\s+"(\d+)"', original).group(1)
    if (old_version, version) != ("105", "106"):
        raise RuntimeError("Expected the unchanged current 105 schema and proposed version 106")
    qrc = source_text("resources/sql.qrc", overlay)
    for backend in ("sqlite", "mysql"):
        name = f"resources/sql/db_update_{backend}_105_106.sql"
        if name not in overlay or f"<file>sql/db_update_{backend}_105_106.sql</file>" not in qrc:
            raise RuntimeError("Backend upgrade missing from review patch or Qt SQL resources")
    return {"schema_version": version, "both_upgrade_resources_registered": True,
            "production_patch_applied": False}


def sqlite_schema(connection) -> list:
    return [(kind, name, table, re.sub(r"\s+", " ", sql or "").strip()) for kind, name, table, sql in
            connection.execute("SELECT type,name,tbl_name,sql FROM sqlite_master ORDER BY type,name")]


def sqlite_physical(connection) -> dict:
    ddl = connection.execute("SELECT sql FROM sqlite_master WHERE name=?", (INDEX,)).fetchone()[0]
    rows = connection.execute("PRAGMA index_xinfo(" + quote(INDEX) + ")").fetchall()
    return {"columns": [(row[2].casefold(), "DESC" if row[3] else "ASC", row[4]) for row in rows if row[5]],
            "predicate": index_definition(ddl)["predicate"]}


def check_sqlite(overlay: dict, trial: dict) -> dict:
    expected = index_definition(trial["index_sql"])
    if expected != proposed_index("sqlite", overlay):
        raise RuntimeError("SQLite upgrade differs from the exact measured partial index")
    reference, fresh, upgraded = (sqlite3.connect(":memory:") for _ in range(3))
    try:
        for connection in (reference, upgraded):
            for statement in app_statements("db_init_sqlite.sql", "sqlite", {}):
                connection.execute(statement)
        reference.execute(trial["index_sql"])
        for statement in app_statements("db_init_sqlite.sql", "sqlite", overlay):
            fresh.execute(statement)
        fresh.execute("INSERT INTO Information VALUES ('schema_version','106')")
        upgraded.execute("INSERT INTO Information VALUES ('schema_version','105')")
        upgrade = app_statements("db_update_sqlite_105_106.sql", "sqlite", overlay)
        for statement in upgrade:
            upgraded.execute(statement)
        before_retry = sqlite_schema(upgraded)
        # Simulate interruption after DDL but before the app stores schema version.
        for statement in upgrade:
            upgraded.execute(statement)
        retry_unchanged = before_retry == sqlite_schema(upgraded)
        upgraded.execute("UPDATE Information SET inf_value='106' WHERE inf_key='schema_version'")
        physical = [sqlite_physical(connection) for connection in (reference, fresh, upgraded)]
        result = {"status": "complete", "new_and_upgraded_schema_identical": sqlite_schema(fresh) == sqlite_schema(upgraded),
                  "retry_does_not_change_schema": retry_unchanged,
                  "physical_index_matches_measured": physical[0] == physical[1] == physical[2],
                  "physical_index": physical[1], "sqlite_version": sqlite3.sqlite_version,
                  "scope": "Three in-memory SQLite databases; no huge fixture opened"}
        if not all(result[key] for key in ("new_and_upgraded_schema_identical", "retry_does_not_change_schema",
                                           "physical_index_matches_measured")):
            raise RuntimeError("SQLite new/upgrade/retry integration check failed")
        return result
    finally:
        for connection in (reference, fresh, upgraded):
            connection.close()


def maria_schema(cursor, database: str) -> list:
    cursor.execute("SELECT TABLE_NAME FROM information_schema.tables WHERE TABLE_SCHEMA=%s ORDER BY TABLE_NAME", (database,))
    result = []
    for (table,) in cursor.fetchall():
        cursor.execute("SHOW CREATE TABLE " + quote(database) + "." + quote(table))
        ddl = cursor.fetchone()[1]
        result.append((table, re.sub(r"\s+", " ", ddl).strip()))
    return result


def maria_physical(cursor, database: str) -> dict:
    cursor.execute("SELECT COLUMN_NAME,COLLATION,SUB_PART,INDEX_TYPE FROM information_schema.statistics "
                   "WHERE TABLE_SCHEMA=%s AND LOWER(TABLE_NAME)='messages' AND LOWER(INDEX_NAME)=LOWER(%s) "
                   "ORDER BY SEQ_IN_INDEX", (database, INDEX))
    rows = cursor.fetchall()
    cursor.execute("SELECT COLLATION_NAME FROM information_schema.columns "
                   "WHERE TABLE_SCHEMA=%s AND LOWER(TABLE_NAME)='messages' AND COLUMN_NAME='title'", (database,))
    return {"columns": [(column.casefold(), "DESC" if direction == "D" else "ASC", prefix, kind)
                        for column, direction, prefix, kind in rows], "title_collation": cursor.fetchone()[0]}


def proposed_maria_metadata(overlay: dict) -> tuple[str, str]:
    """Extract adjacent C++ string literals from the exact review-only helper."""
    header = source_text("src/librssguard/database/feeddisplayindex.h", overlay)
    query_body = re.search(r"inline QString metadataSql\(\)\s*\{(.*?)\n\}", header, re.DOTALL)
    signature_body = re.search(r"signature\s*==\s*QStringLiteral\((.*?)\);", header, re.DOTALL)
    if not query_body or not signature_body:
        raise RuntimeError("Cannot extract exact proposed metadata SQL/guard signature")

    def literals(body):
        return "".join(json.loads('"' + token + '"') for token in re.findall(r'"((?:\\.|[^"\\])*)"', body))

    return literals(query_body[1]), literals(signature_body[1])


def check_mariadb(arguments, overlay: dict, trial: dict) -> dict:
    expected = index_definition(trial["index_sql"])
    if expected != proposed_index("mysql", overlay) or expected["predicate"]:
        raise RuntimeError("MariaDB upgrade differs from the exact measured full index")
    pymysql = importlib.import_module("pymysql")
    data_dir = arguments.server_data_dir.resolve(strict=True)
    if shutil.disk_usage(data_dir).free < 1000000000:
        raise RuntimeError("Integration check requires a 1GB free disk reserve")
    connection = pymysql.connect(host="127.0.0.1", port=arguments.port, charset="utf8mb4", autocommit=True,
        user=os.environ.get("RSSGUARD_SQL_AUDIT_MARIADB_USER", "root"),
        password=os.environ.get("RSSGUARD_SQL_AUDIT_MARIADB_PASSWORD", ""), connect_timeout=10, read_timeout=120)
    token = uuid.uuid4().hex[:16]
    names = {kind: f"rssguard_sql_audit_integration_{token}_{kind}" for kind in ("reference", "fresh", "upgraded")}
    created = []
    result = {}
    try:
        with connection.cursor() as cursor:
            cursor.execute("SELECT @@datadir,VERSION(),@@lower_case_table_names,@@innodb_page_size,"
                           "@@innodb_default_row_format,@@default_storage_engine")
            actual_dir, version, lower_names, page_size, row_format, engine = cursor.fetchone()
            if Path(actual_dir).resolve() != data_dir or "mariadb" not in version.lower():
                raise RuntimeError("Not the explicitly isolated local MariaDB server")
            if page_size < 16384 or row_format.lower() != "dynamic" or engine.lower() != "innodb":
                raise RuntimeError("Measured index requires modern DYNAMIC/InnoDB >=16KB pages")
            for kind, database in names.items():
                cursor.execute("SELECT COUNT(*) FROM information_schema.schemata WHERE SCHEMA_NAME=%s", (database,))
                if cursor.fetchone()[0]:
                    raise RuntimeError("Generated disposable integration schema unexpectedly exists")
                cursor.execute("CREATE DATABASE " + quote(database) + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
                created.append(database)
                cursor.execute("USE " + quote(database))
                cursor.execute("SET NAMES 'utf8mb4'")
                cursor.execute("SET CHARACTER SET utf8mb4")
                cursor.execute("SET FOREIGN_KEY_CHECKS=1")
                for statement in app_statements("db_init_mysql.sql", "mysql", overlay if kind == "fresh" else {}, database):
                    cursor.execute(statement)
                cursor.execute("INSERT INTO Information VALUES ('schema_version',%s)", ("106" if kind == "fresh" else "105",))
                if kind == "reference":
                    cursor.execute(trial["index_sql"])
                elif kind == "upgraded":
                    upgrade = app_statements("db_update_mysql_105_106.sql", "mysql", overlay, database)
                    for statement in upgrade:
                        cursor.execute(statement)
                    before_retry = maria_schema(cursor, database)
                    for statement in upgrade:
                        cursor.execute(statement)
                    result["retry_does_not_change_schema"] = before_retry == maria_schema(cursor, database)
                    cursor.execute("UPDATE Information SET inf_value='106' WHERE inf_key='schema_version'")
            physical = {kind: maria_physical(cursor, database) for kind, database in names.items()}
            required_columns = [(column, direction, None, "BTREE") for column, direction in expected["columns"]]
            metadata_sql, expected_signature = proposed_maria_metadata(overlay)
            metadata_results = {}
            for kind in ("fresh", "upgraded"):
                cursor.execute("USE " + quote(names[kind]))
                cursor.execute(metadata_sql)
                observed_version, signature = cursor.fetchone()
                metadata_results[kind] = {"server_version": observed_version, "signature": signature,
                    "exact_signature_matches_helper": signature == expected_signature,
                    "usable_guard_result": "mariadb" in observed_version.casefold() and signature == expected_signature}
            # Exercise the real server's ignored-index capability/error fallback,
            # using only the empty disposable fresh schema, then restore its flag.
            cursor.execute("USE " + quote(names["fresh"]))
            cursor.execute("ALTER TABLE Messages ALTER INDEX " + quote(INDEX) + " IGNORED")
            try:
                cursor.execute(metadata_sql)
                ignored_version, ignored_signature = cursor.fetchone()
                ignored_usable = "mariadb" in ignored_version.casefold() and ignored_signature == expected_signature
                error_code = None
                try:
                    cursor.execute("SELECT id FROM Messages FORCE INDEX (" + quote(INDEX) + ") WHERE feed=1")
                    cursor.fetchall()
                except pymysql.err.OperationalError as error:
                    error_code = error.args[0]
                cursor.execute("SELECT id FROM Messages WHERE feed=1")
                fallback_rows = cursor.fetchall()
                ignored_control = {"schema": names["fresh"], "ignored_signature": ignored_signature,
                    "signature_differs_from_usable_index": ignored_signature != expected_signature,
                    "usable_guard_result": ignored_usable, "hinted_query_native_error_code": error_code,
                    "unhinted_fallback_succeeded": len(fallback_rows) == 0, "fallback_rows": len(fallback_rows)}
                if ignored_usable or ignored_signature == expected_signature or error_code != 1176 or fallback_rows:
                    raise RuntimeError("Real MariaDB ignored-index/error-1176 fallback control failed")
            finally:
                cursor.execute("ALTER TABLE Messages ALTER INDEX " + quote(INDEX) + " NOT IGNORED")
            result.update({"status": "complete", "server_version": version, "lower_case_table_names": lower_names,
                "innodb_page_size": page_size, "row_format": row_format, "storage_engine": engine,
                "new_and_upgraded_schema_identical": maria_schema(cursor, names["fresh"]) == maria_schema(cursor, names["upgraded"]),
                "physical_index_matches_measured": physical["reference"] == physical["fresh"] == physical["upgraded"],
                "physical_column_directions_match_trial": physical["fresh"]["columns"] == required_columns,
                "physical_index": physical["fresh"], "schemas_created": list(created),
                "proposed_metadata_query": metadata_sql, "expected_helper_signature": expected_signature,
                "metadata_guard_results": metadata_results,
                "ignored_index_negative_control": ignored_control,
                "proposed_metadata_guard_usable_on_fresh_and_upgraded": all(row["usable_guard_result"] for row in metadata_results.values()),
                "scope": "Empty uniquely named integration schemas; huge fixture not opened"})
            if not all(result[key] for key in ("retry_does_not_change_schema", "new_and_upgraded_schema_identical",
                "physical_index_matches_measured", "physical_column_directions_match_trial",
                "proposed_metadata_guard_usable_on_fresh_and_upgraded")):
                raise RuntimeError("MariaDB new/upgrade/retry physical integration check failed")
    finally:
        with connection.cursor() as cursor:
            for database in reversed(created):
                if database not in names.values() or not re.fullmatch(r"rssguard_sql_audit_integration_[a-f0-9]{16}_(reference|fresh|upgraded)", database):
                    raise RuntimeError("Refusing cleanup outside exact generated disposable schemas")
                cursor.execute("DROP DATABASE " + quote(database))
            for database in created:
                cursor.execute("SELECT COUNT(*) FROM information_schema.schemata WHERE SCHEMA_NAME=%s", (database,))
                if cursor.fetchone()[0]:
                    raise RuntimeError("Disposable integration schema cleanup failed")
        connection.close()
    result["all_disposable_schemas_removed"] = True
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--patch", type=Path, default=OUT / "dual-backend-proposal.patch")
    parser.add_argument("--sqlite-trial", type=Path, default=OUT / "index-trial-literal-results.json")
    parser.add_argument("--maria-trial", type=Path)
    parser.add_argument("--mariadb", action="store_true", help="run only after benchmark timings finish")
    parser.add_argument("--port", type=int, default=33316)
    parser.add_argument("--server-data-dir", type=Path, default=OUT / "maria/data")
    parser.add_argument("--output", type=Path, default=OUT / "dual-backend-proposal-integration-checks.json")
    arguments = parser.parse_args()
    overlay = patch_overlay(arguments.patch)
    result = {"patch_sha256": hashlib.sha256(arguments.patch.read_bytes()).hexdigest(),
              "checker_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "static": check_static(overlay),
              "sqlite": check_sqlite(overlay, json.loads(arguments.sqlite_trial.read_text(encoding="utf-8")))}
    if arguments.mariadb:
        if not arguments.maria_trial:
            parser.error("--mariadb requires the completed measured --maria-trial JSON")
        result["mariadb"] = check_mariadb(arguments, overlay, json.loads(arguments.maria_trial.read_text(encoding="utf-8")))
    else:
        result["mariadb"] = {"status": "pending_after_benchmark_cleanup"}
    arguments.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
