"""Generate a review-only patch/overlay and disposable native C++ checks.

No production files are edited and this generator never opens a database.
CHANGES and the exact index SQL constants are exported for independent checkers.
"""
import difflib
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'benchmark-results/sql-audit-20261014'
OVERLAY = OUT / 'dual-proposal-source'
CPP = OUT / 'dual-proposal-cpp'
SQLITE_INDEX_SQL = ('CREATE INDEX IF NOT EXISTS idx_Messages_feed_display '
                    'ON Messages (feed, date_created DESC, title ASC, id DESC) '
                    'WHERE is_deleted = 0 AND is_pdeleted = 0;')
MARIA_INDEX_SQL = ('CREATE INDEX IF NOT EXISTS idx_Messages_feed_display '
                   'ON Messages (feed, is_deleted, is_pdeleted, date_created DESC, title ASC, id DESC);')

HELPER = r'''// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDDISPLAYINDEX_H
#define FEEDDISPLAYINDEX_H

#include <QString>

namespace FeedDisplayIndex {
inline QString mariaCreateSql() {
  return QStringLiteral("CREATE INDEX IF NOT EXISTS idx_Messages_feed_display "
                        "ON Messages (feed, is_deleted, is_pdeleted, date_created DESC, title ASC, id DESC);");
}

inline bool isOptionalMariaSchemaStatement(const QString& statement) {
  return statement.simplified() == mariaCreateSql();
}

inline QString metadataSql() {
  return QStringLiteral("SELECT VERSION(), COALESCE(GROUP_CONCAT("
                        "CONCAT(SEQ_IN_INDEX, ':', LOWER(COLUMN_NAME), ':', COALESCE(COLLATION, ''), ':', "
                        "COALESCE(SUB_PART, 0), ':', UPPER(INDEX_TYPE), ':', UPPER(IGNORED)) "
                        "ORDER BY SEQ_IN_INDEX SEPARATOR ','), '') "
                        "FROM information_schema.STATISTICS "
                        "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'Messages' "
                        "AND INDEX_NAME = 'idx_Messages_feed_display';");
}

inline bool usableMariaIndex(const QString& server_version, const QString& signature) {
  // Check actual directions/full columns, rather than parsing a numeric version.
  // Older servers report ASC for ignored DESC, or cannot read IGNORED metadata.
  return server_version.contains(QStringLiteral("MariaDB"), Qt::CaseInsensitive) &&
         signature == QStringLiteral("1:feed:A:0:BTREE:NO,2:is_deleted:A:0:BTREE:NO,"
                                     "3:is_pdeleted:A:0:BTREE:NO,4:date_created:D:0:BTREE:NO,"
                                     "5:title:A:0:BTREE:NO,6:id:D:0:BTREE:NO");
}

inline bool canonicalSingleFeed(const QString& filter) {
  const QString prefix = QStringLiteral("Messages.feed = ");
  const QString suffix = QStringLiteral(" AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0");
  if (!filter.startsWith(prefix) || !filter.endsWith(suffix)) {
    return false;
  }

  const QString id_text = filter.mid(prefix.size(), filter.size() - prefix.size() - suffix.size());
  bool ok = false;
  const int id = id_text.toInt(&ok);
  return ok && id > 0 && id_text == QString::number(id);
}

inline bool eligible(bool available,
                     const QString& filter,
                     const QString& order,
                     int limit,
                     int offset,
                     int additional_article_id) {
  return available && limit <= 0 && offset == 0 && additional_article_id <= 0 &&
         order == QStringLiteral("ORDER BY Messages.date_created DESC, Messages.title ASC, Messages.id DESC") &&
         canonicalSingleFeed(filter);
}

inline QString applyHint(const QString& statement) {
  QString hinted = statement;
  const QString from = QStringLiteral(" FROM Messages WHERE ");
  const int at = hinted.indexOf(from);
  if (at >= 0) {
    hinted.insert(at + QStringLiteral(" FROM Messages").size(),
                  QStringLiteral(" FORCE INDEX (idx_Messages_feed_display)"));
  }
  return hinted;
}

inline bool retryWithoutHint(const QString& fallback, const QString& native_error_code) {
  // MariaDB reports 1176 for a dropped or ignored hinted index.
  return !fallback.isEmpty() && native_error_code == QStringLiteral("1176");
}
} // namespace FeedDisplayIndex

#endif // FEEDDISPLAYINDEX_H
'''


def replace_once(text, old, new, name):
    if text.count(old) != 1:
        raise ValueError(f'{name}: expected one source anchor, found {text.count(old)}')
    return text.replace(old, new, 1)


def read(name):
    return (ROOT / name).read_text(encoding='utf-8')


def proposed_changes():
    changes = {'src/librssguard/database/feeddisplayindex.h': HELPER}
    name = 'src/librssguard/database/databasedriver.h'
    changes[name] = replace_once(read(name), '    virtual QString version() = 0;',
        '    virtual QString version() = 0;\n'
        '    virtual bool feedDisplayIndexAvailable() const { return false; }', name)
    changes[name] = replace_once(changes[name], '    void setSchemaVersion(SqlQuery& query, int new_schema_version, bool empty_table);',
        '    virtual void executeSchemaStatement(SqlQuery& query, const QString& statement);\n\n'
        '    void setSchemaVersion(SqlQuery& query, int new_schema_version, bool empty_table);', name)
    name = 'src/librssguard/database/databasedriver.cpp'
    changes[name] = read(name)
    if changes[name].count('            query_db.exec(statement);') != 1 or changes[name].count('        query_db.exec(statement);') != 2:
        raise ValueError('Unexpected schema statement loops')
    changes[name] = changes[name].replace('query_db.exec(statement);', 'executeSchemaStatement(query_db, statement);')
    changes[name] = replace_once(changes[name], 'void DatabaseDriver::setSchemaVersion(',
        'void DatabaseDriver::executeSchemaStatement(SqlQuery& query, const QString& statement) {\n'
        '  query.exec(statement);\n}\n\nvoid DatabaseDriver::setSchemaVersion(', name)
    name = 'src/librssguard/database/mariadbdriver.h'
    changes[name] = replace_once(read(name), '    virtual QString version();',
        '    virtual QString version();\n    virtual bool feedDisplayIndexAvailable() const;', name)
    changes[name] = replace_once(changes[name], '    virtual void setPragmas(SqlQuery& query);',
        '    virtual void setPragmas(SqlQuery& query);\n'
        '    virtual void updateDatabaseSchema(QSqlDatabase& db, const QString& database_name = {});\n'
        '    virtual void executeSchemaStatement(SqlQuery& query, const QString& statement);\n\n'
        '  private:\n'
        '    // Set once during initial schema setup, before DatabaseWorker starts.\n'
        '    bool m_feedDisplayIndexAvailable = false;', name)
    name = 'src/librssguard/database/mariadbdriver.cpp'
    changes[name] = replace_once(read(name), '#include "database/databasefactory.h"',
        '#include "database/databasefactory.h"\n#include "database/feeddisplayindex.h"', name)
    methods = '''bool MariaDbDriver::feedDisplayIndexAvailable() const {
  return m_feedDisplayIndexAvailable;
}

void MariaDbDriver::executeSchemaStatement(SqlQuery& query, const QString& statement) {
  if (!FeedDisplayIndex::isOptionalMariaSchemaStatement(statement)) {
    DatabaseDriver::executeSchemaStatement(query, statement);
  }
  else if (!query.exec(statement, false)) {
    qWarningNN << LOGSEC_DB << "Optional feed display index unavailable; retaining normal query planning:"
               << query.lastError().text();
  }
}

void MariaDbDriver::updateDatabaseSchema(QSqlDatabase& db, const QString& database_name) {
  DatabaseDriver::updateDatabaseSchema(db, database_name);
  SqlQuery query(db);

  // Initial connection only; never mutate this cache from reader/writer threads.
  if (query.exec(FeedDisplayIndex::metadataSql(), false) && query.next()) {
    m_feedDisplayIndexAvailable =
      FeedDisplayIndex::usableMariaIndex(query.value(0).toString(), query.value(1).toString());
  }
}

'''
    changes[name] = replace_once(changes[name], 'QString MariaDbDriver::foreignKeysEnable() const {',
        methods + 'QString MariaDbDriver::foreignKeysEnable() const {', name)
    name = 'src/librssguard/core/messagesmodelsqllayer.h'
    changes[name] = replace_once(read(name), '    QString selectStatement(int limit, int offset, int additional_article_id = -1) const;',
        '    QString selectStatement(int limit, int offset, int additional_article_id = -1,\n'
        '                            QString* fallback_statement = nullptr) const;', name)
    name = 'src/librssguard/core/messagesmodelsqllayer.cpp'
    changes[name] = replace_once(read(name), '#include "database/databasequeries.h"',
        '#include "database/databasequeries.h"\n#include "database/feeddisplayindex.h"\n'
        '#include "exceptions/sqlexception.h"', name)
    changes[name] = replace_once(changes[name], '  QString statemnt = selectStatement(limit, offset, additional_article_id);',
        '  QString fallback_statement;\n'
        '  QString statemnt = selectStatement(limit, offset, additional_article_id, &fallback_statement);', name)
    changes[name] = replace_once(changes[name], '    q.exec(statemnt);', '''    if (fallback_statement.isEmpty()) {
      q.exec(statemnt);
    }
    else if (!q.exec(statemnt, false)) {
      const QSqlError error = q.lastError();

      if (!FeedDisplayIndex::retryWithoutHint(fallback_statement, error.nativeErrorCode())) {
        THROW_EX(SqlException, error);
      }

      // Failed exec has yielded no rows; retry the unchanged, unhinted SELECT.
      q.finish();
      q.exec(fallback_statement);
    }''', name)
    changes[name] = replace_once(changes[name], 'QString MessagesModelSqlLayer::selectStatement(int limit, int offset, int additional_article_id) const {',
        'QString MessagesModelSqlLayer::selectStatement(int limit, int offset, int additional_article_id,\n'
        '                                               QString* fallback_statement) const {', name)
    old = '''  return QL1S("SELECT ") + formatFields() + QL1S(" FROM Messages WHERE ") + fltr + QL1S(" ") + orderByClause() +
         QL1S(" ") + limitOffset(limit, offset) + QL1C(';');'''
    new = '''  const QString order = orderByClause();
  const QString statement = QL1S("SELECT ") + formatFields() + QL1S(" FROM Messages WHERE ") + fltr + QL1S(" ") + order +
                            QL1S(" ") + limitOffset(limit, offset) + QL1C(';');
  const bool hinted = FeedDisplayIndex::eligible(qApp->database()->driver()->feedDisplayIndexAvailable(),
                                                m_filter, order, limit, offset, additional_article_id);

  if (fallback_statement != nullptr) {
    *fallback_statement = hinted ? statement : QString();
  }

  return hinted ? FeedDisplayIndex::applyHint(statement) : statement;'''
    changes[name] = replace_once(changes[name], old, new, name)
    name = 'src/librssguard/CMakeLists.txt'
    changes[name] = replace_once(read(name), '  database/databasefactory.h\n',
        '  database/databasefactory.h\n  database/feeddisplayindex.h\n', name)
    name = 'src/librssguard/definitions/definitions.h'
    changes[name] = replace_once(read(name), '#define APP_DB_SCHEMA_VERSION                "105"',
        '#define APP_DB_SCHEMA_VERSION                "106"', name)
    for backend, ddl in [('sqlite', SQLITE_INDEX_SQL), ('mysql', MARIA_INDEX_SQL)]:
        name = f'resources/sql/db_init_{backend}.sql'
        changes[name] = read(name).rstrip() + '\n-- !\n' + ddl + '\n'
        name = f'resources/sql/db_update_{backend}_105_106.sql'
        changes[name] = ('USE ##;\n-- !\n' if backend == 'mysql' else '') + ddl + '\n'
    name = 'resources/sql.qrc'
    changes[name] = replace_once(read(name), '  </qresource>',
        '\n    <file>sql/db_update_sqlite_105_106.sql</file>\n'
        '    <file>sql/db_update_mysql_105_106.sql</file>\n  </qresource>', name)
    return changes


CHANGES = proposed_changes()


def extract_method(text, name):
    start = text.index('QString MessagesModelSqlLayer::' + name + '(')
    brace = text.index('{', start)
    # Only selectStatement's outer/inner blocks; strings contain no braces here.
    depth = 1
    end = brace + 1
    while depth:
        if text[end] == '{': depth += 1
        if text[end] == '}': depth -= 1
        end += 1
    return text[start:end]


NATIVE_TEST_PREFIX = r'''#include "database/feeddisplayindex.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <algorithm>
#include <stdexcept>

#define QSL(value) QStringLiteral(value)
#define QL1S(value) QLatin1String(value)
#define QL1C(value) QLatin1Char(value)

struct DriverStub { bool available = false; bool feedDisplayIndexAvailable() const { return available; } };
struct FactoryStub { DriverStub value; DriverStub* driver() { return &value; } };
struct ApplicationStub { FactoryStub value; FactoryStub* database() { return &value; } };
ApplicationStub appStub;
#undef qApp
#define qApp (&appStub)

class MessagesModelSqlLayer {
public:
  QString m_filter, fields, order;
  QString selectStatement(int limit, int offset, int additional_article_id=-1, QString* fallback_statement=nullptr) const;
  QString formatFields() const { return fields; }
  QString orderByClause() const { return order; }
  QString limitOffset(int limit, int offset) const {
    if(limit>0 && offset>0) return QString("LIMIT %1 OFFSET %2").arg(limit).arg(offset);
    if(limit>0) return QString("LIMIT %1").arg(limit);
    if(offset>0) return QString("LIMIT 184467440737095 OFFSET %1").arg(offset);
    return {};
  }
};

class OriginalSqlLayer : public MessagesModelSqlLayer {
public:
  QString selectStatement(int limit, int offset, int additional_article_id=-1) const;
};
'''

NATIVE_TEST_BODY = r'''
static QJsonArray checks;
static void check(const QString& name, bool passed) {
  checks.append(QJsonObject{{"name",name},{"passed",passed}});
  if(!passed) throw std::runtime_error(name.toStdString());
}

static QString feedFilter(const QString& id) {
  return "Messages.feed = "+id+" AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0";
}

static double median(QList<double> values) {
  std::sort(values.begin(),values.end()); return values[values.size()/2];
}

int main(int argc,char** argv) {
  QCoreApplication app(argc,argv);
  if(app.arguments().size()!=3) return 2;
  QJsonObject report{{"status","running"},{"qt_version",QString::fromLatin1(qVersion())},
                    {"scope","Native Qt guard/metadata helpers and exact proposed/original selectStatement bodies; factory/field/order inputs isolated. No database queries. Actual changed translation units compiled separately."},
                    {"database_queries_executed",0},{"production_files_modified",false}};
  try {
    QFile input(app.arguments()[1]); if(!input.open(QIODevice::ReadOnly)) throw std::runtime_error("Input open failed");
    auto audit=QJsonDocument::fromJson(input.readAll()).object();
    const QString originalSql=audit["results"].toArray().last().toObject()["sql"].toString();
    const QString from=" FROM Messages WHERE ";
    const QString fields=originalSql.mid(7,originalSql.indexOf(from)-7);
    const QString defaultOrder="ORDER BY Messages.date_created DESC, Messages.title ASC, Messages.id DESC";
    const QString signature="1:feed:A:0:BTREE:NO,2:is_deleted:A:0:BTREE:NO,3:is_pdeleted:A:0:BTREE:NO,4:date_created:D:0:BTREE:NO,5:title:A:0:BTREE:NO,6:id:D:0:BTREE:NO";
    check("native-supported-index",FeedDisplayIndex::usableMariaIndex("11.8.8-MariaDB",signature));
    check("legacy-prefix-no-numeric-parser",FeedDisplayIndex::usableMariaIndex("5.5.5-10.8.2-MariaDB",signature));
    check("non-Maria-MySQL-disabled",!FeedDisplayIndex::usableMariaIndex("8.4.7-MySQL",signature));
    check("unknown-missing-index-disabled",!FeedDisplayIndex::usableMariaIndex("11.8.8-MariaDB",{}));
    for(const auto& replacement:QStringList{"4:date_created:A:0:BTREE:NO","4:date_created:D:191:BTREE:NO","4:date_created:D:0:HASH:NO","4:date_created:D:0:BTREE:YES"}) {
      QString altered=signature; altered.replace("4:date_created:D:0:BTREE:NO",replacement);
      check("metadata-reject-"+replacement,!FeedDisplayIndex::usableMariaIndex("11.8.8-MariaDB",altered));
    }
    check("old-server-ASC-only-fallback",!FeedDisplayIndex::usableMariaIndex("10.7.8-MariaDB",QString(signature).replace(":D:",":A:")));
    check("metadata-truncation-fallback",!FeedDisplayIndex::usableMariaIndex("11.8.8-MariaDB",signature.left(80)));
    check("optional-DDL-exact",FeedDisplayIndex::isOptionalMariaSchemaStatement(FeedDisplayIndex::mariaCreateSql()));
    check("optional-DDL-whitespace",FeedDisplayIndex::isOptionalMariaSchemaStatement("\n  "+FeedDisplayIndex::mariaCreateSql()+"\n"));
    check("other-DDL-remains-mandatory",!FeedDisplayIndex::isOptionalMariaSchemaStatement("CREATE INDEX idx_other ON Messages(feed);"));
    check("different-layout-remains-mandatory",!FeedDisplayIndex::isOptionalMariaSchemaStatement(QString(FeedDisplayIndex::mariaCreateSql()).replace("id DESC","id ASC")));
    check("hint-index-missing-retry",FeedDisplayIndex::retryWithoutHint(originalSql,"1176"));
    check("ordinary-query-never-retries",!FeedDisplayIndex::retryWithoutHint({},"1176"));
    check("unrelated-error-never-retries",!FeedDisplayIndex::retryWithoutHint(originalSql,"1054"));

    MessagesModelSqlLayer proposed; proposed.fields=fields; proposed.order=defaultOrder; proposed.m_filter=feedFilter("1");
    OriginalSqlLayer original; original.fields=fields; original.order=defaultOrder; original.m_filter=proposed.m_filter;
    appStub.value.value.available=true;
    QString fallback; const QString candidate=proposed.selectStatement(0,0,-1,&fallback);
    check("full-original-query-exact",original.selectStatement(0,0,-1)==originalSql);
    check("forced-query-matches-measured-shape",candidate==QString(originalSql).replace(from," FROM Messages FORCE INDEX (idx_Messages_feed_display) WHERE "));
    check("fallback-is-exact-original",fallback==originalSql);
    check("projection-all18-unchanged",candidate.startsWith("SELECT "+fields+" FROM Messages FORCE INDEX"));
    check("correlated-label-aggregate-preserved",candidate.contains("SELECT GROUP_CONCAT(Labels.custom_id) FROM LabelsInMessages lim JOIN Labels"));
    check("no-LIMIT-introduced",!candidate.contains("LIMIT"));
    check("zero-additional-id-same-guard",proposed.selectStatement(0,0,0).contains(" FORCE INDEX "));
    check("negative-limit-unbounded",proposed.selectStatement(-1,0,-1).contains(" FORCE INDEX "));
    appStub.value.value.available=false;
    check("SQLite-unhinted-same-SQL",proposed.selectStatement(0,0,-1,&fallback)==originalSql && fallback.isEmpty());
    appStub.value.value.available=true;
    check("positive-limit-unhinted",!proposed.selectStatement(10000,0,-1,&fallback).contains(" FORCE INDEX ") && fallback.isEmpty());
    check("positive-offset-unhinted",!proposed.selectStatement(0,10000,-1).contains(" FORCE INDEX "));
    check("additional-id-OR-unhinted",!proposed.selectStatement(0,0,99).contains(" FORCE INDEX ") && proposed.selectStatement(0,0,99).contains("OR Messages.id = 99"));

    const QStringList negativeFilters={feedFilter("0"),feedFilter("-1"),feedFilter("01"),feedFilter("+1"),feedFilter("2147483648"),feedFilter("1 "),
      "Messages.feed IN (1, 2) AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
      "Messages.feed IN (NULL) AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
      "Messages.account_id = 1 AND Messages.is_deleted = 0 AND Messages.is_pdeleted = 0",
      "Messages.account_id = 1 AND Messages.is_deleted = 1 AND Messages.is_pdeleted = 0",
      feedFilter("1")+" AND Messages.is_read = 0",feedFilter("1")+" AND Messages.is_important = 1",
      feedFilter("1")+" AND EXISTS (SELECT 1 FROM LabelsInMessages WHERE message=Messages.id)",
      "("+feedFilter("1")+")",feedFilter("1")+" OR Messages.id = 99",feedFilter("1")+"\n",feedFilter("1")+";", "0 > 1"};
    int i=0;
    for(const auto& filter:negativeFilters) {
      proposed.m_filter=filter; original.m_filter=filter;
      check(QString("negative-filter-%1").arg(++i),proposed.selectStatement(0,0,-1,&fallback)==original.selectStatement(0,0,-1) && fallback.isEmpty());
    }
    proposed.m_filter=original.m_filter=feedFilter("1");
    const QStringList negativeOrders={QString(),"ORDER BY Messages.date_created DESC, Messages.id DESC",
      "ORDER BY Messages.date_created ASC, Messages.title ASC, Messages.id ASC",
      "ORDER BY Messages.date_created DESC, Messages.title DESC, Messages.id DESC",
      "ORDER BY Messages.title ASC, Messages.date_created DESC, Messages.id ASC",
      defaultOrder+", Messages.score DESC","ORDER BY msg_labels ASC, Messages.id ASC",
      "ORDER BY Messages.is_read ASC, Messages.id ASC","ORDER BY Messages.score DESC, Messages.id DESC"};
    i=0;
    for(const auto& order:negativeOrders) {
      proposed.order=original.order=order;
      check(QString("negative-sort-%1").arg(++i),proposed.selectStatement(0,0,-1,&fallback)==original.selectStatement(0,0,-1) && fallback.isEmpty());
    }
    proposed.order=original.order=defaultOrder;
    proposed.m_filter=feedFilter("2147483647");
    check("positive-int32-maximum",proposed.selectStatement(0,0,-1).contains(" FORCE INDEX "));
    proposed.m_filter=original.m_filter=feedFilter("1");

    // Repeated render-only measurements use no server and are reported separately.
    constexpr int loops=20000; quint64 sink=0; QJsonArray overhead;
    for(const auto& mode:QStringList{"canonical-Maria","canonical-SQLite","noncanonical-Maria"}) {
      appStub.value.value.available=(mode!="canonical-SQLite");
      proposed.m_filter=original.m_filter=(mode=="noncanonical-Maria" ? feedFilter("1")+" AND Messages.is_read=0" : feedFilter("1"));
      QList<double> before,after;
      for(int round=0;round<7;++round) {
        for(int slot=0;slot<2;++slot) {
          const bool updated=round%2 ? slot==0 : slot==1;
          QElapsedTimer timer; timer.start();
          for(int run=0;run<loops;++run) {
            const QString value=updated ? proposed.selectStatement(0,0,-1,&fallback) : original.selectStatement(0,0,-1);
            sink+=quint64(value.size());
          }
          const double ns=double(timer.nsecsElapsed())/loops;
          if(round>0) (updated?after:before).append(ns);
        }
      }
      overhead.append(QJsonObject{{"mode",mode},{"calls_per_sample",loops},{"samples_per_variant",6},
                                 {"original_median_ns_per_call",median(before)},
                                 {"proposed_median_ns_per_call",median(after)},
                                 {"delta_ns_per_call",median(after)-median(before)}});
    }
    report["render_only_overhead"]=overhead; report["render_sink"]=qint64(sink);
    report["checks"]=checks; report["checks_passed"]=checks.size(); report["status"]="passed";
  } catch(const std::exception& error) { report["status"]="error"; report["error"]=QString::fromUtf8(error.what()); report["checks"]=checks; }
  QFile output(app.arguments()[2]); if(!output.open(QIODevice::WriteOnly)) return 3;
  output.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
  QTextStream(stdout) << "Native C++ proposal checks: " << report["status"].toString() << ", " << checks.size() << " checks; zero DB queries.\n";
  return report["status"].toString()=="passed" ? 0 : 1;
}
'''


def generate():
    OUT.mkdir(parents=True, exist_ok=True)
    CPP.mkdir(parents=True, exist_ok=True)
    patch = []
    baselines = {}
    added = removed = 0
    for name, text in CHANGES.items():
        original = read(name) if (ROOT / name).exists() else ''
        destination = OVERLAY / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(text, encoding='utf-8')
        # This checkout mixes LF and CRLF, even within a few files. Preserve
        # unchanged raw line endings so an ordinary git apply --check succeeds.
        old_raw = (ROOT / name).read_bytes().decode('utf-8') if (ROOT / name).exists() else ''
        old_raw_lines = old_raw.splitlines(True)
        old_lines, new_lines = original.splitlines(True), text.splitlines(True)
        default_eol = '\r\n' if old_raw.count('\r\n') > old_raw.count('\n') / 2 else '\n'
        new_raw_lines = []
        for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, old_lines, new_lines, autojunk=False).get_opcodes():
            if tag == 'equal':
                new_raw_lines.extend(old_raw_lines[i1:i2])
            elif tag in ('replace', 'insert'):
                new_raw_lines.extend(line[:-1] + default_eol if line.endswith('\n') else line
                                     for line in new_lines[j1:j2])
        diff = list(difflib.unified_diff(old_raw_lines, new_raw_lines,
            fromfile='a/' + name if original else '/dev/null', tofile='b/' + name))
        patch.append('diff --git a/' + name + ' b/' + name + '\n')
        if not original: patch.append('new file mode 100644\n')
        for line in diff:
            patch.append(line if line.endswith('\n') else line + '\n\\ No newline at end of file\n')
        added += sum(line.startswith('+') and not line.startswith('+++') for line in diff)
        removed += sum(line.startswith('-') and not line.startswith('---') for line in diff)
        if original: baselines[name] = hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
    (OUT / 'dual-backend-proposal.patch').write_text(''.join(patch), encoding='utf-8', newline='\n')
    original_method = extract_method(read('src/librssguard/core/messagesmodelsqllayer.cpp'), 'selectStatement')
    original_method = original_method.replace('MessagesModelSqlLayer::', 'OriginalSqlLayer::')
    proposed_method = extract_method(CHANGES['src/librssguard/core/messagesmodelsqllayer.cpp'], 'selectStatement')
    (CPP / 'native_proposal_checks.cpp').write_text(NATIVE_TEST_PREFIX + '\n' + original_method + '\n' + proposed_method + NATIVE_TEST_BODY, encoding='utf-8')
    # Actual changed translation units compile against a private overlay before
    # existing includes. These compile checks do not invoke an application link.
    flag_text = read('build/Desktop_Qt_6_11_0_MSVC2022_64bit-Debug/src/librssguard/CMakeFiles/rssguard.dir/flags.make')
    flags = {kind: re.search(r'^CXX_' + kind + r' = (.*)$', flag_text, re.M).group(1)
             for kind in ('DEFINES', 'INCLUDES', 'FLAGS')}
    flags['FLAGS'] = flags['FLAGS'].replace('/Zi', '/Z7')
    commands = ['@echo off', 'setlocal',
                'call "C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"',
                'if errorlevel 1 exit /b %errorlevel%']
    for name in ('database/databasedriver.cpp', 'database/mariadbdriver.cpp', 'core/messagesmodelsqllayer.cpp'):
        stem = Path(name).stem
        rsp = CPP / (stem + '.rsp')
        rsp.write_text('/nologo /c\n' + '-I"' + (OVERLAY / 'src/librssguard').as_posix() + '"\n' +
            flags['DEFINES'] + '\n' + flags['INCLUDES'] + '\n' + flags['FLAGS'] + '\n' +
            '/Fo"' + (CPP / (stem + '.obj')).as_posix() + '"\n' +
            '/Fd"' + (CPP / (stem + '.pdb')).as_posix() + '"\n' +
            '"' + (OVERLAY / 'src/librssguard' / name).as_posix() + '"\n', encoding='utf-8')
        commands += ['cl.exe @"' + rsp.as_posix() + '" > "' + (CPP / (stem + '-compile.log')).as_posix() + '" 2>&1',
                     'if errorlevel 1 exit /b %errorlevel%']
    rsp = CPP / 'native-checks.rsp'
    rsp.write_text('/nologo /O2 /MD /EHsc /std:c++17 /Zc:__cplusplus /permissive- /utf-8 /DQT_NO_DEBUG\n' +
        '-I"' + (OVERLAY / 'src/librssguard').as_posix() + '"\n' +
        '-IC:/Qt/6.11.0/msvc2022_64/include -IC:/Qt/6.11.0/msvc2022_64/include/QtCore\n' +
        '/Fo"' + (CPP / 'native_proposal_checks.obj').as_posix() + '"\n' +
        '/Fe"' + (CPP / 'native_proposal_checks.exe').as_posix() + '"\n' +
        '"' + (CPP / 'native_proposal_checks.cpp').as_posix() + '"\n' +
        '/link /LIBPATH:C:/Qt/6.11.0/msvc2022_64/lib Qt6Core.lib\n', encoding='utf-8')
    commands += ['cl.exe @"' + rsp.as_posix() + '" > "' + (CPP / 'native-checks-compile.log').as_posix() + '" 2>&1',
                 'exit /b %errorlevel%']
    (CPP / 'compile-proposal.cmd').write_text('\n'.join(commands) + '\n', encoding='utf-8')
    manifest = {'status': 'review_only_unapplied', 'files': list(CHANGES), 'added_lines': added,
                'removed_lines': removed, 'production_source_sha256': baselines,
                'sqlite_index_sql': SQLITE_INDEX_SQL, 'maria_index_sql': MARIA_INDEX_SQL,
                'capability_sql': 'See generated feeddisplayindex.h metadataSql()',
                'schema_version': '106', 'old_indexes_removed': [], 'database_queries_executed_by_generator': 0}
    (OUT / 'dual-backend-proposal-metadata.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: manifest[k] for k in ('status', 'added_lines', 'removed_lines', 'schema_version')}, indent=2))


if __name__ == '__main__':
    generate()
