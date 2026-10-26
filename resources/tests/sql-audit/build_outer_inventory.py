"""Build the reviewed SQL inventory outside DatabaseQueries (no DB execution)."""
from pathlib import Path
import ast
import json
import re

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
inventory = []
sources = {
    p.relative_to(ROOT).as_posix(): p.read_text(encoding="utf-8-sig")
    for p in ROOT.joinpath("src").rglob("*")
    if p.suffix in {".cpp", ".h"} and "3rd-party" not in p.parts
}


def line_of(text, pos):
    return text.count("\n", 0, pos) + 1


def functions(text):
    # Out-of-class definitions begin at column zero; indented qualified calls
    # are expressions inside their enclosing function.
    return list(re.finditer(r"^(?=[^\s\n])[^\n;{}]*?\b(\w+::\w+)\s*\(", text, re.M))


def containing_function(text, pos):
    found = [m.group(1) for m in functions(text) if m.start() <= pos]
    return found[-1] if found else "<global>"


def callers(function):
    name = function.split("::")[-1]
    found = []
    for file, text in sources.items():
        definitions = functions(text)
        for m in re.finditer(r"\b" + re.escape(name) + r"\s*\(", text):
            line = line_of(text, m.start())
            if any(line == line_of(text, d.start()) for d in definitions):
                continue
            # Declarations and comments are not execution call sites.
            beginning = text.rfind("\n", 0, m.start()) + 1
            before = text[beginning:m.start()]
            if before.lstrip().startswith(("//", "*")):
                continue
            # Exclude occurrences within C++ string literals (such as SQL
            # SELECT version()) and qualified invocations of another class.
            if len(re.findall(r'(?<!\\)"', before)) % 2:
                continue
            qualifier = re.search(r"(\w+)::$", before)
            owner = function.split("::")[0]
            if qualifier and qualifier.group(1) != owner:
                continue
            enclosing = containing_function(text, m.start())
            if owner in {"QuiteRssImport", "RssGuard4Import"} and not enclosing.startswith(owner+"::"):
                continue
            if owner == "SqliteDriver" and file.endswith("mariadbdriver.cpp"):
                continue
            if owner == "MariaDbDriver" and file.endswith("sqlitedriver.cpp"):
                continue
            if name == "version" and owner in {"SqliteDriver", "MariaDbDriver"} and not before.endswith("driver()->"):
                continue
            if file.endswith(".h"):
                continue
            found.append({"file": file, "line": line, "function": enclosing})
    return found


def add(file, line, function, sql, details="", dialects=None, callsites=None, **extra):
    entry = dict(id=f"O{len(inventory)+1:03}", file=file, line=line, function=function,
                 sql_template=sql, dynamic_details=details,
                 dialects=dialects or ["sqlite", "mariadb"],
                 callers=callsites if callsites is not None else callers(function))
    entry.update(extra)
    inventory.append(entry)
    return entry


def balanced_argument(text, start):
    depth, in_string, escaped = 1, False, False
    for i in range(start, len(text)):
        c = text[i]
        if in_string:
            if escaped:
                escaped = False
            elif c == "\\":
                escaped = True
            elif c == '"':
                in_string = False
        elif c == '"':
            in_string = True
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return text[start:i]
    raise ValueError("Unbalanced C++ expression")


def first_literal(arg):
    m = re.search(r'(?:"(?:\\.|[^"\\])*"\s*)+', arg)
    return "".join(ast.literal_eval(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', m.group(0))) if m else None


# All literal execution/preparation sites in these files, independently found by
# a first-party scan for SqlQuery / QSqlQuery and SQL execution APIs.
literal_files = [
    "src/librssguard/database/sqlitedriver.cpp",
    "src/librssguard/database/mariadbdriver.cpp",
    "src/librssguard/database/databasedriver.cpp",
    "src/librssguard-gmail/src/gmailserviceroot.cpp",
    "src/librssguard-standard/src/rssguard4import.cpp",
    "src/librssguard-standard/src/quiterssimport.cpp",
    "src/librssguard/filtering/filteringsystem.cpp",
]
for file in literal_files:
    text = sources[file]
    dialects = ["sqlite"] if "sqlitedriver" in file or "import.cpp" in file else ["mariadb"] if "mariadbdriver" in file else ["sqlite", "mariadb"]
    for m in re.finditer(r"\b\w+\.(exec|prepare)\s*\(", text):
        arg = balanced_argument(text, m.end())
        # Ternary schema-version prepare is explicitly expanded below.
        if "empty_table" in arg:
            continue
        sql = first_literal(arg)
        if not sql or not re.match(r"(SELECT|PRAGMA|REINDEX|VACUUM|OPTIMIZE|SET|USE)\b", sql):
            continue
        line = line_of(text, m.start())
        function = containing_function(text, m.start())
        details = "Named placeholders are bound with QSqlQuery::bindValue." if ":" in sql else "Static SQL."
        extra = {"execution_api": m.group(1)}
        if "%1" in sql:
            details = "%1 is the configured database name escaped by DatabaseFactory::escapeIdentifier (backticks doubled, enclosed in backticks)."
        if "import.cpp" in file:
            extra["database_scope"] = "legacy-rssguard4-schema-10" if "rssguard4" in file else "legacy-quiterss-schema-17"
            details += " Executed against the separately opened import source DB, not the current-schema application DB."
        add(file, line, function, sql, details, dialects, **extra)

driver = "src/librssguard/database/databasedriver.cpp"
add(driver, 203, "DatabaseDriver::setSchemaVersion", "INSERT INTO Information VALUES ('schema_version', :schema_version);",
    "empty_table=true during initialization; :schema_version is APP_DB_SCHEMA_VERSION (105), stored as a string; executed at line 209.")
add(driver, 204, "DatabaseDriver::setSchemaVersion", "UPDATE Information SET inf_value = :schema_version WHERE inf_key = 'schema_version';",
    "empty_table=false after upgrading through all migrations; :schema_version is 105; executed at line 209.")
for line, function, suffix in [(118, "DatabaseDriver::setForeignKeyChecksEnabled", "ON"),
                               (124, "DatabaseDriver::setForeignKeyChecksDisabled", "OFF")]:
    add(driver, line, function, f"PRAGMA foreign_keys={suffix};",
        "Virtual foreignKeysEnable/Disable dispatch to SqliteDriver; SQLite caller is ServiceRoot::onSyncInFinished before/after replacing feeds and labels.", ["sqlite"])
    add(driver, line, function, f"SET FOREIGN_KEY_CHECKS={'1' if suffix == 'ON' else '0'};",
        "Virtual foreignKeysEnable/Disable dispatch to MariaDbDriver.", ["mariadb"])
add("src/librssguard/database/sqlitedriver.cpp", 176, "SqliteDriver::setPragmas", "PRAGMA foreign_keys=ON;",
    "Virtual foreignKeysEnable return value at line 211; setPragmas runs on schema initialization and each newly opened connection.", ["sqlite"])
add("src/librssguard/database/mariadbdriver.cpp", 203, "MariaDbDriver::afterAddDatabase",
    "CREATE DATABASE IF NOT EXISTS %1 CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;",
    "Only when initial USE fails; %1=escaped_database_name; query_db.exec(create_db) at line 209.", ["mariadb"])

fields = ", ".join([
    "Messages.id", "Messages.is_read", "Messages.is_important", "Messages.is_deleted", "Messages.is_pdeleted",
    "Messages.feed", "Messages.title", "Messages.url", "Messages.author", "Messages.date_created",
    "Messages.date_retrieved", "Messages.contents", "Messages.enclosures", "Messages.score", "Messages.account_id",
    "Messages.custom_id", "Messages.custom_data",
    "(SELECT GROUP_CONCAT(Labels.custom_id) FROM LabelsInMessages lim JOIN Labels ON lim.label = Labels.id WHERE lim.message = Messages.id) AS msg_labels",
])
sort_fields = ["Messages.id", "Messages.is_read", "Messages.is_important", "Messages.is_deleted", "Messages.is_pdeleted",
               "Messages.feed", "Messages.title", "Messages.url", "Messages.author", "Messages.date_created",
               "Messages.date_retrieved", "Messages.contents", "Messages.enclosures", "Messages.score", "Messages.account_id",
               "Messages.custom_id", "Messages.custom_data", "msg_labels"]
filters = {
    "empty": "0 > 1",
    "bin": "Messages.account_id = {account_id} AND Messages.is_deleted = 1 AND Messages.is_pdeleted = 0",
    "important": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.is_important = 1",
    "unread": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND Messages.is_read = 0",
    "probe-regex": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND (Messages.title REGEXP '{escaped_regex}' OR Messages.contents REGEXP '{escaped_regex}')",
    "probe-sql": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND ({user_authored_where})",
    "label": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND EXISTS (SELECT 1 FROM LabelsInMessages WHERE LabelsInMessages.label = {label_id} AND LabelsInMessages.message = Messages.id)",
    "all-labels": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0 AND EXISTS (SELECT 1 FROM LabelsInMessages WHERE LabelsInMessages.message = Messages.id)",
    "account": "Messages.account_id = {account_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
    "feeds-empty": "Messages.feed IN (NULL) AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
    "feed": "Messages.feed = {feed_id} AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
    "feeds-multiple": "Messages.feed IN ({comma_separated_feed_ids}) AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
}
details = (
    "SQL constructed by selectStatement at line 194, executed by fetchMessages at line 106. "
    "Projection is DatabaseQueries::messageTableAttributes(), including correlated label GROUP_CONCAT; "
    "fields and sorting columns are bounded. Filters are built by DatabaseQueries::whereClause* and supplied by "
    "ServiceRoot::loadMessagesForItem (serviceroot.cpp:991-1022). Probes permit arbitrary user-authored WHERE SQL "
    "(including subqueries/functions); regex is escaped using doubled single quotes. An optional "
    "additional_article_id>0 replaces WHERE with ({filter}) OR Messages.id = {additional_article_id}. "
    "orderByClause may be empty or contain up to three unique selected fields ASC/DESC plus id tie-breaker "
    "(primary direction); secondary id follows primary direction. Defaults are date_created DESC, title ASC, id DESC "
    "(messagesview.cpp:508-510). Restored column profiles/user choices may change ordering. "
    "limitOffset: limit>0 and offset>0 -> LIMIT L OFFSET O; limit>0 -> LIMIT L; "
    "offset>0 and limit<=0 -> SQLite LIMIT -1 OFFSET O / MariaDB LIMIT 184467440737095 OFFSET O; neither -> empty. "
    "Lazy loading defaults false (settingskeys.cpp:165), so initial selection returns ALL matching articles; "
    "with lazy loading default batch size=10000 (messagesmodel.h:15), initial offset=0, "
    "subsequent offset=current m_messages.size(). fetchAllArticles uses limit=0 and offset=current count."
)
entry = add("src/librssguard/core/messagesmodelsqllayer.cpp", 106, "MessagesModelSqlLayer::fetchMessages",
            f"SELECT {fields} FROM Messages WHERE {{filter}} {{order_by}} {{limit_offset}};", details,
            callsites=[{"file": "src/librssguard/core/messagesmodel.cpp", "line": 186, "function": "MessagesModel::fetchMoreArticles"},
                       {"file": "src/librssguard/core/messagesmodel.cpp", "line": 247, "function": "MessagesModel::fetchInitialArticles"}],
            builder_line=194, projection=fields, filter_variants=filters, sort_fields=sort_fields,
            default_order_by="ORDER BY Messages.date_created DESC, Messages.title ASC, Messages.id DESC",
            default_limit=0, lazy_batch_size=10000,
            selection_callsites=[{"file": "src/librssguard/services/abstract/serviceroot.cpp", "line": line, "variant": variant}
                                  for line, variant in [(993,"bin"),(996,"important"),(999,"unread"),(1002,"probe-regex/probe-sql"),
                                                        (1005,"label"),(1008,"all-labels"),(1011,"account"),(1014,"empty"),
                                                        (1022,"feeds-empty/feed/feeds-multiple")]])

duplicates = ("FilterMessage::duplicateWhereClause (filterobjects.cpp:432-477) appends selected equality predicates "
              "for each of SameTitle (title=:title), SameUrl (url=:url), SameAuthor (author=:author), "
              "SameDateCreated (date_created=:date_created milliseconds), SameCustomId (custom_id=:custom_id); "
              "all 2^5 combinations are possible. Always account_id=:account_id. ExistingArticles and id>0 add id!=:id. "
              "Unless AllFeedsSameAccount is selected add feed=:feed. Predicates join with AND. "
              "Bindings come from current article and filtering account.")
add("src/librssguard/filtering/filterobjects.cpp", 490, "FilterMessage::databaseMessage",
    f"SELECT {fields} FROM Messages WHERE {{where_clause}} ORDER BY Messages.id DESC LIMIT 1;",
    duplicates + " getArticleFromDatabase uses duplicateWhereClause; getArticleFromDatabaseWinkler instead uses Messages.id=:message_id. Executed at line 497.",
    where_variants=["{duplicateWhereClause}", "Messages.id = :message_id"], projection=fields)
add("src/librssguard/filtering/filterobjects.cpp", 543, "FilterMessage::isAlreadyInDatabase",
    "SELECT COUNT(*) FROM Messages WHERE {duplicateWhereClause};", duplicates + " Prepared at line 546; executed at line 552. Public Q_INVOKABLE exposed to user-authored JavaScript article filters.",
    callsites=[{"file": "src/librssguard/filtering/filterobjects.h", "line": 108, "function": "Q_INVOKABLE script API"}])

# The transaction APIs send SQL through the selected Qt driver without SqlQuery.
for file, function, specs in [
    ("src/librssguard/services/abstract/label.cpp", "Label::assignToMessages", [(79,"BEGIN"),(86,"COMMIT"),(91,"ROLLBACK")]),
    ("src/librssguard/services/abstract/label.cpp", "Label::deassignFromMessages", [(111,"BEGIN"),(118,"COMMIT"),(123,"ROLLBACK")]),
    ("src/librssguard/miscellaneous/feedreader.cpp", "FeedReader::removeMessageFilter", [(353,"BEGIN"),(361,"COMMIT"),(366,"ROLLBACK")]),
]:
    for line, sql in specs:
        add(file, line, function, sql,
            "Implicit Qt SQL driver statement from QSqlDatabase::transaction/commit/rollback; rollback only on exception. SQLite statement shown; MariaDB equivalent is driver-managed transaction operation.",
            dialects=["sqlite", "mariadb"], execution_api="QSqlDatabase")

# Resolve the few same-named APIs whose receiver type cannot be inferred from
# a lexical name search. These callers were read manually.
for entry in inventory:
    if entry["function"] == "FeedReader::removeMessageFilter":
        entry["callers"] = [{"file":"src/librssguard/gui/dialogs/formmessagefiltersmanager.cpp",
                             "line":433, "function":"FormMessageFiltersManager::removeSelectedFilter"}]
    if entry["function"] == "DatabaseDriver::setSchemaVersion":
        entry["callers"] = [{"file":driver,
                             "line":143 if entry["sql_template"].startswith("INSERT") else 191,
                             "function":"DatabaseDriver::updateDatabaseSchema"}]
    if entry["function"] == "FilteringSystem::duplicateCandidates":
        entry["dynamic_details"] += (" Account variant selected by AllFeedsSameAccount; otherwise feed variant. "
                                     "Loaded once per FilteringSystem/feed-fetch or existing-filter run and cached; "
                                     "Q_INVOKABLE isAlreadyInDatabaseWinkler/getArticleFromDatabaseWinkler use the candidates. "
                                     "No SQL LIMIT; rows fully consumed and Jaro-Winkler evaluation follows in C++.")
    entry["callers_resolution"] = "reviewed source call sites; driver calls are virtual dispatch"

expansions = {
    "sqlite": {"$$":"INTEGER PRIMARY KEY", "^^":"BLOB", "**":"TEXT", "~~":"COLLATE NOCASE", "§§1":"PRAGMA foreign_keys=ON;", "§§0":"PRAGMA foreign_keys=OFF;", "##":"`{escaped_database_name}`"},
    "mariadb": {"$$":"INTEGER AUTO_INCREMENT PRIMARY KEY", "^^":"MEDIUMBLOB", "**":"MEDIUMTEXT", "~~":"", "§§1":"SET FOREIGN_KEY_CHECKS=1;", "§§0":"SET FOREIGN_KEY_CHECKS=0;", "##":"`{escaped_database_name}`"},
}
include_notes = []
for path in sorted(ROOT.joinpath("resources/sql").glob("*.sql")):
    file = path.relative_to(ROOT).as_posix()
    text = path.read_text(encoding="utf-8-sig")
    file_dialects = ["sqlite"] if "sqlite" in path.name else ["mariadb"] if "mysql" in path.name else ["sqlite","mariadb"]
    for chunk in re.split(r"-- !\r?\n", text):
        sql = chunk.strip()
        if not sql:
            continue
        line = line_of(text, text.find(chunk)) + len(chunk) - len(chunk.lstrip())
        # The trim offset above is characters, so calculate by newline count.
        position = text.find(chunk) + len(chunk) - len(chunk.lstrip())
        line = line_of(text, position)
        if sql.startswith("!!"):
            include_notes.append(f"{file}:{line} includes {sql[3:]}")
            continue
        dialect_sql = {}
        for dialect in file_dialects:
            expanded = sql
            for placeholder, replacement in expansions[dialect].items():
                expanded = expanded.replace(placeholder, replacement)
            dialect_sql[dialect] = expanded
        if "db_init" in path.name:
            sites = [{"file":driver,"line":140,"function":"DatabaseDriver::updateDatabaseSchema"}]
            context = "Initialization only."
        else:
            sites = [{"file":driver,"line":181,"function":"DatabaseDriver::updateDatabaseSchema"}]
            context = "Historical migration; requires its matching pre-migration schema, not schema 105."
        add(file, line, "DatabaseDriver::updateDatabaseSchema via prepareScript", sql,
            context + " SQL resources split on -- ! newline; !! directives include common resource. "
            "Driver expands $$ primary key, ^^ blob, ** contents text, ~~ collation, ## escaped DB name, §§1/§§0 foreign-key commands before execution.",
            file_dialects, callsites=sites, dialect_sql=dialect_sql,
            database_scope="current-schema-initialization" if "db_init" in path.name else "historical-migration")

OUT.mkdir(parents=True, exist_ok=True)
OUT.joinpath("outer-inventory.json").write_text(json.dumps(inventory, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
OUT.joinpath("outer-notes.txt").write_text(
    "Outer inventory scope: all first-party src except DatabaseQueries .cpp/.h (owned by other agents); all resources/sql statements.\n"
    "Searches: first-party SqlQuery/QSqlQuery/QSqlDatabase/QSqlTableModel/QSqlQueryModel/sqlite3_/mysql_/mariadb_, .exec/.prepare/execBatch/setQuery, transaction/commit/rollback, SQL keywords.\n"
    "No QSqlTableModel or QSqlQueryModel instantiated; includes in standardserviceroot/ttrssserviceroot unused. DatabaseWorker routes callbacks and emits no SQL directly. DatabaseCleaner delegates query SQL to DatabaseQueries and maintenance SQL to drivers. SqlQuery wrapper executes/logs supplied statements only.\n"
    "All database-using service/plugins and GUI files other than the direct sites listed call DatabaseQueries methods; their callsites are captured by the DatabaseQueries inventory. Current database schema is 105.\n"
    "Excluded: vendored **/3rd-party/**, developer benchmark/generator files under resources/tests, comments and disabled block-commented transaction code in DatabaseQueries::storeMessages. Legacy source import queries ARE included, with separate schema scope because current-schema DB cannot execute them.\n"
    "Dynamic family coverage describes generators and full bounded choices, rather than enumerating infinite numeric/list values or every arbitrary user-supplied SQL WHERE clause. A finite benchmark cannot test every possible user-authored clause; representative clauses must be reported as samples.\n"
    "Application-authored SQL and Qt transaction operations are inventoried. Qt driver internal metadata/protocol statements are not authored in this repo and require runtime tracing of the exact deployed Qt plugin to enumerate exhaustively. QSqlDatabase connection APIs do not provide application SQL strings.\n"
    "Resource include directives are not SQL statements; all their included statements are inventoried in the common resource with both dialect expansions:\n" + "\n".join(include_notes) + "\n",
    encoding="utf-8")
print(f"Wrote {len(inventory)} outer entries")
