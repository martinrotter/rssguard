// Standalone audit runner: uses the same Qt QSQLITE driver as RSS Guard.
// Usage: qt_benchmark.exe DATABASE CASES.json RESULTS.json
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSysInfo>
#include <QTextStream>
#include <algorithm>
#include <stdexcept>

static void checked(QSqlQuery &q, const QString &sql) {
  if (!q.exec(sql)) throw std::runtime_error(q.lastError().text().toStdString());
}

static void configure(const QSqlDatabase &db) {
  QSqlQuery q(db);
  for (const auto &sql : {"PRAGMA foreign_keys=ON;", "PRAGMA encoding='UTF-8';",
                         "PRAGMA page_size=32768;", "PRAGMA cache_size=-65536;",
                         "PRAGMA mmap_size=100000000;", "PRAGMA synchronous=OFF;",
                         "PRAGMA temp_store=MEMORY;", "PRAGMA journal_mode=WAL;",
                         "PRAGMA busy_timeout=5000;"}) checked(q, QString::fromLatin1(sql));
}

static void writeJson(const QString &path, const QJsonObject &object) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Cannot write results");
  file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
}

static void prepare(QSqlQuery &q, const QJsonObject &test, const QString &prefix = {}) {
  if (!q.prepare(prefix + test["sql"].toString()))
    throw std::runtime_error(q.lastError().text().toStdString());
  const auto binds = test["bindings"].toObject();
  for (auto it = binds.begin(); it != binds.end(); ++it) q.bindValue(it.key(), it.value().toVariant());
}

static QJsonArray explain(const QSqlDatabase &db, const QJsonObject &test) {
  QSqlQuery q(db);
  prepare(q, test, "EXPLAIN QUERY PLAN ");
  if (!q.exec()) throw std::runtime_error(q.lastError().text().toStdString());
  QJsonArray rows;
  while (q.next()) rows.append(QJsonObject{{"id",q.value(0).toInt()},
                                          {"parent",q.value(1).toInt()},
                                          {"detail",q.value(3).toString()}});
  return rows;
}

static QJsonObject sample(QSqlDatabase db, const QJsonObject &test, int number) {
  const bool mutation = test["mutation"].toBool();
  if (mutation && !db.transaction()) throw std::runtime_error("Cannot begin isolation transaction");
  QSqlQuery q(db);
  q.setForwardOnly(true); // SqlQuery::SqlQuery uses exactly this setting.
  QElapsedTimer clock;
  clock.start();
  double prepareMs = 0;
  bool ok;
  if (test["direct_exec"].toBool()) {
    ok = q.exec(test["sql"].toString());
  } else {
    prepare(q, test);
    prepareMs = double(clock.nsecsElapsed()) / 1e6;
    ok = q.exec();
  }
  if (!ok) throw std::runtime_error(q.lastError().text().toStdString());
  const double execMs = double(clock.nsecsElapsed()) / 1e6;
  qint64 rows = 0, textCharacters = 0, binaryBytes = 0;
  const int columns = q.record().count();
  while (q.next()) {
    ++rows;
    for (int col = 0; col < columns; ++col) {
      const auto value = q.value(col);
      if (value.metaType().id() == QMetaType::QString) textCharacters += value.toString().size();
      else if (value.metaType().id() == QMetaType::QByteArray) binaryBytes += value.toByteArray().size();
    }
  }
  if (q.lastError().isValid()) throw std::runtime_error(q.lastError().text().toStdString());
  const double totalMs = double(clock.nsecsElapsed()) / 1e6;
  const qint64 affected = q.numRowsAffected();
  q.finish();
  if (mutation && !db.rollback()) throw std::runtime_error("Cannot roll back isolated mutation");
  return {{"sample", number}, {"cache",number == 0 ? "first_in_connection_os_cache_uncontrolled" : "repeated"},
          {"prepare_ms",prepareMs}, {"exec_including_prepare_ms",execMs}, {"total_ms",totalMs},
          {"rows",rows}, {"text_characters",textCharacters}, {"binary_bytes",binaryBytes},
          {"affected_rows",affected}};
}

int main(int argc, char **argv) {
  QCoreApplication app(argc, argv);
  QTextStream out(stdout);
  if (app.arguments().size() != 4) { out << "Expected DATABASE CASES.json RESULTS.json\n"; return 2; }
  const auto databasePath = app.arguments()[1], casesPath = app.arguments()[2], resultsPath = app.arguments()[3];
  QJsonObject report{{"qt_version",QString::fromLatin1(qVersion())},
                     {"platform",QSysInfo::prettyProductName()}, {"driver","QSQLITE"},
                     {"database",databasePath}, {"cases_file",casesPath},
                     {"status","running"}, {"forward_only",true},
                     {"measurement","prepare + exec + full row/value retrieval; excludes Message conversion, GUI and worker scheduling"},
                     {"cache_note","No OS cache flush. First sample is not a cold disk test."}};
  QJsonArray results;
  try {
    QFile casesFile(casesPath);
    if (!casesFile.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot open cases");
    const auto manifest = QJsonDocument::fromJson(casesFile.readAll()).object();
    QFile metadataFile(QFileInfo(databasePath).dir().filePath("fixture_metadata.json"));
    if (!metadataFile.open(QIODevice::ReadOnly)) throw std::runtime_error("Fixture metadata required");
    const auto metadata = QJsonDocument::fromJson(metadataFile.readAll()).object();
    if (metadata["status"].toString() != "complete" || !metadata["canonical_schema_matches"].toBool()
        || metadata["schema_version"].toString() != "105"
        || metadata["counts"].toObject()["Messages"].toInteger() < 1000000
        || QFileInfo(metadata["database"].toString()).canonicalFilePath() != QFileInfo(databasePath).canonicalFilePath())
      throw std::runtime_error("Refusing a path that is not the validated huge synthetic fixture");
    bool readOnly = true;
    for (const auto &value : manifest["cases"].toArray())
      if (value.toObject()["mutation"].toBool()) readOnly = false;
    report["order"] = manifest["order"];
    report["inventory_sha256"] = manifest["inventory_sha256"];
    auto db = QSqlDatabase::addDatabase("QSQLITE", "isolated_audit");
    db.setDatabaseName(databasePath);
    db.setConnectOptions(readOnly ? "QSQLITE_ENABLE_REGEXP;QSQLITE_OPEN_READONLY" : "QSQLITE_ENABLE_REGEXP");
    report["read_only"] = readOnly;
    if (!db.open()) throw std::runtime_error(db.lastError().text().toStdString());
    configure(db);
    QSqlQuery info(db);
    checked(info, "SELECT sqlite_version();"); info.next(); report["sqlite_version"] = info.value(0).toString(); info.finish();
    QJsonObject pragmas;
    for (const auto &name : {"foreign_keys","page_size","cache_size","mmap_size","synchronous","temp_store","journal_mode","busy_timeout"}) {
      checked(info, "PRAGMA " + QString::fromLatin1(name) + ";"); info.next();
      pragmas[QString::fromLatin1(name)] = QJsonValue::fromVariant(info.value(0)); info.finish();
    }
    report["pragmas"] = pragmas;
    for (const auto &value : manifest["cases"].toArray()) {
      const auto test = value.toObject();
      out << "Testing " << test["id"].toString() << ": " << test["scenario"].toString() << Qt::endl;
      QJsonObject result = test;
      result["plan"] = explain(db, test);
      QJsonArray samples;
      QList<double> repeated;
      for (int run = 0; run < 4; ++run) {
        const auto timed = sample(db, test, run);
        samples.append(timed);
        if (run > 0) repeated.append(timed["total_ms"].toDouble());
        out << "  " << run << ": " << timed["total_ms"].toDouble() << " ms; " << timed["rows"].toInteger() << " rows" << Qt::endl;
      }
      std::sort(repeated.begin(), repeated.end());
      const double median = repeated[1], threshold = test["threshold_ms"].toDouble(250.0);
      result["samples"] = samples; result["median_repeated_ms"] = median; result["threshold_ms"] = threshold;
      const bool slow = median >= threshold;
      result["status"] = slow ? "potentially_slow" : "passed";
      results.append(result);
      report["results"] = results;
      writeJson(resultsPath, report);
      if (slow) {
        report["status"] = "stopped_at_first_potentially_slow_query";
        report["stopped_case_id"] = test["id"];
        writeJson(resultsPath, report);
        out << "STOP: median " << median << " ms >= " << threshold << " ms. No later cases executed." << Qt::endl;
        db.close();
        return 10;
      }
    }
    report["status"] = "all_manifest_cases_passed";
    report["results"] = results;
    writeJson(resultsPath, report);
    db.close();
    return 0;
  } catch (const std::exception &e) {
    report["status"] = "error"; report["error"] = QString::fromUtf8(e.what()); report["results"] = results;
    writeJson(resultsPath, report); out << "ERROR: " << e.what() << Qt::endl; return 1;
  }
}
