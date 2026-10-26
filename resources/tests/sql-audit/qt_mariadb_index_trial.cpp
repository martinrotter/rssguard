// Native QMYSQL index-only A/B trial against one disposable MariaDB fixture.
// DDL commits in MariaDB: index/edge cleanup is explicit, never a DDL rollback.
// Usage: qt_mariadb_index_trial.exe METADATA.json ORIGINAL_RESULTS.json OUTPUT.json
// Forced candidate: qt_mariadb_index_trial.exe --force-candidate METADATA.json ORIGINAL_RESULTS.json OUTPUT.json
// Representative single-feed controls: --feed-controls METADATA.json ORIGINAL_RESULTS.json OUTPUT.json
// Connection-only profile: qt_mariadb_index_trial.exe --smoke OUTPUT.json
#define main originalAuditMain
#include "qt_benchmark.cpp"
#undef main
#include <QCryptographicHash>
#include <QDataStream>
#include <QStorageInfo>

static const QString schema = QStringLiteral("rssguard_sql_audit_20261014_maria");
static const QString indexName = QStringLiteral("idx_Messages_feed_display");
static const qint64 edgeFirst = 2100001, edgeLast = 2100008;
static QString expectedDataDir;
static qint64 originalAutoIncrement=0;

static QString quoted(QString value) {
  value.replace('`', "``");
  return '`' + value + '`';
}

static QJsonObject readObject(const QString &path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot open JSON input");
  QJsonParseError error;
  const auto document = QJsonDocument::fromJson(file.readAll(), &error);
  if (error.error != QJsonParseError::NoError || !document.isObject())
    throw std::runtime_error("Invalid JSON input");
  return document.object();
}

static QVariant scalar(const QSqlDatabase &db, const QString &sql) {
  QSqlQuery q(db); checked(q, sql);
  if (!q.next()) throw std::runtime_error("Expected one scalar row");
  return q.value(0);
}

static void diskGuard() {
  QStorageInfo disk(expectedDataDir);
  if (!disk.isValid() || disk.bytesAvailable() < 1000000000LL)
    throw std::runtime_error("Maria trial requires at least 1GB free disk");
}

static QJsonObject profile(const QSqlDatabase &db) {
  QSqlQuery q(db);
  checked(q, "SELECT VERSION() AS server_version, DATABASE() AS database_name, @@datadir AS data_dir, "
             "@@collation_database AS collation_database, @@collation_connection AS collation_connection, "
             "@@character_set_client AS character_set_client, @@character_set_connection AS character_set_connection, "
             "@@sql_mode AS sql_mode, @@group_concat_max_len AS group_concat_max_len, "
             "@@innodb_page_size AS innodb_page_size, @@innodb_buffer_pool_size AS innodb_buffer_pool_size, "
             "@@foreign_key_checks AS foreign_key_checks, @@check_constraint_checks AS check_constraint_checks, "
             "@@default_storage_engine AS default_storage_engine, @@innodb_default_row_format AS default_row_format, "
             "@@innodb_flush_log_at_trx_commit AS innodb_flush_log_at_trx_commit;");
  if (!q.next()) throw std::runtime_error("Cannot read server profile");
  QJsonObject result;
  const auto record = q.record();
  for (int i=0; i<record.count(); ++i) result[record.fieldName(i)] = QJsonValue::fromVariant(q.value(i));
  if (result["database_name"].toString() != schema
      || !result["server_version"].toString().contains("MariaDB", Qt::CaseInsensitive)
      || QFileInfo(result["data_dir"].toString()).canonicalFilePath() != QFileInfo(expectedDataDir).canonicalFilePath())
    throw std::runtime_error("Connection is not the explicitly isolated local MariaDB server/schema");
  return result;
}

static QJsonArray mariaExplain(const QSqlDatabase &db, const QString &sql) {
  QSqlQuery q(db); checked(q, "EXPLAIN " + sql);
  const auto record = q.record(); QJsonArray rows;
  while (q.next()) {
    QJsonObject row;
    for (int col=0; col<record.count(); ++col) row[record.fieldName(col)] = QJsonValue::fromVariant(q.value(col));
    rows.append(row);
  }
  if (q.lastError().isValid()) throw std::runtime_error(q.lastError().text().toStdString());
  return rows;
}

static QJsonObject messagePlan(const QJsonArray &plan) {
  for (const auto &value : plan) {
    const auto row = value.toObject();
    if (row["table"].toString().compare("Messages", Qt::CaseInsensitive) == 0) {
      // possible_keys, cardinality and cost estimates are not physical plan identity.
      QStringList extra = row["Extra"].toString().split(';', Qt::SkipEmptyParts);
      for (auto &part : extra) part = part.simplified();
      std::sort(extra.begin(), extra.end());
      return {{"table", "Messages"}, {"key", row["key"]}, {"type", row["type"]}, {"Extra", extra.join("; ")}};
    }
  }
  throw std::runtime_error("EXPLAIN does not contain Messages");
}

static QStringList indexes(const QSqlDatabase &db) {
  QSqlQuery q(db);
  checked(q, "SELECT DISTINCT INDEX_NAME FROM information_schema.statistics WHERE TABLE_SCHEMA=DATABASE() "
             "AND LOWER(TABLE_NAME)='messages' ORDER BY INDEX_NAME;");
  QStringList names; while (q.next()) names.append(q.value(0).toString());
  return names;
}

static QString replayOriginal(const QSqlDatabase &db, const QString &sql, const QJsonObject &plan) {
  QString hint;
  if (plan["type"].toString() == "ALL") {
    QStringList names;
    for (const auto &name : indexes(db)) names.append(quoted(name));
    if (names.isEmpty()) throw std::runtime_error("Cannot construct ALL-plan index exclusion");
    hint = " IGNORE INDEX (" + names.join(',') + ")";
  } else {
    const QString name = plan["key"].toString();
    if (name.isEmpty()) throw std::runtime_error("Original non-ALL plan has no chosen key");
    hint = " FORCE INDEX (" + quoted(name) + ")";
  }
  QString result = sql;
  if (result.count(" FROM Messages WHERE ") != 1) throw std::runtime_error("Unexpected query shape");
  result.replace(" FROM Messages WHERE ", " FROM Messages" + hint + " WHERE ");
  return result;
}

static QString forcedCandidate(QString sql) {
  if (sql.count(" FROM Messages WHERE ")!=1) throw std::runtime_error("Unexpected candidate query shape");
  sql.replace(" FROM Messages WHERE "," FROM Messages FORCE INDEX ("+quoted(indexName)+") WHERE ");
  return sql;
}

static QJsonObject fingerprint(const QSqlDatabase &db, const QString &sql) {
  QSqlQuery q(db); q.setForwardOnly(true); checked(q,sql);
  const int columns = q.record().count();
  if (columns != 18) throw std::runtime_error("Equivalence query must retrieve all 18 original columns");
  QCryptographicHash hash(QCryptographicHash::Sha256); qint64 count=0;
  while (q.next()) {
    QByteArray bytes; QDataStream stream(&bytes,QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0); stream << quint32(columns);
    // QVariant serialization retains NULL/value types, bytes and exact label strings.
    for (int c=0; c<columns; ++c) stream << q.value(c);
    hash.addData(bytes); ++count;
  }
  if (q.lastError().isValid()) throw std::runtime_error(q.lastError().text().toStdString());
  return {{"rows",count},{"columns",columns},{"ordered_all_values_sha256",QString::fromLatin1(hash.result().toHex())}};
}

static double median(QList<double> values) {
  std::sort(values.begin(),values.end());
  if (values.isEmpty()) throw std::runtime_error("No samples");
  return values.size()%2 ? values[values.size()/2] : (values[values.size()/2-1]+values[values.size()/2])/2;
}

static QJsonObject statistics(const QJsonArray &samples) {
  QList<double> values;
  for (const auto &s:samples) values.append(s.toObject()["total_ms"].toDouble());
  if (values.isEmpty()) throw std::runtime_error("No measurements");
  return {{"samples",samples.size()},{"median_ms",median(values)},
          {"min_ms",*std::min_element(values.begin(),values.end())},
          {"max_ms",*std::max_element(values.begin(),values.end())}};
}

static QJsonArray writes(QSqlDatabase db, QTextStream &out) {
  QSqlQuery source(db);
  checked(source,"SELECT feed,title,is_read,is_important,is_deleted,url,author,score,date_created,date_retrieved,contents,enclosures,custom_id,custom_data,account_id FROM Messages WHERE id=2000000;");
  if (!source.next()) throw std::runtime_error("Missing write seed");
  QList<QVariant> seed; for (int col=0; col<15; ++col) seed.append(source.value(col)); source.finish();
  QStringList rowSql;
  for (int row=0; row<1000; ++row) {
    QStringList values;
    for (int col=0; col<15; ++col) {
      QString value=seed[col].toString();
      if (col==1) value=QString("Trial insert %1").arg(row);
      if (col==12) value=QString("trial-only-%1").arg(row);
      if (col==0 || col==2 || col==3 || col==4 || col==7 || col==8 || col==9 || col==14) values.append(value);
      else { value.replace('\'',"''"); values.append('\''+value+'\''); }
    }
    rowSql.append("("+values.join(',')+")");
  }
  const QString sql="INSERT INTO Messages (feed,title,is_read,is_important,is_deleted,url,author,score,date_created,date_retrieved,contents,enclosures,custom_id,custom_data,account_id) VALUES "+rowSql.join(',')+';';
  if (!db.transaction()) throw std::runtime_error("Cannot begin write-control transaction");
  QJsonArray result;
  try {
    QSqlQuery control(db);
    for (int run=0; run<5; ++run) {
      diskGuard(); checked(control,"SAVEPOINT trial_write;");
      QSqlQuery q(db); QElapsedTimer timer; timer.start();
      if (!q.exec(sql)) throw std::runtime_error(q.lastError().text().toStdString());
      const double total=double(timer.nsecsElapsed())/1e6;
      const auto affected=q.numRowsAffected(); q.finish();
      if (affected != 1000) throw std::runtime_error("Write control affected unexpected row count");
      checked(control,"ROLLBACK TO SAVEPOINT trial_write;"); checked(control,"RELEASE SAVEPOINT trial_write;");
      diskGuard();
      if (run>0) result.append(QJsonObject{{"sample",run},{"total_ms",total},{"prepare_bind_ms",0},
                                          {"engine_exec_ms",total},{"rows",affected}});
      out << "  bulk insert " << run << ": " << total << " ms; " << affected << " rows (rolled back)" << Qt::endl;
    }
    if (!db.rollback()) throw std::runtime_error("Write-control rollback failed");
  } catch (...) { db.rollback(); throw; }
  return result;
}

static QJsonObject indexSize(const QSqlDatabase &db) {
  QSqlQuery q(db);
  checked(q,"SELECT index_name,stat_value FROM mysql.innodb_index_stats WHERE database_name=DATABASE() "
            "AND LOWER(table_name)='messages' AND LOWER(index_name)=LOWER('"+indexName+"') AND stat_name='size';");
  if (!q.next()) return {{"available",false},{"note","No persistent index-size row available; no ANALYZE was issued"}};
  const qint64 pages=q.value(1).toLongLong();
  const qint64 pageSize=scalar(db,"SELECT @@innodb_page_size;").toLongLong();
  return {{"available",true},{"index_name",q.value(0).toString()},{"pages",pages},
          {"page_size",pageSize},{"estimated_bytes",pages*pageSize},{"source","mysql.innodb_index_stats stat_name=size"}};
}

static QString edgeSql(QString sql) {
  sql.replace("Messages.feed = 1 AND", "Messages.feed = 750 AND");
  return sql;
}

static QJsonArray edgeIds(const QSqlDatabase &db, QString sql) {
  sql.replace("Messages.feed = 750 AND", "Messages.id BETWEEN 2100001 AND 2100008 AND Messages.feed = 750 AND");
  QSqlQuery q(db); q.setForwardOnly(true); checked(q,sql); QJsonArray ids;
  while (q.next()) ids.append(q.value(0).toLongLong());
  return ids;
}

static void cleanup(QSqlDatabase db, bool dropIndex, bool removeEdges) {
  if (!db.isOpen()) return;
  db.rollback(); // End any active DML transaction before DDL; no DDL rollback claim.
  QSqlQuery q(db);
  if (dropIndex) checked(q,"DROP INDEX IF EXISTS "+quoted(indexName)+" ON Messages;");
  if (removeEdges) {
    if (!db.transaction()) throw std::runtime_error("Cannot begin committed edge cleanup");
    checked(q,"DELETE FROM Messages WHERE id BETWEEN 2100001 AND 2100008;");
    if (!db.commit()) throw std::runtime_error("Cannot commit edge cleanup");
  }
  // InnoDB consumes AUTO_INCREMENT values even for rolled-back inserts.
  // This separate DDL restores that counter only inside the guarded fixture.
  if (originalAutoIncrement>0)
    checked(q,"ALTER TABLE Messages AUTO_INCREMENT="+QString::number(originalAutoIncrement)+";");
}

static void verifyRestoration(QJsonObject &report,const QSqlDatabase &db,const QString &sql) {
  report["trial_index_removed"]=!indexes(db).contains(indexName,Qt::CaseInsensitive);
  report["trial_rows_removed"]=scalar(db,"SELECT COUNT(*) FROM Messages WHERE id BETWEEN 2100001 AND 2100008;").toInt()==0;
  report["trial_label_links_removed"]=scalar(db,"SELECT COUNT(*) FROM LabelsInMessages WHERE message BETWEEN 2100001 AND 2100008;").toInt()==0;
  report["message_count_restored"]=scalar(db,"SELECT COUNT(*) FROM Messages;").toLongLong()==2000000;
  report["baseline_restored_fingerprint"]=fingerprint(db,sql);
  report["baseline_restored"]=report["baseline_restored_fingerprint"]==report["original_fingerprint"];
  report["restored_plan"]=mariaExplain(db,sql); report["restored_chosen_plan"]=messagePlan(report["restored_plan"].toArray());
  report["auto_increment_after_trial"]=QJsonValue::fromVariant(scalar(db,"SELECT AUTO_INCREMENT FROM information_schema.tables WHERE TABLE_SCHEMA=DATABASE() AND LOWER(TABLE_NAME)='messages';"));
  report["auto_increment_restored"]=report["auto_increment_after_trial"].toInteger()==originalAutoIncrement;
  if (!report["trial_index_removed"].toBool() || !report["trial_rows_removed"].toBool()
      || !report["trial_label_links_removed"].toBool() || !report["message_count_restored"].toBool()
      || !report["baseline_restored"].toBool() || !report["auto_increment_restored"].toBool())
    throw std::runtime_error("Explicit fixture restoration checks failed");
}

static void representativeFeeds(QSqlDatabase db,QJsonObject &report,const QJsonObject &test,
                                const QString &output,QTextStream &out,bool &trialIndexCreated) {
  const QString sql=test["sql"].toString();
  const QList<int> feeds={6000,750,2,751};
  QJsonArray originals;
  report["original_fingerprint"]=fingerprint(db,sql);
  for (const int feed:feeds) {
    QString controlSql=sql;
    controlSql.replace("Messages.feed = 1 AND","Messages.feed = "+QString::number(feed)+" AND");
    QJsonObject control{{"feed",feed},{"original_sql",controlSql}};
    control["scenario"]=feed==6000?"empty":feed==750?"populated_small":feed==751?"medium_heavy":"ordinary";
    control["before_index_plan"]=mariaExplain(db,controlSql);
    control["before_index_chosen_plan"]=messagePlan(control["before_index_plan"].toArray());
    control["original_fingerprint"]=fingerprint(db,controlSql);
    out << "Feed control " << feed << ": " << control["original_fingerprint"].toObject()["rows"].toInteger() << " visible rows" << Qt::endl;
    originals.append(control);
  }
  QSqlQuery q(db); diskGuard(); QElapsedTimer timer; timer.start(); trialIndexCreated=true;
  checked(q,report["index_sql"].toString());
  report["index_build_ms"]=double(timer.nsecsElapsed())/1e6; report["index_size"]=indexSize(db);
  QJsonArray completed;
  for (const auto &value:originals) {
    auto control=value.toObject();
    const QString originalSql=control["original_sql"].toString();
    const QString baselineSql=replayOriginal(db,originalSql,control["before_index_chosen_plan"].toObject());
    const QString candidateSql=forcedCandidate(originalSql);
    control["interleaved_baseline_sql"]=baselineSql; control["candidate_sql"]=candidateSql;
    control["interleaved_baseline_plan"]=mariaExplain(db,baselineSql);
    control["interleaved_baseline_chosen_plan"]=messagePlan(control["interleaved_baseline_plan"].toArray());
    control["candidate_plan"]=mariaExplain(db,candidateSql);
    control["candidate_chosen_plan"]=messagePlan(control["candidate_plan"].toArray());
    const auto candidatePlan=control["candidate_chosen_plan"].toObject();
    if (candidatePlan["key"].toString().compare(indexName,Qt::CaseInsensitive)!=0
        || candidatePlan["Extra"].toString().contains("filesort",Qt::CaseInsensitive))
      throw std::runtime_error("Representative forced candidate did not use the display key without filesort");
    if (control["interleaved_baseline_chosen_plan"]!=control["before_index_chosen_plan"])
      throw std::runtime_error("Representative original key/type/Extra replay differs");
    control["baseline_replay_fingerprint"]=fingerprint(db,baselineSql);
    control["candidate_fingerprint"]=fingerprint(db,candidateSql);
    control["equivalence_passed"]=control["original_fingerprint"]==control["candidate_fingerprint"]
                                  && control["original_fingerprint"]==control["baseline_replay_fingerprint"];
    if (!control["equivalence_passed"].toBool()) throw std::runtime_error("Representative ordered full-value fingerprint differs");
    QJsonObject baseline=test,candidate=test; baseline["sql"]=baselineSql; candidate["sql"]=candidateSql;
    sample(db,baseline,0); sample(db,candidate,0); QJsonArray baselineSamples,candidateSamples;
    out << "Six balanced native pairs for feed " << control["feed"].toInt() << Qt::endl;
    for (int pair=0;pair<6;++pair) {
      for (int slot=0;slot<2;++slot) {
        const bool useCandidate=pair%2==0?slot==1:slot==0;
        const auto timed=sample(db,useCandidate?candidate:baseline,pair+1);
        if (useCandidate) candidateSamples.append(timed); else baselineSamples.append(timed);
        out << "  " << pair << (useCandidate?" forced: ":" baseline: ") << timed["total_ms"].toDouble() << " ms" << Qt::endl;
      }
      control["baseline_samples"]=baselineSamples; control["candidate_samples"]=candidateSamples;
      auto progress=completed; progress.append(control); report["representative_feed_cases"]=progress;
      writeJson(output,report); diskGuard();
    }
    control["baseline_stats"]=statistics(baselineSamples); control["candidate_stats"]=statistics(candidateSamples);
    control["latency_change_percent"]=(control["candidate_stats"].toObject()["median_ms"].toDouble()
                                       /control["baseline_stats"].toObject()["median_ms"].toDouble()-1)*100;
    completed.append(control); report["representative_feed_cases"]=completed; writeJson(output,report);
  }
  q.finish(); cleanup(db,trialIndexCreated,false); trialIndexCreated=false;
  verifyRestoration(report,db,sql);
  // Each control's natural no-index query also matches after explicit DROP.
  QJsonArray restored;
  for (const auto &value:completed) {
    auto control=value.toObject();
    control["restored_fingerprint"]=fingerprint(db,control["original_sql"].toString());
    control["restored"]=control["restored_fingerprint"]==control["original_fingerprint"];
    if (!control["restored"].toBool()) throw std::runtime_error("Representative control restoration differs");
    restored.append(control);
  }
  report["representative_feed_cases"]=restored;
}

int main(int argc,char **argv) {
  QCoreApplication app(argc,argv); QTextStream out(stdout);
  auto arguments=app.arguments();
  const bool feedControls=arguments.size()>1 && arguments[1]=="--feed-controls";
  const bool forceCandidate=feedControls || (arguments.size()>1 && arguments[1]=="--force-candidate");
  if (forceCandidate) arguments.removeAt(1);
  const bool smoke=arguments.size()==3 && arguments[1]=="--smoke";
  if (!smoke && arguments.size()!=4) {
    out << "Expected [--force-candidate|--feed-controls] METADATA.json ORIGINAL_RESULTS.json OUTPUT.json, or --smoke OUTPUT.json\n"; return 2;
  }
  const QString output=smoke?arguments[2]:arguments[3];
  expectedDataDir=QDir(QCoreApplication::applicationDirPath()).filePath("data");
  app.addLibraryPath(QDir(QCoreApplication::applicationDirPath()).filePath("qtplugins"));
  QJsonObject report{{"status","running"},{"backend","MariaDB"},{"driver","QMYSQL"},
                    {"qt_version",QString::fromLatin1(qVersion())},{"host","127.0.0.1"},{"port",33316},{"schema",schema},
                    {"lazy_loading_changed",false},{"limit_added",false},{"forward_only",true},
                    {"force_candidate",forceCandidate},
                    {"mode",feedControls?"representative_feed_controls":forceCandidate?"forced_candidate":"unforced_candidate"},
                    {"measurement","Same sample() implementation as SQLite audit: direct exec + full row/value retrieval; native QMYSQL client buffering included; no object/UI conversion. Fingerprints untimed. OS cache not flushed."}};
  QSqlDatabase db; bool trialIndexCreated=false, edgeRowsCreated=false, candidateRejected=false;
  try {
    QJsonObject metadata, oldReport, test;
    if (!smoke) {
      metadata=readObject(arguments[1]);
      if (metadata["status"].toString()!="complete" || metadata["database"].toString()!=schema
          || metadata["schema_version"].toString()!="105" || !metadata["all_logical_rows_match_sqlite"].toBool()
          || metadata["counts"].toObject()["Messages"].toInteger()!=2000000)
        throw std::runtime_error("Full imported and validated isolated MariaDB fixture is required");
      oldReport=readObject(arguments[2]);
      const auto oldResults=oldReport["results"].toArray();
      if (oldResults.isEmpty()) throw std::runtime_error("Original audit has no results");
      test=oldResults.last().toObject();
      if (test["id"].toString()!="O052/hot-feed-default-all" || !test["direct_exec"].toBool()
          || test["sql"].toString().contains("LIMIT",Qt::CaseInsensitive))
        throw std::runtime_error("Unchanged direct full-result finding SQL is required");
    }
    db=QSqlDatabase::addDatabase("QMYSQL","isolated_mariadb_index_trial");
    db.setHostName("127.0.0.1"); db.setPort(33316); db.setDatabaseName(schema);
    db.setUserName(qEnvironmentVariable("RSSGUARD_SQL_AUDIT_MARIADB_USER","root"));
    db.setPassword(qEnvironmentVariable("RSSGUARD_SQL_AUDIT_MARIADB_PASSWORD"));
    db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=10;MYSQL_OPT_READ_TIMEOUT=300;MYSQL_OPT_WRITE_TIMEOUT=300");
    if (!db.open()) throw std::runtime_error(db.lastError().text().toStdString());
    report["connection_profile_before_settings"]=profile(db);
    QSqlQuery q(db);
    if (smoke) {
      report["profile"]=report["connection_profile_before_settings"];
      report["status"]="connection_profile_passed_no_benchmark";
      writeJson(output,report); db.close(); out << "Native QMYSQL connection/profile passed; no benchmark run." << Qt::endl; return 0;
    }
    // MariaDbDriver::setPragmas, without altering server/global settings.
    checked(q,"SET NAMES 'utf8mb4';"); checked(q,"SET CHARACTER SET utf8mb4;");
    report["profile"]=profile(db);
    diskGuard();
    if (!report["profile"].toObject()["server_version"].toString().startsWith("11.8.8")
        || report["profile"].toObject()["collation_database"].toString()!="utf8mb4_unicode_ci"
        || report["profile"].toObject()["foreign_key_checks"].toInteger()!=1
        || report["profile"].toObject()["check_constraint_checks"].toInteger()!=1)
      throw std::runtime_error("Fixture server version/collation/constraint profile differs from the requested target");
    if (scalar(db,"SELECT inf_value FROM Information WHERE inf_key='schema_version';").toString()!="105"
        || scalar(db,"SELECT COUNT(*) FROM Messages;").toLongLong()!=2000000
        || scalar(db,"SELECT COUNT(*) FROM Messages WHERE id BETWEEN 2100001 AND 2100008;").toInt()!=0
        || indexes(db).contains(indexName,Qt::CaseInsensitive))
      throw std::runtime_error("Fixture schema/count/edge/index preconditions failed");
    checked(q,"SHOW CREATE TABLE Messages;"); q.next(); report["messages_create_table"]=q.value(1).toString(); q.finish();
    originalAutoIncrement=scalar(db,"SELECT AUTO_INCREMENT FROM information_schema.tables WHERE TABLE_SCHEMA=DATABASE() AND LOWER(TABLE_NAME)='messages';").toLongLong();
    if (originalAutoIncrement<=2000000) throw std::runtime_error("Unexpected baseline AUTO_INCREMENT state");
    report["auto_increment_before_trial"]=originalAutoIncrement;
    report["metadata_file"]=arguments[1]; report["original_results_file"]=arguments[2];
    report["original_sql"]=test["sql"]; report["original_source"]=test["source"];
    const QString sql=test["sql"].toString(), smallSql=edgeSql(sql);
    report["index_sql"]="CREATE INDEX "+quoted(indexName)+" ON Messages (feed,is_deleted,is_pdeleted,date_created DESC,title ASC,id DESC);";
    if (feedControls) {
      representativeFeeds(db,report,test,output,out,trialIndexCreated);
      report["status"]="complete_representative_controls_explicit_cleanup";
      writeJson(output,report); db.close(); out << "Representative native feed controls complete; explicit cleanup/restoration verified." << Qt::endl; return 0;
    }
    report["baseline_plan"]=mariaExplain(db,sql);
    const auto originalPlan=messagePlan(report["baseline_plan"].toArray()); report["baseline_chosen_plan"]=originalPlan;
    out << "Native MariaDB full-result baseline" << Qt::endl;
    QJsonArray pre;
    for (int run=0; run<4; ++run) {
      const auto timed=sample(db,test,run); pre.append(timed);
      out << "  " << run << ": " << timed["total_ms"].toDouble() << " ms; " << timed["rows"].toInteger() << " rows" << Qt::endl;
    }
    report["before_index_samples"]=pre; report["original_fingerprint"]=fingerprint(db,sql);
    QList<double> baselineRepeated;
    for (int i=1;i<pre.size();++i) baselineRepeated.append(pre[i].toObject()["total_ms"].toDouble());
    report["baseline_repeated_pre_index_median_ms"]=median(baselineRepeated);
    out << "Baseline literal bulk insert cost" << Qt::endl;
    report["baseline_write_samples"]=writes(db,out);
    // Edge rows are committed because CREATE INDEX is a committing DDL operation.
    edgeRowsCreated=true;
    if (!db.transaction()) throw std::runtime_error("Cannot begin edge insert transaction");
    const QStringList titles={"Alpha","alpha",QString::fromUtf8("caf\xC3\xA9"),"cafe ","Alpha ","BETA","beta",QString::fromUtf8("\xC3\xA9" "clair")};
    for (int i=0; i<8; ++i) {
      const int deleted=i==5||i==7, purged=i>=6;
      QString title=titles[i]; title.replace('\'',"''");
      checked(q,QString("INSERT INTO Messages (id,is_read,is_important,is_deleted,is_pdeleted,feed,title,url,author,date_created,contents,enclosures,score,account_id,custom_id,custom_data,date_retrieved) SELECT %1,%2,is_important,%3,%4,750,'%5',NULL,'',1791936000000,contents,enclosures,score,1,'trial-edge-%1',NULL,1791936000000 FROM Messages WHERE id=2000000;")
                   .arg(edgeFirst+i).arg(i%2).arg(deleted).arg(purged).arg(title));
    }
    checked(q,"INSERT INTO LabelsInMessages(message,label) VALUES(2100001,1),(2100001,2),(2100002,1);");
    if (!db.commit()) throw std::runtime_error("Cannot commit edge rows");
    report["edge_sql"]=smallSql; report["edge_baseline_fingerprint"]=fingerprint(db,smallSql);
    report["edge_native_visible_ids_before"]=edgeIds(db,smallSql);
    const QJsonArray expectedEdgeIds={2100005,2100002,2100001,2100004,2100003};
    report["edge_expected_native_visible_ids"]=expectedEdgeIds;
    if (report["edge_native_visible_ids_before"].toArray()!=expectedEdgeIds)
      throw std::runtime_error("Native Unicode/case/trailing-space/id-tie or deletion-flag edge control differs");
    out << "Creating full MariaDB composite index (explicit cleanup, no DDL rollback)" << Qt::endl;
    diskGuard(); QElapsedTimer build; build.start(); trialIndexCreated=true; checked(q,report["index_sql"].toString());
    report["index_build_ms"]=double(build.nsecsElapsed())/1e6; diskGuard();
    report["index_size"]=indexSize(db);
    QJsonObject candidate=test;
    if (forceCandidate) candidate["sql"]=forcedCandidate(sql);
    const QString candidateSql=candidate["sql"].toString(), candidateSmallSql=edgeSql(candidateSql);
    report["candidate_sql"]=candidateSql;
    report["indexed_plan"]=mariaExplain(db,candidateSql); report["indexed_chosen_plan"]=messagePlan(report["indexed_plan"].toArray());
    if (forceCandidate) {
      const auto chosen=report["indexed_chosen_plan"].toObject();
      if (chosen["key"].toString().compare(indexName,Qt::CaseInsensitive)!=0
          || chosen["Extra"].toString().contains("filesort",Qt::CaseInsensitive)) {
        candidateRejected=true;
        throw std::runtime_error("Forced candidate did not use the candidate key without filesort; stopped before candidate timing");
      }
      out << "Forced candidate plan uses display index without filesort; first-sample regression guard" << Qt::endl;
      const auto first=sample(db,candidate,0);
      report["first_forced_candidate_sample"]=first;
      report["early_regression_threshold_factor"]=1.5;
      report["early_regression_threshold_ms"]=median(baselineRepeated)*1.5;
      out << "  first forced candidate: " << first["total_ms"].toDouble() << " ms" << Qt::endl;
      if (first["total_ms"].toDouble()>=median(baselineRepeated)*1.5) {
        candidateRejected=true;
        throw std::runtime_error("First forced-candidate sample regressed >=50% against repeated baseline; stopped before additional candidate execution");
      }
    }
    report["indexed_fingerprint"]=fingerprint(db,candidateSql); report["edge_indexed_fingerprint"]=fingerprint(db,candidateSmallSql);
    report["edge_native_visible_ids_indexed"]=edgeIds(db,candidateSmallSql);
    if (report["original_fingerprint"]!=report["indexed_fingerprint"]
        || report["edge_baseline_fingerprint"]!=report["edge_indexed_fingerprint"]
        || report["edge_native_visible_ids_before"]!=report["edge_native_visible_ids_indexed"])
      throw std::runtime_error("Ordered all-value equivalence failed");
    report["equivalence_passed"]=true;
    const QString replaySql=replayOriginal(db,sql,originalPlan), smallReplaySql=edgeSql(replaySql);
    QJsonObject baseline=test; baseline["sql"]=replaySql;
    report["interleaved_baseline_sql"]=replaySql; report["interleaved_baseline_plan"]=mariaExplain(db,replaySql);
    report["interleaved_baseline_chosen_plan"]=messagePlan(report["interleaved_baseline_plan"].toArray());
    if (report["interleaved_baseline_chosen_plan"].toObject()!=originalPlan
        || fingerprint(db,replaySql)!=report["original_fingerprint"])
      throw std::runtime_error("Original chosen key/type/Extra replay or full equivalence changed");
    out << "Six balanced interleaved full-result A/B pairs" << Qt::endl;
    sample(db,baseline,0); sample(db,candidate,0); QJsonArray originalSamples,indexedSamples;
    for (int pair=0; pair<6; ++pair) {
      for (int slot=0; slot<2; ++slot) {
        const bool indexed=pair%2==0?slot==1:slot==0;
        const auto timed=sample(db,indexed?candidate:baseline,pair+1);
        if (indexed) indexedSamples.append(timed); else originalSamples.append(timed);
        out << "  pair " << pair << (indexed?" index: ":" original plan: ") << timed["total_ms"].toDouble() << " ms" << Qt::endl;
      }
      report["interleaved_baseline_samples"]=originalSamples; report["interleaved_indexed_samples"]=indexedSamples;
      writeJson(output,report); diskGuard();
    }
    report["baseline_stats"]=statistics(originalSamples); report["indexed_stats"]=statistics(indexedSamples);
    out << "Indexed literal bulk insert cost" << Qt::endl;
    report["indexed_write_samples"]=writes(db,out);
    report["baseline_write_stats"]=statistics(report["baseline_write_samples"].toArray());
    report["indexed_write_stats"]=statistics(report["indexed_write_samples"].toArray());
    report["write_control"]="Actual application literal 1000-row INSERT/15 columns; SQL execution timed; generation/commit/rollback excluded; DML transaction + savepoint rollback. InnoDB AUTO_INCREMENT allocations are not rolled back.";
    if (!db.transaction()) throw std::runtime_error("Cannot begin edge update transaction");
    checked(q,"UPDATE Messages SET title='Beta',date_created=1791936000001,is_deleted=0,is_pdeleted=0 WHERE id=2100007;");
    checked(q,"UPDATE Messages SET is_deleted=1 WHERE id=2100002;");
    if (!db.commit()) throw std::runtime_error("Cannot commit edge updates");
    report["edge_updated_original_plan_fingerprint"]=fingerprint(db,smallReplaySql);
    report["edge_updated_indexed_fingerprint"]=fingerprint(db,candidateSmallSql);
    report["edge_updates_equivalent"]=report["edge_updated_original_plan_fingerprint"]==report["edge_updated_indexed_fingerprint"];
    if (!report["edge_updates_equivalent"].toBool()) throw std::runtime_error("Edge-update equivalence failed");
    out << "Explicit candidate DROP and committed edge/cascade cleanup" << Qt::endl;
    q.finish(); cleanup(db,trialIndexCreated,edgeRowsCreated); trialIndexCreated=false; edgeRowsCreated=false;
    verifyRestoration(report,db,sql);
    report["status"]="complete_explicit_cleanup"; writeJson(output,report); db.close();
    out << "Maria trial complete; full values/order equal; candidate index and edge data removed." << Qt::endl; return 0;
  } catch (const std::exception &error) {
    report["status"]=candidateRejected?"candidate_rejected_early":"error"; report["error"]=QString::fromUtf8(error.what());
    try {
      cleanup(db,trialIndexCreated,edgeRowsCreated);
      if (report.contains("original_fingerprint")) verifyRestoration(report,db,report["original_sql"].toString());
      report["error_cleanup_completed"]=true;
    }
    catch (const std::exception &cleanupError) { report["error_cleanup_completed"]=false; report["cleanup_error"]=QString::fromUtf8(cleanupError.what()); }
    writeJson(output,report); out << "ERROR: " << error.what() << Qt::endl;
    if (db.isOpen()) db.close(); return candidateRejected?11:1;
  }
}
