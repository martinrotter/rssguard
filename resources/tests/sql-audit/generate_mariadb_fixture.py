#!/usr/bin/env python3
"""Import the huge synthetic SQLite fixture into a NEW isolated MariaDB database.

Install PyMySQL separately before running. The script uses the application's
canonical MySQL initialization SQL and imports every column of every table with
foreign-key checks enabled. It never modifies the source SQLite database, drops
an existing target, changes application indexes, or benchmarks application SQL.

Credentials come from environment variables rather than command-line arguments.
Only a loopback server with an explicitly supplied local data directory is accepted.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import importlib
import json
import os
import re
import shutil
import sqlite3
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TABLE_ORDER = ("Information", "Accounts", "Categories", "Feeds", "MessageFilters",
               "Labels", "Probes", "MessageFiltersInFeeds", "Messages", "LabelsInMessages")
COMPOSITE_ORDER = {"Information": ("inf_key",),
                   "MessageFiltersInFeeds": ("filter", "feed"),
                   "LabelsInMessages": ("message", "label")}


def identifier(value: str) -> str:
    return "`" + value.replace("`", "``") + "`"


def canonical_ddl(database: str) -> tuple[list[str], str, dict[str, str]]:
    """Match DatabaseDriver::prepareScript + MariaDbDriver placeholder methods."""
    sql_dir = ROOT / "resources/sql"
    definitions = (ROOT / "src/librssguard/definitions/definitions.h").read_text(encoding="utf-8")
    version = re.search(r'#define\s+APP_DB_SCHEMA_VERSION\s+"(\d+)"', definitions).group(1)
    hashes = {}

    def expand(name: str) -> list[str]:
        source = (sql_dir / name).read_text(encoding="utf-8")
        hashes[name] = hashlib.sha256(source.encode("utf-8")).hexdigest()
        result = []
        for statement in re.split(r"-- !\r?\n", source):
            if not statement.strip():
                continue
            if statement.startswith("!!"):
                result.extend(expand(statement[3:].strip()))
            else:
                result.append(statement.strip())
        return result

    statements = expand("db_init_mysql.sql")
    replacements = {"##": identifier(database), "$$": "INTEGER AUTO_INCREMENT PRIMARY KEY",
                    "^^": "MEDIUMBLOB", "**": "MEDIUMTEXT", "~~": "",
                    "\u00a7\u00a71": "SET FOREIGN_KEY_CHECKS=1;",
                    "\u00a7\u00a70": "SET FOREIGN_KEY_CHECKS=0;"}
    for placeholder, replacement in replacements.items():
        statements = [statement.replace(placeholder, replacement) for statement in statements]
    return statements, version, hashes


def digest_rows(digest, rows) -> None:
    def encode(value):
        if isinstance(value, (bytes, bytearray, memoryview)):
            return {"bytes_base64": base64.b64encode(bytes(value)).decode("ascii")}
        return value
    for row in rows:
        # Preserve scalar types, NULLs and exact text/binary contents across engines.
        encoded = json.dumps([encode(value) for value in row], ensure_ascii=False,
                             separators=(",", ":"), allow_nan=False).encode("utf-8")
        digest.update(encoded)
        digest.update(b"\n")


def guard_disk(data_dir: Path, reserve: int) -> int:
    free = shutil.disk_usage(data_dir).free
    if free < reserve:
        raise RuntimeError(f"Import stopped to preserve disk reserve: free={free}, required={reserve}")
    return free


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def validate_native_schema(connection, source, metadata: dict, database: str,
                           statements: list[str], version: str) -> None:
    """Read-only backend-native schema, FK and integrity checks.

    Windows MariaDB commonly stores table names in lowercase. The application SQL
    retains mixed-case spelling, so comparison must follow lower_case_table_names.
    Column and index identifiers are case-insensitive on MariaDB independently.
    """
    with connection.cursor() as cursor:
        cursor.execute("SELECT @@lower_case_table_names")
        lower_case_table_names = cursor.fetchone()[0]
        fold_table = (lambda value: value.casefold()) if lower_case_table_names in (1, 2) else (lambda value: value)
        metadata["lower_case_table_names"] = lower_case_table_names
        cursor.execute("SELECT TABLE_NAME,ENGINE,ROW_FORMAT,DATA_LENGTH,INDEX_LENGTH "
                       "FROM information_schema.tables WHERE table_schema=%s ORDER BY TABLE_NAME", (database,))
        tables = cursor.fetchall()
        if {fold_table(row[0]) for row in tables} != {fold_table(table) for table in TABLE_ORDER}:
            raise RuntimeError("Destination table set differs from the canonical application schema")
        if any(row[1].lower() != "innodb" or row[2].lower() != "dynamic" for row in tables):
            raise RuntimeError("Destination native engine/row formats differ from the validated server defaults")
        metadata["native_table_statistics"] = [list(row) for row in tables]
        native_names = {fold_table(row[0]): row[0] for row in tables}
        metadata["native_table_names"] = {table: native_names[fold_table(table)] for table in TABLE_ORDER}
        metadata["show_create_tables"] = {}
        for table in TABLE_ORDER:
            source_columns = [row[1] for row in source.execute("PRAGMA table_info(" + identifier(table) + ")")]
            cursor.execute("SELECT COLUMN_NAME FROM information_schema.columns "
                           "WHERE table_schema=%s AND table_name=%s ORDER BY ORDINAL_POSITION", (database, table))
            if [row[0].casefold() for row in cursor.fetchall()] != [column.casefold() for column in source_columns]:
                raise RuntimeError("Destination column names/order differ from canonical source: " + table)
            cursor.execute("SELECT COUNT(*) FROM " + identifier(table))
            if cursor.fetchone()[0] != metadata["counts"][table]:
                raise RuntimeError("Destination row count differs: " + table)
            cursor.execute("SHOW CREATE TABLE " + identifier(table))
            metadata["show_create_tables"][table] = cursor.fetchone()[1]
        expected_indexes = re.findall(r"CREATE INDEX\s+(\w+)\s+ON\s+(\w+)\s*\(([^)]+)\)",
                                      "\n".join(statements), re.IGNORECASE)
        cursor.execute("SELECT TABLE_NAME,INDEX_NAME,COLUMN_NAME,SEQ_IN_INDEX,SUB_PART "
                       "FROM information_schema.statistics WHERE table_schema=%s "
                       "ORDER BY TABLE_NAME,INDEX_NAME,SEQ_IN_INDEX", (database,))
        native_index_rows = cursor.fetchall()
        native_indexes = {}
        for table, index, column, sequence, prefix in native_index_rows:
            native_indexes.setdefault((fold_table(table), index.casefold()), []).append((column.casefold(), prefix))
        for index, table, columns in expected_indexes:
            expected_columns = [(column.strip().casefold(), None) for column in columns.split(",")]
            if native_indexes.get((fold_table(table), index.casefold())) != expected_columns:
                raise RuntimeError("Canonical application index missing/different: " + index)
        metadata["canonical_named_indexes"] = [index for index, table, columns in expected_indexes]
        metadata["native_index_names"] = sorted({row[1] for row in native_index_rows})
        metadata["native_index_columns"] = [list(row) for row in native_index_rows]
        expected_fks = []
        metadata["foreign_key_orphans"] = []
        for table in TABLE_ORDER:
            for fk in source.execute("PRAGMA foreign_key_list(" + identifier(table) + ")"):
                parent, child_column, parent_column = fk[2:5]
                expected_fks.append((fold_table(table), child_column.casefold(), fold_table(parent), parent_column.casefold()))
                cursor.execute("SELECT COUNT(*) FROM " + identifier(table) + " c LEFT JOIN " + identifier(parent) +
                    " p ON c." + identifier(child_column) + "=p." + identifier(parent_column) +
                    " WHERE p." + identifier(parent_column) + " IS NULL")
                orphan_count = cursor.fetchone()[0]
                if orphan_count:
                    metadata["foreign_key_orphans"].append([table, child_column, parent, orphan_count])
        if metadata["foreign_key_orphans"]:
            raise RuntimeError("Foreign-key orphan rows found in destination")
        cursor.execute("SELECT TABLE_NAME,COLUMN_NAME,REFERENCED_TABLE_NAME,REFERENCED_COLUMN_NAME "
                       "FROM information_schema.key_column_usage WHERE table_schema=%s "
                       "AND REFERENCED_TABLE_NAME IS NOT NULL", (database,))
        actual_fks = [(fold_table(table), child.casefold(), fold_table(parent), referenced.casefold())
                      for table, child, parent, referenced in cursor.fetchall()]
        if set(actual_fks) != set(expected_fks):
            raise RuntimeError("Destination FK relationships differ from canonical source")
        metadata["native_foreign_keys"] = actual_fks
        cursor.execute("SELECT inf_value FROM Information WHERE inf_key='schema_version'")
        if cursor.fetchone() != (version,):
            raise RuntimeError("Destination schema version differs from the application")
        cursor.execute("CHECK TABLE " + ",".join(map(identifier, TABLE_ORDER)))
        checks = cursor.fetchall()
        metadata["check_table"] = [list(row) for row in checks]
        if any(row[2].lower() == "error" for row in checks):
            raise RuntimeError("MariaDB CHECK TABLE reported corruption")
        checked_ok = {fold_table(row[0].rsplit(".", 1)[-1]) for row in checks
                      if row[2].lower() == "status" and row[3].upper() == "OK"}
        if checked_ok != {fold_table(table) for table in TABLE_ORDER}:
            raise RuntimeError("MariaDB CHECK TABLE did not confirm all ten tables are OK")


def validate_existing(arguments) -> dict:
    """Resume only native validation after all copied rows were already hashed.

    Requires the complete, matching row-hash evidence from the prior import.
    Does not execute CREATE, ALTER, INSERT, UPDATE, DELETE, DROP or ANALYZE.
    """
    pymysql = importlib.import_module("pymysql")
    source_path = arguments.source.resolve(strict=True)
    data_dir = arguments.server_data_dir.resolve(strict=True)
    guard_disk(data_dir, arguments.reserve_bytes)
    metadata = json.loads(arguments.metadata.read_text(encoding="utf-8"))
    if metadata.get("database") != arguments.database or Path(metadata.get("source_sqlite", "")).resolve() != source_path:
        raise RuntimeError("Prior import metadata identifies a different database/source")
    for key in ("counts", "source_row_sha256", "mariadb_row_sha256"):
        if set(metadata.get(key, {})) != set(TABLE_ORDER):
            raise RuntimeError("Prior import lacks complete all-table count/hash evidence")
    if metadata["source_row_sha256"] != metadata["mariadb_row_sha256"]:
        raise RuntimeError("Prior import row hashes did not match; validation-only recovery is inappropriate")
    statements, version, source_hashes = canonical_ddl(arguments.database)
    if metadata.get("canonical_ddl_sources_sha256") != source_hashes or metadata.get("schema_version") != version:
        raise RuntimeError("Canonical application SQL changed since the completed import")
    source = sqlite3.connect(source_path.as_uri() + "?mode=ro", uri=True)
    source.execute("PRAGMA query_only=ON")
    source.execute("BEGIN")
    connection = None
    started = time.monotonic()
    metadata["prior_import_attempt_elapsed_seconds"] = metadata.get("elapsed_seconds")
    try:
        connection = pymysql.connect(host=arguments.host, port=arguments.port, database=arguments.database,
            user=os.environ.get(arguments.user_env, "root"), password=os.environ.get(arguments.password_env, ""),
            charset="utf8mb4", autocommit=False, connect_timeout=10, read_timeout=300, write_timeout=300)
        with connection.cursor() as cursor:
            cursor.execute("SELECT @@datadir,@@foreign_key_checks,@@check_constraint_checks,@@collation_database,VERSION()")
            actual_dir, fk_checks, check_constraints, collation, server_version = cursor.fetchone()
            if Path(actual_dir).resolve() != data_dir or fk_checks != 1 or check_constraints != 1:
                raise RuntimeError("Existing isolated server directory or constraint settings differ")
            if collation != "utf8mb4_unicode_ci" or server_version != metadata.get("server_version"):
                raise RuntimeError("Existing server version/database collation differ from the completed import")
        for table in TABLE_ORDER:
            if source.execute("SELECT COUNT(*) FROM " + identifier(table)).fetchone()[0] != metadata["counts"][table]:
                raise RuntimeError("Source row counts changed since completed import: " + table)
        validate_native_schema(connection, source, metadata, arguments.database, statements, version)
        metadata.update({"status": "complete", "all_logical_rows_match_sqlite": True,
            "free_disk_bytes": guard_disk(data_dir, arguments.reserve_bytes),
            "validation_recovery_seconds": round(time.monotonic() - started, 2),
            "validation_recovery_mode": "read-only native checks; preserves completed exact all-row hash evidence",
            "fixture_importer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "limitations": ["Logical values match the aged SQLite fixture; physical InnoDB aging is not reproduced.",
                "Database and column collation are utf8mb4_unicode_ci, as created by the application.",
                "No extra indexes, ANALYZE TABLE, OPTIMIZE TABLE or application SQL benchmarks were applied.",
                "The PyMySQL driver differs from the application's Qt QMYSQL driver."]})
        metadata.pop("error_type", None)
        write_json(arguments.metadata, metadata)
        write_json(arguments.metadata.with_name("mariadb_fixture_progress.json"),
                   {"status": "complete", "counts": metadata["counts"]})
        print(json.dumps({"status": "complete", "database": arguments.database, "counts": metadata["counts"],
                          "lower_case_table_names": metadata["lower_case_table_names"],
                          "validation_recovery_seconds": metadata["validation_recovery_seconds"]}), flush=True)
        return metadata
    finally:
        source.close()
        if connection is not None:
            connection.close()


def import_fixture(arguments) -> dict:
    # Import is deferred until execution; --help and source inspection need no dependency/server.
    try:
        pymysql = importlib.import_module("pymysql")
    except ImportError as error:
        raise RuntimeError("PyMySQL is required; install it in the selected Python runtime first") from error
    source_path = arguments.source.resolve(strict=True)
    data_dir = arguments.server_data_dir.resolve(strict=True)
    if not data_dir.is_dir():
        raise RuntimeError("Expected an existing isolated MariaDB data directory")
    source_size = source_path.stat().st_size
    initial_free = guard_disk(data_dir, arguments.reserve_bytes)
    # InnoDB page/secondary-index/redo/undo overhead differs from SQLite. This is
    # an estimate, not a promise; the runtime reserve guard remains authoritative.
    estimated_target_bytes = int(source_size * arguments.innodb_size_factor)
    if initial_free < estimated_target_bytes + arguments.reserve_bytes:
        raise RuntimeError("Insufficient estimated disk space for import: "
                           f"free={initial_free}, estimated_target={estimated_target_bytes}, "
                           f"reserve={arguments.reserve_bytes}")
    statements, version, source_hashes = canonical_ddl(arguments.database)
    source = sqlite3.connect(source_path.as_uri() + "?mode=ro", uri=True)
    source.execute("PRAGMA query_only=ON")
    source.execute("BEGIN")
    source_version = source.execute("SELECT inf_value FROM Information WHERE inf_key='schema_version'").fetchone()
    if source_version != (version,):
        source.close()
        raise RuntimeError("Synthetic SQLite source does not have the current application schema version")
    source_tables = {row[0] for row in source.execute(
        "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'")}
    if source_tables != set(TABLE_ORDER):
        source.close()
        raise RuntimeError("Source tables do not match the complete canonical application schema")
    credentials = {"user": os.environ.get(arguments.user_env, "root"),
                   "password": os.environ.get(arguments.password_env, "")}
    started = time.monotonic()
    progress_path = arguments.metadata.with_name("mariadb_fixture_progress.json")
    connection = None
    metadata = {"status": "initializing", "database": arguments.database,
                "source_sqlite": str(source_path), "source_sqlite_bytes": source_size,
                "schema_version": version, "canonical_ddl_sources_sha256": source_hashes,
                "fixture_importer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "initial_free_disk_bytes": initial_free, "estimated_target_bytes": estimated_target_bytes,
                "reserve_bytes": arguments.reserve_bytes, "batch_size": arguments.batch_size,
                "counts": {}, "source_row_sha256": {}, "mariadb_row_sha256": {}}
    try:
        connection = pymysql.connect(host=arguments.host, port=arguments.port,
            charset="utf8mb4", autocommit=False, connect_timeout=10, read_timeout=300,
            write_timeout=300, **credentials)
        with connection.cursor() as cursor:
            cursor.execute("SELECT @@datadir,@@default_storage_engine,@@innodb_default_row_format,"
                           "@@innodb_page_size,VERSION()")
            actual_dir, default_engine, row_format, page_size, server_version = cursor.fetchone()
            if Path(actual_dir).resolve() != data_dir:
                raise RuntimeError("Connected server data directory differs from the explicit isolated directory")
            if default_engine.lower() != "innodb":
                raise RuntimeError("This fixture expects the isolated server's default engine to be InnoDB")
            if row_format.lower() != "dynamic":
                raise RuntimeError("Canonical utf8mb4 unique keys require a modern InnoDB DYNAMIC default row format")
            if page_size < 16384:
                raise RuntimeError("The canonical utf8mb4 VARCHAR(400) unique key needs an InnoDB page size >=16KB")
            if "mariadb" not in server_version.lower():
                raise RuntimeError("The requested cross-engine comparison requires MariaDB")
            cursor.execute("SELECT COUNT(*) FROM information_schema.schemata WHERE schema_name=%s",
                           (arguments.database,))
            if cursor.fetchone()[0]:
                raise RuntimeError("Target database already exists; choose a new isolated database name")
            cursor.execute("CREATE DATABASE " + identifier(arguments.database) +
                           " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci")
            cursor.execute("USE " + identifier(arguments.database))
            # This matches MariaDbDriver::setPragmas. Keep FK checks ON throughout.
            cursor.execute("SET NAMES 'utf8mb4'")
            cursor.execute("SET CHARACTER SET utf8mb4")
            cursor.execute("SET FOREIGN_KEY_CHECKS=1")
            for statement in statements:
                cursor.execute(statement)
            cursor.execute("SELECT @@foreign_key_checks,@@character_set_client,@@character_set_connection,"
                           "@@collation_database,@@sql_mode,@@innodb_page_size,@@innodb_buffer_pool_size,"
                           "@@innodb_flush_log_at_trx_commit,@@check_constraint_checks")
            settings = cursor.fetchone()
            if settings[0] != 1 or settings[-1] != 1:
                raise RuntimeError("Foreign-key and CHECK constraint enforcement must be enabled")
            if settings[1:4] != ("utf8mb4", "utf8mb4", "utf8mb4_unicode_ci"):
                raise RuntimeError("Destination charset/collation differs from application initialization")
            metadata.update({"server_version": server_version, "storage_engine": default_engine,
                "default_row_format": row_format, "session_settings": dict(zip(
                    ("foreign_key_checks", "character_set_client", "character_set_connection",
                     "collation_database", "sql_mode", "innodb_page_size", "innodb_buffer_pool_size",
                     "innodb_flush_log_at_trx_commit", "check_constraint_checks"), settings))})
        connection.commit()
        for table in TABLE_ORDER:
            columns = [row[1] for row in source.execute("PRAGMA table_info(" + identifier(table) + ")")]
            order = COMPOSITE_ORDER.get(table, ("id",))
            select_sql = "SELECT " + ",".join(map(identifier, columns)) + " FROM " + identifier(table)
            ordered_sql = select_sql + " ORDER BY " + ",".join(map(identifier, order))
            insert_sql = ("INSERT INTO " + identifier(table) + " (" + ",".join(map(identifier, columns)) +
                          ") VALUES (" + ",".join(["%s"] * len(columns)) + ")")
            source_cursor = source.execute(ordered_sql)
            digest = hashlib.sha256()
            imported = 0
            with connection.cursor() as cursor:
                while rows := source_cursor.fetchmany(arguments.batch_size):
                    cursor.executemany(insert_sql, rows)
                    connection.commit()
                    digest_rows(digest, rows)
                    imported += len(rows)
                    free = guard_disk(data_dir, arguments.reserve_bytes)
                    if imported % (arguments.batch_size * 100) == 0:
                        progress = {"status": "importing", "table": table, "rows": imported,
                                    "free_disk_bytes": free, "elapsed_seconds": round(time.monotonic() - started, 2)}
                        write_json(progress_path, progress)
                        print(json.dumps(progress), flush=True)
            metadata["counts"][table] = imported
            metadata["source_row_sha256"][table] = digest.hexdigest()
            print(json.dumps({"table": table, "imported_rows": imported}), flush=True)
        # Read every destination value back, using an unbuffered cursor to keep
        # memory bounded independently of table size; verify exact logical data.
        metadata["status"] = "validating"
        write_json(progress_path, {"status": "validating", "elapsed_seconds": round(time.monotonic() - started, 2)})
        for table in TABLE_ORDER:
            source_columns = [row[1] for row in source.execute("PRAGMA table_info(" + identifier(table) + ")")]
            with connection.cursor() as cursor:
                cursor.execute("SELECT COLUMN_NAME FROM information_schema.columns "
                               "WHERE table_schema=%s AND table_name=%s ORDER BY ORDINAL_POSITION",
                               (arguments.database, table))
                if [row[0] for row in cursor.fetchall()] != source_columns:
                    raise RuntimeError("Destination column names/order differ from canonical source: " + table)
                cursor.execute("SELECT COUNT(*) FROM " + identifier(table))
                if cursor.fetchone()[0] != metadata["counts"][table]:
                    raise RuntimeError("Destination row count differs: " + table)
            order = COMPOSITE_ORDER.get(table, ("id",))
            digest = hashlib.sha256()
            with connection.cursor(pymysql.cursors.SSCursor) as cursor:
                cursor.execute("SELECT " + ",".join(map(identifier, source_columns)) + " FROM " + identifier(table) +
                               " ORDER BY " + ",".join(map(identifier, order)))
                while rows := cursor.fetchmany(arguments.batch_size):
                    digest_rows(digest, rows)
            metadata["mariadb_row_sha256"][table] = digest.hexdigest()
            if digest.hexdigest() != metadata["source_row_sha256"][table]:
                raise RuntimeError("Destination logical rows differ from the SQLite source: " + table)
        validate_native_schema(connection, source, metadata, arguments.database, statements, version)
        metadata.update({"status": "complete", "all_logical_rows_match_sqlite": True,
            "free_disk_bytes": guard_disk(data_dir, arguments.reserve_bytes),
            "elapsed_seconds": round(time.monotonic() - started, 2),
            "limitations": ["Logical values match the aged SQLite fixture; physical InnoDB aging is not reproduced.",
                "Database and column collation are utf8mb4_unicode_ci, as created by the application.",
                "No extra indexes, ANALYZE TABLE, OPTIMIZE TABLE or application SQL benchmarks were applied.",
                "The PyMySQL driver differs from the application's Qt QMYSQL driver."]})
        write_json(arguments.metadata, metadata)
        write_json(progress_path, {"status": "complete", "counts": metadata["counts"]})
        print(json.dumps({"status": "complete", "database": arguments.database, "counts": metadata["counts"],
                          "elapsed_seconds": metadata["elapsed_seconds"]}), flush=True)
        return metadata
    except Exception as error:
        if connection is not None:
            connection.rollback()
        metadata.update({"status": "failed", "error_type": type(error).__name__,
                         "elapsed_seconds": round(time.monotonic() - started, 2)})
        write_json(arguments.metadata, metadata)
        # Preserve a partial fixture for inspection; no DROP DATABASE is issued.
        raise
    finally:
        source.close()
        if connection is not None:
            connection.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="existing synthetic huge.db, opened read-only")
    parser.add_argument("--database", required=True, help="new rssguard_sql_audit_* database")
    parser.add_argument("--server-data-dir", type=Path, required=True, help="explicit isolated server data directory")
    parser.add_argument("--host", choices=("127.0.0.1", "localhost", "::1"), default="127.0.0.1")
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--user-env", default="RSSGUARD_SQL_AUDIT_MARIADB_USER")
    parser.add_argument("--password-env", default="RSSGUARD_SQL_AUDIT_MARIADB_PASSWORD")
    parser.add_argument("--batch-size", type=int, default=1000)
    parser.add_argument("--reserve-bytes", type=int, default=1000000000)
    parser.add_argument("--innodb-size-factor", type=float, default=1.4,
                        help="conservative source-size multiplier for free-space preflight (default 1.4)")
    parser.add_argument("--metadata", type=Path,
                        default=ROOT / "benchmark-results/sql-audit-20261014/mariadb_fixture_metadata.json")
    parser.add_argument("--validate-existing", action="store_true",
                        help="resume read-only native validation using completed matching all-row hash evidence")
    arguments = parser.parse_args()
    if not re.fullmatch(r"rssguard_sql_audit_[a-z0-9_]+", arguments.database):
        parser.error("database must have the rssguard_sql_audit_ prefix and lowercase letters/digits/underscores")
    if not 1024 <= arguments.port <= 65535 or not 1 <= arguments.batch_size <= 10000:
        parser.error("port must be 1024..65535; batch size must be 1..10000")
    if arguments.reserve_bytes < 0 or arguments.innodb_size_factor < 1.0:
        parser.error("reserve must be nonnegative; InnoDB size factor must be >=1.0")
    if arguments.validate_existing:
        validate_existing(arguments)
    else:
        import_fixture(arguments)


if __name__ == "__main__":
    main()
