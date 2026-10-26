// Transactional index-only A/B trial. Full-result query and app settings unchanged.
// Reuse the original runner's exact timing/value retrieval implementation.
#define main originalAuditMain
#include "qt_benchmark.cpp"
#undef main
#include <QCryptographicHash>
#include <QDataStream>
#include <QStorageInfo>

static const QString trialIndex = QStringLiteral("idx_Messages_feed_display");
static void diskGuard(const QString &path) {
  QStorageInfo disk(QFileInfo(path).absolutePath());
  if (disk.bytesAvailable() < 1000000000LL) throw std::runtime_error("Trial requires at least 1GB free disk");
}

static QJsonObject fingerprint(const QSqlDatabase &db, const QString &sql) {
  QSqlQuery q(db); q.setForwardOnly(true); checked(q,sql);
  const int columns = q.record().count();
  QCryptographicHash hash(QCryptographicHash::Sha256);
  qint64 count=0;
  while(q.next()) {
    QByteArray bytes;
    QDataStream stream(&bytes,QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);
    stream << quint32(columns);
    for (int c=0;c<columns;++c) stream << q.value(c);
    hash.addData(bytes); ++count;
  }
  if(q.lastError().isValid()) throw std::runtime_error(q.lastError().text().toStdString());
  return {{"rows",count},{"columns",columns},{"ordered_all_values_sha256",QString::fromLatin1(hash.result().toHex())}};
}

static double median(QList<double> values) {
  std::sort(values.begin(),values.end());
  return values.size()%2 ? values[values.size()/2] : (values[values.size()/2-1]+values[values.size()/2])/2;
}

static QJsonObject statistics(const QJsonArray &samples) {
  QList<double> values;
  for(const auto &s:samples) values.append(s.toObject()["total_ms"].toDouble());
  return {{"samples",samples.size()},{"median_ms",median(values)},
          {"min_ms",*std::min_element(values.begin(),values.end())},
          {"max_ms",*std::max_element(values.begin(),values.end())}};
}

static QJsonArray writes(QSqlDatabase db, const QString &path, QTextStream &out) {
  // Match updateMessages' literal, 15-column, 1000-row bulk SQL (cpp:1229-1273).
  QSqlQuery source(db);
  checked(source,"SELECT feed,title,is_read,is_important,is_deleted,url,author,score,date_created,date_retrieved,contents,enclosures,custom_id,custom_data,account_id FROM Messages WHERE id=2000000;");
  if(!source.next()) throw std::runtime_error("Missing write seed");
  QList<QVariant> seed;
  for(int col=0;col<15;++col) seed.append(source.value(col));
  source.finish();
  QStringList rowSql;
  for(int row=0;row<1000;++row) {
    QStringList values;
    for(int col=0;col<15;++col) {
      QString value=seed[col].toString();
      if(col==1) value=QString("Trial insert %1").arg(row);
      if(col==12) value=QString("trial-only-%1").arg(row);
      if(col==0 || col==2 || col==3 || col==4 || col==7 || col==8 || col==9 || col==14)
        values.append(value);
      else { value.replace('\'',"''"); values.append('\''+value+'\''); }
    }
    rowSql.append("("+values.join(',')+")");
  }
  const QString sql="INSERT INTO Messages (feed,title,is_read,is_important,is_deleted,url,author,score,date_created,date_retrieved,contents,enclosures,custom_id,custom_data,account_id) VALUES "+rowSql.join(',')+';';
  QJsonArray result;
  QSqlQuery control(db);
  for(int run=0;run<5;++run) {
    diskGuard(path); checked(control,"SAVEPOINT trial_write;");
    QSqlQuery q(db); QElapsedTimer timer; timer.start();
    const double prepared=0;
    if(!q.exec(sql)) throw std::runtime_error(q.lastError().text().toStdString());
    const double total=double(timer.nsecsElapsed())/1e6;
    const auto affected=q.numRowsAffected(); q.finish();
    checked(control,"ROLLBACK TO trial_write;"); checked(control,"RELEASE trial_write;");
    diskGuard(path);
    if(run>0) result.append(QJsonObject{{"sample",run},{"total_ms",total},{"prepare_bind_ms",prepared},
                                      {"engine_exec_ms",total-prepared},{"rows",affected}});
    out << "  bulk insert " << run << ": " << total << " ms; " << affected << " rows (rolled back)" << Qt::endl;
  }
  return result;
}

static QString fixtureSmallSql(const QString &fullSql) {
  QString sql=fullSql; sql.replace("Messages.feed = 1 AND","Messages.feed = 750 AND"); return sql;
}

int main(int argc,char **argv) {
  QCoreApplication app(argc,argv); QTextStream out(stdout);
  if(app.arguments().size()!=4) { out << "Expected DATABASE ORIGINAL_RESULTS.json TRIAL_RESULTS.json\n"; return 2; }
  const QString path=app.arguments()[1], oldPath=app.arguments()[2], output=app.arguments()[3];
  QJsonObject report{{"status","running"},{"lazy_loading_changed",false},{"limit_added",false},
                     {"qt_version",QString::fromLatin1(qVersion())},
                     {"measurement","Same full-row/value timing as original audit; no object/UI conversion. Hash validation untimed. OS cache not flushed."}};
  QSqlDatabase db;
  bool transaction=false;
  try {
    diskGuard(path);
    QFile meta(QFileInfo(path).dir().filePath("fixture_metadata.json"));
    if(!meta.open(QIODevice::ReadOnly)) throw std::runtime_error("Fixture metadata required");
    const auto metadata=QJsonDocument::fromJson(meta.readAll()).object();
    if(metadata["status"].toString()!="complete" || metadata["schema_version"].toString()!="105"
       || !metadata["canonical_schema_matches"].toBool()
       || metadata["counts"].toObject()["Messages"].toInteger()!=2000000
       || QFileInfo(metadata["database"].toString()).canonicalFilePath()!=QFileInfo(path).canonicalFilePath())
      throw std::runtime_error("Not the validated isolated synthetic fixture");
    QFile old(oldPath); if(!old.open(QIODevice::ReadOnly)) throw std::runtime_error("Original results required");
    const auto oldReport=QJsonDocument::fromJson(old.readAll()).object();
    QJsonObject test=oldReport["results"].toArray().last().toObject();
    if(test["id"].toString()!="O052/hot-feed-default-all") throw std::runtime_error("Wrong finding");
    const QString sql=test["sql"].toString();
    if(sql.contains("LIMIT",Qt::CaseInsensitive)) throw std::runtime_error("Full result query required");
    report["original_sql"]=sql; report["original_source"]=test["source"];
    report["database"]=path; report["index_sql"]="CREATE INDEX "+trialIndex+" ON Messages (feed,date_created DESC,title ASC,id DESC) WHERE is_deleted=0 AND is_pdeleted=0;";
    db=QSqlDatabase::addDatabase("QSQLITE","index_trial"); db.setDatabaseName(path);
    db.setConnectOptions("QSQLITE_ENABLE_REGEXP");
    if(!db.open()) throw std::runtime_error(db.lastError().text().toStdString());
    configure(db);
    QSqlQuery q(db); checked(q,"SELECT sqlite_version();"); q.next(); report["sqlite_version"]=q.value(0).toString(); q.finish();
    checked(q,"SELECT COUNT(*) FROM sqlite_master WHERE name='"+trialIndex+"';"); q.next();
    if(q.value(0).toInt()!=0) throw std::runtime_error("Trial index already exists"); q.finish();
    checked(q,"PRAGMA page_count;"); q.next(); const qint64 beforePages=q.value(0).toLongLong(); q.finish();
    checked(q,"PRAGMA freelist_count;"); q.next(); const qint64 beforeFree=q.value(0).toLongLong(); q.finish();
    if(!db.transaction()) throw std::runtime_error("Cannot start isolated trial"); transaction=true;
    report["baseline_plan"]=explain(db,test);
    out << "Original full-result baseline" << Qt::endl;
    QJsonArray pre;
    for(int run=0;run<4;++run) { auto s=sample(db,test,run); pre.append(s); out << "  " << run << ": " << s["total_ms"].toDouble() << " ms" << Qt::endl; }
    report["before_index_samples"]=pre;
    report["original_fingerprint"]=fingerprint(db,sql);
    out << "Baseline bulk insert cost" << Qt::endl;
    report["baseline_write_samples"]=writes(db,path,out);
    // Tiny ties and membership fixtures live in a different feed; all are rolled back.
    checked(q,"SAVEPOINT trial_edge_cases;");
    for(int i=0;i<8;++i) {
      const QString title = i%2 ? "alpha" : "Alpha";
      checked(q,QString("INSERT INTO Messages (id,is_read,is_important,is_deleted,is_pdeleted,feed,title,url,author,date_created,contents,enclosures,score,account_id,custom_id,custom_data,date_retrieved) SELECT %1,%2,is_important,%3,%4,750,'%5',NULL,'',1791936000000,contents,enclosures,score,1,'trial-edge-%1',NULL,1791936000000 FROM Messages WHERE id=2000000;")
                   .arg(2100001+i).arg(i%2).arg(i<4?0:(i-4)/2).arg(i<4?0:(i-4)%2).arg(title));
    }
    checked(q,"INSERT INTO LabelsInMessages(message,label) VALUES(2100001,1),(2100001,2),(2100002,1);");
    const QString smallSql=fixtureSmallSql(sql);
    report["edge_baseline_fingerprint"]=fingerprint(db,smallSql);
    diskGuard(path);
    out << "Creating trial index inside transaction" << Qt::endl;
    QElapsedTimer buildTimer; buildTimer.start(); checked(q,report["index_sql"].toString());
    report["index_build_ms"]=double(buildTimer.nsecsElapsed())/1e6; diskGuard(path);
    checked(q,"PRAGMA page_count;"); q.next(); const qint64 afterPages=q.value(0).toLongLong(); q.finish();
    checked(q,"PRAGMA freelist_count;"); q.next(); const qint64 afterFree=q.value(0).toLongLong(); q.finish();
    report["index_allocated_bytes"]=(afterPages-afterFree-beforePages+beforeFree)*32768;
    report["indexed_plan"]=explain(db,test);
    report["indexed_fingerprint"]=fingerprint(db,sql);
    report["edge_indexed_fingerprint"]=fingerprint(db,smallSql);
    if(report["original_fingerprint"]!=report["indexed_fingerprint"]
       || report["edge_baseline_fingerprint"]!=report["edge_indexed_fingerprint"])
      throw std::runtime_error("Ordered full-value fingerprint mismatch");
    report["equivalence_passed"]=true;
    // Replay the verified original planner choice while the candidate index exists,
    // so interleaved measurements share identical data, connection and OS cache.
    QJsonObject baseline=test;
    QString baselineSql=sql; baselineSql.replace(" FROM Messages WHERE "," FROM Messages INDEXED BY idx_Messages3 WHERE ");
    baseline["sql"]=baselineSql;
    report["interleaved_baseline_sql"]=baselineSql;
    report["interleaved_baseline_plan"]=explain(db,baseline);
    if(fingerprint(db,baselineSql)!=report["original_fingerprint"])
      throw std::runtime_error("Original-plan replay changed results");
    out << "Interleaved full-result A/B (no LIMIT)" << Qt::endl;
    sample(db,baseline,0); sample(db,test,0);
    QJsonArray oldSamples,newSamples;
    for(int pair=0;pair<6;++pair) {
      for(int slot=0;slot<2;++slot) {
        const bool indexed=(pair%2==0)?slot==1:slot==0;
        const auto timed=sample(db,indexed?test:baseline,pair+1);
        if(indexed) newSamples.append(timed); else oldSamples.append(timed);
        out << "  pair " << pair << (indexed?" index: ":" original plan: ") << timed["total_ms"].toDouble() << " ms" << Qt::endl;
      }
      report["interleaved_baseline_samples"]=oldSamples; report["interleaved_indexed_samples"]=newSamples;
      writeJson(output,report);
    }
    report["baseline_stats"]=statistics(oldSamples); report["indexed_stats"]=statistics(newSamples);
    out << "Indexed bulk insert cost" << Qt::endl;
    report["indexed_write_samples"]=writes(db,path,out);
    report["baseline_write_stats"]=statistics(report["baseline_write_samples"].toArray());
    report["indexed_write_stats"]=statistics(report["indexed_write_samples"].toArray());
    report["write_control"]="Actual application shape: one literal 1000-row INSERT, same 15 columns; SQL exec timed, generation/commit/rollback excluded. Savepoint rollback preserves fixture.";
    // Status/order changes exercise maintenance; compare the original plan with automatic index.
    checked(q,"UPDATE Messages SET title='Beta',date_created=1791936000001,is_deleted=0,is_pdeleted=0 WHERE id=2100007;");
    checked(q,"UPDATE Messages SET is_deleted=1 WHERE id=2100002;");
    QString smallOriginal=smallSql; smallOriginal.replace(" FROM Messages WHERE "," FROM Messages INDEXED BY idx_Messages3 WHERE ");
    report["edge_updates_equivalent"]=(fingerprint(db,smallOriginal)==fingerprint(db,smallSql));
    if(!report["edge_updates_equivalent"].toBool()) throw std::runtime_error("Edge updates changed results");
    q.finish(); if(!db.rollback()) throw std::runtime_error("Rollback failed"); transaction=false;
    checked(q,"PRAGMA wal_checkpoint(TRUNCATE);"); q.finish();
    checked(q,"SELECT COUNT(*) FROM sqlite_master WHERE name='"+trialIndex+"';"); q.next();
    report["trial_index_removed"]=q.value(0).toInt()==0; q.finish();
    checked(q,"SELECT COUNT(*) FROM Messages WHERE id>=2100001;"); q.next();
    report["trial_rows_removed"]=q.value(0).toInt()==0; q.finish();
    report["baseline_restored_fingerprint"]=fingerprint(db,sql);
    report["baseline_restored"]=(report["baseline_restored_fingerprint"]==report["original_fingerprint"]);
    if(!report["trial_index_removed"].toBool() || !report["trial_rows_removed"].toBool() || !report["baseline_restored"].toBool())
      throw std::runtime_error("Fixture restoration check failed");
    report["status"]="complete_rolled_back"; writeJson(output,report); db.close();
    out << "Trial complete; full results equal; index and trial data rolled back." << Qt::endl;
    return 0;
  } catch(const std::exception &e) {
    if(transaction) { db.rollback(); QSqlQuery q(db); q.exec("PRAGMA wal_checkpoint(TRUNCATE);"); }
    report["status"]="error"; report["error"]=QString::fromUtf8(e.what()); writeJson(output,report);
    out << "ERROR: " << e.what() << Qt::endl; if(db.isOpen()) db.close(); return 1;
  }
}
