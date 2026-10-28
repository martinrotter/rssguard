// For license of this file, see <project-root-folder>/LICENSE.md.

#include "database/databasequeries.h"
#include "filtering/messagesforfiltersmodel.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/settingskeys.h"
#include "miscellaneous/skinfactory.h"
#include "services/abstract/labelsnode.h"

#include <memory>

#include <QDir>
#include <QSettings>
#include <QSignalSpy>
#include <QSortFilterProxyModel>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeView>

namespace {

  class TestServiceRoot : public ServiceRoot {
    public:
      QString code() const override {
        return QSL("filtering-results-test");
      }

      QList<Message> obtainNewMessages(Feed*,
                                       const QHash<BagOfMessages, QStringList>&,
                                       const QHash<QString, QStringList>&) override {
        return {};
      }
  };

  void setScript(MessageFilter& filter, const QString& body) {
    filter.setName(QSL("Result test filter"));
    filter.setScript(QSL("function filterMessage() { %1; return Msg.Accept; }").arg(body));
  }

  int rowForId(const MessagesForFiltersModel& model, int id) {
    for (int row = 0; row < model.rowCount({}); ++row) {
      if (model.resultForRow(row)->m_original.m_id == id) {
        return row;
      }
    }
    return -1;
  }

  void prepareProfile(const QString& profile) {
    QDir().mkpath(profile + QDir::separator() + QSL(APP_CFG_PATH));
    QSettings settings(profile + QDir::separator() + QSL(APP_CFG_PATH) + QDir::separator() + QSL(APP_CFG_FILE),
                       QSettings::IniFormat);
    settings.setValue(GROUP(General) + QSL("/") + General::ShowSplashScreen, false);
    settings.setValue(GROUP(General) + QSL("/") + General::UpdateOnStartup, false);
    settings.setValue(GROUP(GUI) + QSL("/") + GUI::UseToastNotifications, false);
    settings.sync();
  }

} // namespace

class TestFilteringResults : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void init();
    void previewShowsEffectiveValues_data();
    void previewShowsEffectiveValues();
    void enclosureSnapshotsRemainIndependent();
    void sortingTracksFilteredValues_data();
    void sortingTracksFilteredValues();
    void repeatedTestsKeepSnapshotsIndependent_data();
    void repeatedTestsKeepSnapshotsIndependent();
    void expectedBodiesKeepFreshValues_data();
    void expectedBodiesKeepFreshValues();
    void mixedFeedsUseTheirOwnContextAndForwardLogs();
    void processingRetainsAcceptedIgnoredAndPurgedResults();
    void processingFailureRetainsEarlierFeedResults();
    void labelsOnlyChangesAndNoOpsAreReported();
    void cleanupIsReportedAfterWriting();
    void purgePreviewHasNoStoredDetails();
    void failedReloadClearsPreviousResults();
    void reinsertedArticleUsesReceiptIdentity();

  private:
    Message seedArticle(Feed* feed, const QString& id, bool important = false);
    Message storedArticle(int id) const;

    std::unique_ptr<TestServiceRoot> m_account;
    Feed* m_feed = nullptr;
    Feed* m_secondFeed = nullptr;
    Label* m_label = nullptr;
};

void TestFilteringResults::initTestCase() {
  m_account = std::make_unique<TestServiceRoot>();
  m_account->setTitle(QSL("Filter results account"));
  m_account->setSortOrder(0);
  m_feed = new Feed(m_account.get());
  m_feed->setTitle(QSL("First feed"));
  m_feed->setCustomId(QSL("first-feed"));
  m_feed->setSortOrder(0);
  m_account->appendChild(m_feed);
  m_secondFeed = new Feed(m_account.get());
  m_secondFeed->setTitle(QSL("Second feed"));
  m_secondFeed->setCustomId(QSL("second-feed"));
  m_secondFeed->setSortOrder(1);
  m_account->appendChild(m_secondFeed);
  m_label = new Label(QSL("Filter label"), {}, m_account->labelsNode());
  m_label->setCustomId(QSL("filter-label"));
  m_account->labelsNode()->appendChild(m_label);
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    DatabaseQueries::createOverwriteAccount(db, m_account.get());
    DatabaseQueries::createOverwriteFeed(db, m_feed, m_account->accountId(), NO_PARENT_CATEGORY);
    DatabaseQueries::createOverwriteFeed(db, m_secondFeed, m_account->accountId(), NO_PARENT_CATEGORY);
    DatabaseQueries::createLabel(qApp->icons(), db, m_label, m_account->accountId());
  });
}

void TestFilteringResults::init() {
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("DELETE FROM Messages;"));
  });
  qApp->settings()->setValue(GROUP(Messages), Messages::IgnoreContentsChanges, true);
  qApp->settings()->setValue(GROUP(Messages), Messages::MarkUnreadOnUpdated, false);
  Feed::ArticleIgnoreLimit unlimited;
  unlimited.m_customizeLimitting = true;
  m_feed->setArticleIgnoreLimit(unlimited);
  m_secondFeed->setArticleIgnoreLimit(unlimited);
  m_account->labelsNode()->updateCounts();
}

Message TestFilteringResults::seedArticle(Feed* feed, const QString& id, bool important) {
  Message message;
  message.m_title = id;
  message.m_customId = id;
  message.m_feedId = feed->id();
  message.m_accountId = m_account->accountId();
  message.m_contents = QSL("Original contents");
  message.m_created = QDateTime::fromMSecsSinceEpoch(1700000000000LL, Qt::UTC);
  message.m_retrieved = message.m_created;
  message.m_isImportant = important;
  message.m_enclosures.append(QSharedPointer<MessageEnclosure>::create(QSL("https://example.com/original.mp3"),
                                                                       QSL("audio/mpeg")));
  QList<Message> batch{message};
  DatabaseQueries::updateMessages(qApp->database(), qApp->settings(), batch, feed, false, true).throwIfFailed();
  return batch.first();
}

Message TestFilteringResults::storedArticle(int id) const {
  return qApp->database()->worker()->read<Message>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT %1 FROM Messages WHERE id = :id;")
                    .arg(DatabaseQueries::messageTableAttributes().join(QSL(", "))));
    query.bindValue(QSL(":id"), id);
    query.exec();
    return query.next() ? Message::fromSqlQuery(query, m_account->labelsNode()->getHashedLabels()) : Message();
  });
}

void TestFilteringResults::previewShowsEffectiveValues_data() {
  QTest::addColumn<bool>("title_change");
  QTest::addColumn<bool>("overwrite");
  for (bool title_change : {false, true}) {
    for (bool overwrite : {false, true}) {
      QTest::newRow(qPrintable(QSL("title%1-overwrite%2").arg(title_change).arg(overwrite)))
        << title_change << overwrite;
    }
  }
}

void TestFilteringResults::previewShowsEffectiveValues() {
  QFETCH(bool, title_change);
  QFETCH(bool, overwrite);
  const Message original = seedArticle(m_feed, QSL("preview-values"), true);
  MessagesForFiltersModel model(qApp);
  model.setMessages({original});
  QVERIFY(!model.index(0, MFM_MODEL_DATABASE).data(Qt::BackgroundRole).isValid());
  MessageFilter filter;
  setScript(filter,
            QSL("msg.isRead = true; msg.isImportant = false; msg.score = 17; %1 msg.overwrite = %2;")
              .arg(title_change ? QSL("msg.title = 'Changed title';") : QString(),
                   overwrite ? QSL("true") : QSL("false")));
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QVERIFY(model.isPreview());
  const auto* row = model.resultForRow(0);
  QVERIFY(row != nullptr);
  QCOMPARE(row->m_decision.value(), FilterMessage::FilteringAction::Accept);
  QVERIFY(row->m_saveResult.has_value());
  QCOMPARE(row->m_saveResult->m_decision.m_action,
           title_change || overwrite ? MessageSaveAction::Update : MessageSaveAction::Skip);
  model.setShowStoredValues(false);
  QVERIFY(model.constMessageForRow(0)->m_isRead);
  QVERIFY(!model.constMessageForRow(0)->m_isImportant);
  QCOMPARE(model.constMessageForRow(0)->m_score, 17.0);
  model.setShowStoredValues(true);
  const Message* expected = model.constMessageForRow(0);
  QCOMPARE(expected->m_isRead, overwrite);
  QCOMPARE(expected->m_isImportant, !overwrite);
  QCOMPARE(expected->m_score, overwrite || title_change ? 17.0 : 0.0);
  QCOMPARE(expected->m_title, title_change ? QSL("Changed title") : original.m_title);
  QVERIFY(!model.data(model.index(0, MFM_MODEL_DATABASE), Qt::DisplayRole).toString().isEmpty());
  QVERIFY(!model.summaryText().isEmpty());
  QCOMPARE(model.index(0, MFM_MODEL_DATABASE).data(Qt::BackgroundRole).value<QColor>(),
           qApp->skins()->colorForModel(title_change || overwrite ? SkinEnums::PaletteColors::Allright
                                                                  : SkinEnums::PaletteColors::FgError).value<QColor>());
  const Message stored = storedArticle(original.m_id);
  QVERIFY(!stored.m_isRead);
  QVERIFY(stored.m_isImportant);
  QCOMPARE(stored.m_score, 0.0);
  QCOMPARE(stored.m_title, original.m_title);
}

void TestFilteringResults::sortingTracksFilteredValues_data() {
  QTest::addColumn<int>("column");
  QTest::addColumn<bool>("descending");
  for (int column : {MFM_MODEL_ISREAD, MFM_MODEL_ISIMPORTANT, MFM_MODEL_ISDELETED, MFM_MODEL_SCORE}) {
    for (bool descending : {false, true}) {
      QTest::newRow(qPrintable(QSL("column%1-descending%2").arg(column).arg(descending))) << column << descending;
    }
  }
}

void TestFilteringResults::sortingTracksFilteredValues() {
  QFETCH(int, column);
  QFETCH(bool, descending);
  QList<Message> messages;
  const QList<int> scores{2, 10, 1, 3};
  for (int i = 0; i < scores.size(); ++i) {
    Message message = seedArticle(m_feed, QSL("sorting-%1").arg(i), i % 2 != 0);
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.prepare(QSL("UPDATE Messages SET is_read = :state, is_important = :state, is_deleted = :state, "
                         "score = :score WHERE id = :id;"));
      query.bindValue(QSL(":state"), i % 2);
      query.bindValue(QSL(":score"), scores.at(i));
      query.bindValue(QSL(":id"), message.m_id);
      query.exec();
    });
    messages.append(storedArticle(message.m_id));
  }

  MessagesForFiltersModel model(qApp);
  model.setMessages(messages);
  QSortFilterProxyModel proxy;
  proxy.setSortCaseSensitivity(Qt::CaseInsensitive);
  proxy.setSortRole(MessagesForFiltersModel::SortRole);
  proxy.setSourceModel(&model);
  QTreeView view;
  view.setSortingEnabled(true);
  view.setModel(&proxy);
  view.sortByColumn(column, descending ? Qt::DescendingOrder : Qt::AscendingOrder);

  const auto verify_order = [&](bool filtered) {
    QList<QVariant> expected;
    if (column == MFM_MODEL_SCORE) {
      for (double value : filtered ? QList<double>{90, 97, 98, 99} : QList<double>{1, 2, 3, 10}) {
        expected.append(value);
      }
    }
    else {
      expected = {false, false, true, true};
    }
    for (int row = 0; row < expected.size(); ++row) {
      const QModelIndex visible = view.model()->index(row, column);
      const QVariant actual = visible.data(MessagesForFiltersModel::SortRole);
      QCOMPARE(actual, expected.at(descending ? expected.size() - row - 1 : row));
      const int source_row = proxy.mapToSource(proxy.index(row, column)).row();
      if (column != MFM_MODEL_SCORE) {
        const bool original = source_row % 2 != 0;
        const bool displayed = filtered ? !original : original;
        QCOMPARE(actual.toBool(), displayed);
        QCOMPARE(visible.data(Qt::DisplayRole).toString(), MessagesForFiltersModel::tr(displayed ? "true" : "false"));
      }
    }
  };
  verify_order(false);

  MessageFilter filter;
  setScript(filter,
            QSL("msg.isRead = !msg.isRead; msg.isImportant = !msg.isImportant; "
                "msg.isDeleted = !msg.isDeleted; msg.score = 100 - msg.score;"));
  for (int run = 0; run < 2; ++run) {
    model.testFilter(&filter, m_account.get());
    QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
    verify_order(true);
    model.setShowStoredValues(true);
    verify_order(false);
    model.setShowStoredValues(false);
    verify_order(true);
  }
}

void TestFilteringResults::enclosureSnapshotsRemainIndependent() {
  const Message original = seedArticle(m_feed, QSL("attachment-snapshot"));
  Message snapshot = MessagesForFiltersModel::snapshotForFiltering(original);
  snapshot.m_enclosures.first()->setMimeType(QSL("audio/ogg"));
  QCOMPARE(original.m_enclosures.first()->mimeType(), QSL("audio/mpeg"));

  MessagesForFiltersModel model(qApp);
  model.setMessages({original});
  MessageFilter filter;
  setScript(filter,
            QSL("msg.enclosures[0].url = 'https://example.com/changed.ogg';"
                "msg.enclosures[0].mimeType = 'audio/ogg';"));
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  model.setShowStoredValues(false);
  QCOMPARE(model.constMessageForRow(0)->m_enclosures.first()->mimeType(), QSL("audio/ogg"));
  model.setShowStoredValues(true);
  QCOMPARE(model.constMessageForRow(0)->m_enclosures.first()->mimeType(), QSL("audio/mpeg"));
  QCOMPARE(model.resultForRow(0)->m_original.m_enclosures.first()->mimeType(), QSL("audio/mpeg"));
  QCOMPARE(original.m_enclosures.first()->url(), QSL("https://example.com/original.mp3"));
  setScript(filter, {});
  model.testFilter(&filter, m_account.get());
  model.setShowStoredValues(false);
  QCOMPARE(model.constMessageForRow(0)->m_enclosures.first()->mimeType(), QSL("audio/mpeg"));
  QCOMPARE(storedArticle(original.m_id).m_enclosures.first()->mimeType(), QSL("audio/mpeg"));
}

void TestFilteringResults::repeatedTestsKeepSnapshotsIndependent_data() {
  QTest::addColumn<bool>("compact");
  QTest::addColumn<bool>("overwrite");
  for (bool compact : {false, true}) {
    for (bool overwrite : {false, true}) {
      QTest::newRow(qPrintable(QSL("compact%1-overwrite%2").arg(compact).arg(overwrite))) << compact << overwrite;
    }
  }
}

void TestFilteringResults::repeatedTestsKeepSnapshotsIndependent() {
  QFETCH(bool, compact);
  QFETCH(bool, overwrite);
  Message original = seedArticle(m_feed, QSL("repeated-snapshot"));
  MessagesForFiltersModel model(qApp);
  MessageFilter filter;
  if (compact) {
    // Retained Process results deliberately omit bodies and enclosures. Test must
    // reload the saved payload before creating its independent snapshots.
    setScript(filter,
              QSL("msg.contents = 'Processed contents';"
                  "msg.enclosures[0].url = 'https://example.com/processed.mp3';"
                  "msg.overwrite = true;"));
    model.processFeeds(&filter, m_account.get(), {m_feed});
    QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
    QCOMPARE(model.rowCount({}), 1);
    QVERIFY(model.resultForRow(0)->m_compact);
    QVERIFY(model.resultForRow(0)->m_original.m_contents.isEmpty());
    QVERIFY(model.resultForRow(0)->m_original.m_enclosures.isEmpty());
    original = storedArticle(original.m_id);
  }
  else {
    model.setMessages({original});
  }

  const QString original_url = original.m_enclosures.first()->url();
  const QString filtered_url = original_url + QSL("?test");
  setScript(filter,
            QSL("msg.contents += ' tested';"
                "msg.enclosures[0].url += '?test';"
                "msg.enclosures[0].mimeType = 'audio/ogg';"
                "msg.overwrite = %1;")
              .arg(overwrite ? QSL("true") : QSL("false")));

  for (int pass = 0; pass < 2; ++pass) {
    model.testFilter(&filter, m_account.get());
    QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
    const auto* row = model.resultForRow(0);
    QVERIFY(row != nullptr);
    QVERIFY(!row->m_compact);
    QVERIFY(row->m_saveResult.has_value());
    QCOMPARE(row->m_original.m_enclosures.size(), 1);
    QCOMPARE(row->m_filtered.m_enclosures.size(), 1);
    QCOMPARE(row->m_expectedStored.m_enclosures.size(), 1);
    QCOMPARE(row->m_saveResult->m_decision.m_action,
             overwrite ? MessageSaveAction::Update : MessageSaveAction::Skip);
    QCOMPARE(row->m_original.m_contents, original.m_contents);
    QCOMPARE(row->m_original.m_enclosures.first()->url(), original_url);
    QCOMPARE(row->m_original.m_enclosures.first()->mimeType(), QSL("audio/mpeg"));
    QCOMPARE(row->m_filtered.m_contents, original.m_contents + QSL(" tested"));
    QCOMPARE(row->m_filtered.m_enclosures.first()->url(), filtered_url);
    QCOMPARE(row->m_filtered.m_enclosures.first()->mimeType(), QSL("audio/ogg"));
    QCOMPARE(row->m_expectedStored.m_contents,
             overwrite ? original.m_contents + QSL(" tested") : original.m_contents);
    QCOMPARE(row->m_expectedStored.m_enclosures.first()->url(), overwrite ? filtered_url : original_url);
    QCOMPARE(row->m_expectedStored.m_enclosures.first()->mimeType(),
             overwrite ? QSL("audio/ogg") : QSL("audio/mpeg"));

    // Even an expected full overwrite must not share mutable enclosure objects
    // with the script output or the immutable input used by the next Test.
    row->m_expectedStored.m_enclosures.first()->setUrl(QSL("https://example.com/expected-only"));
    QCOMPARE(row->m_filtered.m_enclosures.first()->url(), filtered_url);
    QCOMPARE(row->m_original.m_enclosures.first()->url(), original_url);
    QCOMPARE(storedArticle(original.m_id).m_contents, original.m_contents);
    QCOMPARE(storedArticle(original.m_id).m_enclosures.first()->url(), original_url);
  }
}

void TestFilteringResults::expectedBodiesKeepFreshValues_data() {
  QTest::addColumn<QString>("loaded_body");
  QTest::addColumn<QString>("stored_body");
  QTest::newRow("equal-text") << QSL("Same body") << QSL("Same body");
  QTest::newRow("changed-text") << QSL("Loaded body") << QSL("New database body");
  QTest::newRow("changed-to-empty") << QSL("Loaded body") << QSL("");
  QTest::newRow("null-to-empty") << QString() << QSL("");
  QTest::newRow("empty-to-null") << QSL("") << QString();
  QTest::newRow("equal-null") << QString() << QString();
}

void TestFilteringResults::expectedBodiesKeepFreshValues() {
  QFETCH(QString, loaded_body);
  QFETCH(QString, stored_body);
  const int id = seedArticle(m_feed, QSL("fresh-preview-body")).m_id;
  const auto store_body = [id](const QString& body) {
    qApp->database()->worker()->write([&](const QSqlDatabase& db) {
      SqlQuery query(db);
      query.prepare(QSL("UPDATE Messages SET contents = :contents WHERE id = :id;"));
      query.bindValue(QSL(":contents"), body);
      query.bindValue(QSL(":id"), id);
      query.exec();
    });
  };
  store_body(loaded_body);
  const Message original = storedArticle(id);
  MessagesForFiltersModel model(qApp);
  model.setMessages({original});

  // The expected values must come from the current database even though the
  // filter continues to start from its originally loaded input snapshot.
  store_body(stored_body);
  const Message stored = storedArticle(id);
  qApp->settings()->setValue(GROUP(Messages), Messages::IgnoreContentsChanges, true);
  MessageFilter filter;
  setScript(filter, {});

  for (int pass = 0; pass < 2; ++pass) {
    model.testFilter(&filter, m_account.get());
    QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
    const auto* row = model.resultForRow(0);
    QVERIFY(row != nullptr);
    QVERIFY(row->m_saveResult.has_value());
    QCOMPARE(row->m_saveResult->m_decision.m_action, MessageSaveAction::Skip);
    QCOMPARE(row->m_expectedStored.m_contents, stored.m_contents);
    QCOMPARE(row->m_expectedStored.m_contents.isNull(), stored.m_contents.isNull());

    // Equal QString values may share storage, but modifying the displayed
    // prediction must remain isolated from the input, script output and DB.
    model.setShowStoredValues(true);
    model.messageForRow(0)->m_contents += QSL(" display-only");
    QCOMPARE(row->m_original.m_contents, original.m_contents);
    QCOMPARE(row->m_original.m_contents.isNull(), original.m_contents.isNull());
    QCOMPARE(row->m_filtered.m_contents, original.m_contents);
    QCOMPARE(row->m_filtered.m_contents.isNull(), original.m_contents.isNull());
    const Message unchanged = storedArticle(id);
    QCOMPARE(unchanged.m_contents, stored.m_contents);
    QCOMPARE(unchanged.m_contents.isNull(), stored.m_contents.isNull());
  }
}

void TestFilteringResults::mixedFeedsUseTheirOwnContextAndForwardLogs() {
  const Message first = seedArticle(m_feed, QSL("first-context"));
  const Message second = seedArticle(m_secondFeed, QSL("second-context"));
  MessagesForFiltersModel model(qApp);
  model.setMessages({second, first});
  QSignalSpy logs(&model, &MessagesForFiltersModel::scriptLogged);
  MessageFilter filter;
  setScript(filter, QSL("msg.title = feed.title + ':' + run.numberOfAcceptedMessages; app.log(feed.customId);"));
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  model.setShowStoredValues(false);
  QCOMPARE(model.constMessageForRow(rowForId(model, first.m_id))->m_title, QSL("First feed:0"));
  QCOMPARE(model.constMessageForRow(rowForId(model, second.m_id))->m_title, QSL("Second feed:0"));
  QCOMPARE(logs.count(), 2);
  QStringList logged;
  for (const auto& log : logs) {
    logged.append(log.first().toString());
  }
  QVERIFY(logged.contains(m_feed->customId()));
  QVERIFY(logged.contains(m_secondFeed->customId()));
}

void TestFilteringResults::processingRetainsAcceptedIgnoredAndPurgedResults() {
  const Message accepted = seedArticle(m_feed, QSL("accept"));
  const Message ignored = seedArticle(m_feed, QSL("ignore"));
  const Message purged = seedArticle(m_feed, QSL("purge"));
  MessagesForFiltersModel model(qApp);
  QSignalSpy logs(&model, &MessagesForFiltersModel::scriptLogged);
  MessageFilter filter;
  setScript(filter,
            QSL("app.log(msg.title); if (msg.title === 'ignore') return Msg.Ignore;"
                "if (msg.title === 'purge') return Msg.Purge; msg.isRead = true; msg.overwrite = true;"));
  model.processFeeds(&filter, m_account.get(), {m_feed});
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QVERIFY(!model.isPreview());
  QCOMPARE(model.rowCount({}), 3);
  QVERIFY(rowForId(model, accepted.m_id) >= 0);
  QVERIFY(rowForId(model, ignored.m_id) >= 0);
  QVERIFY(rowForId(model, purged.m_id) >= 0);
  const auto* accepted_row = model.resultForRow(rowForId(model, accepted.m_id));
  QVERIFY(accepted_row->m_saveResult.has_value());
  QCOMPARE(accepted_row->m_saveResult->m_outcome, MessageWriteOutcome::Updated);
  QCOMPARE(model.resultForRow(rowForId(model, ignored.m_id))->m_decision.value(),
           FilterMessage::FilteringAction::Ignore);
  QVERIFY(model.resultForRow(rowForId(model, purged.m_id))->m_purged);
  for (const Message& article : {accepted, ignored, purged}) {
    const auto expected_color = article.m_id == accepted.m_id ? SkinEnums::PaletteColors::Allright
                                                            : SkinEnums::PaletteColors::FgError;
    QCOMPARE(model.index(rowForId(model, article.m_id), MFM_MODEL_DATABASE).data(Qt::BackgroundRole).value<QColor>(),
             qApp->skins()->colorForModel(expected_color).value<QColor>());
  }
  QCOMPARE(logs.count(), 3);
  QVERIFY(!model.summaryText().isEmpty());
  QVERIFY(storedArticle(accepted.m_id).m_isRead);
  QVERIFY(!storedArticle(ignored.m_id).m_isRead);
  QCOMPARE(storedArticle(purged.m_id).m_id, 0);
  QVERIFY(!model.data(model.index(rowForId(model, purged.m_id), MFM_MODEL_DATABASE), Qt::DisplayRole)
             .toString()
             .isEmpty());
}

void TestFilteringResults::processingFailureRetainsEarlierFeedResults() {
  const Message first = seedArticle(m_feed, QSL("completed-feed"));
  const Message second = seedArticle(m_secondFeed, QSL("failed-feed"));
  MessagesForFiltersModel model(qApp);
  MessageFilter filter;
  setScript(filter,
            QSL("if (feed.customId === 'second-feed') throw new Error('expected second-feed failure');"
                "msg.isRead = true; msg.overwrite = true;"));
  model.processFeeds(&filter, m_account.get(), {m_feed, m_secondFeed});
  QVERIFY(model.lastError().contains(QL1S("expected second-feed failure")));
  QVERIFY(!model.summaryText().isEmpty());
  QVERIFY(rowForId(model, first.m_id) >= 0);
  QVERIFY(rowForId(model, second.m_id) >= 0);
  const auto* completed = model.resultForRow(rowForId(model, first.m_id));
  QVERIFY(completed->m_saveResult.has_value());
  QCOMPARE(completed->m_saveResult->m_outcome, MessageWriteOutcome::Updated);
  QVERIFY(!model.resultForRow(rowForId(model, second.m_id))->m_error.isEmpty());
  QCOMPARE(model.index(rowForId(model, second.m_id), MFM_MODEL_DATABASE).data(Qt::BackgroundRole).value<QColor>(),
           qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError).value<QColor>());
  QVERIFY(storedArticle(first.m_id).m_isRead);
  QVERIFY(!storedArticle(second.m_id).m_isRead);
}

void TestFilteringResults::labelsOnlyChangesAndNoOpsAreReported() {
  const Message original = seedArticle(m_feed, QSL("label-only"));
  MessagesForFiltersModel model(qApp);
  model.setMessages({original});
  MessageFilter filter;
  setScript(filter, QSL("msg.assignLabel('filter-label');"));
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QVERIFY(model.resultForRow(0)->m_saveResult.has_value());
  QCOMPARE(model.resultForRow(0)->m_saveResult->m_decision.m_action, MessageSaveAction::Skip);
  QVERIFY(model.resultForRow(0)->m_saveResult->m_labelsChanged);
  QCOMPARE(model.index(0, MFM_MODEL_DATABASE).data(Qt::BackgroundRole).value<QColor>(),
           qApp->skins()->colorForModel(SkinEnums::PaletteColors::Allright).value<QColor>());
  QList<Message> batch{original};
  batch.first().m_assignedLabels.append(m_label);
  batch.first().m_assignedLabelsByFilter.append(m_label);
  const auto preview = DatabaseQueries::previewMessageUpdates(qApp->database(), qApp->settings(), batch, m_feed);
  QCOMPARE(preview.first().m_decision.m_action, MessageSaveAction::Skip);
  QVERIFY(preview.first().m_labelsChanged);
  const auto report = m_account->updateMessages(batch, m_feed, true, MessageUpdateDetail::Detailed);
  QVERIFY2(report.succeeded(), qPrintable(report.m_error));
  QCOMPARE(report.m_results.first().m_outcome, MessageWriteOutcome::Skipped);
  QCOMPARE(report.labelChanges(), 1);
  QCOMPARE(m_label->countOfAllMessages(), 1);
  QCOMPARE(m_label->countOfUnreadMessages(), 1);

  const auto repeat_preview = DatabaseQueries::previewMessageUpdates(qApp->database(), qApp->settings(), batch, m_feed);
  QVERIFY(!repeat_preview.first().m_labelsChanged);
  const auto repeat = m_account->updateMessages(batch, m_feed, true, MessageUpdateDetail::Detailed);
  QVERIFY2(repeat.succeeded(), qPrintable(repeat.m_error));
  QCOMPARE(repeat.labelChanges(), 0);
  QVERIFY(repeat.m_results.first().m_labelsChecked);

  batch.first().m_assignedLabels.clear();
  batch.first().m_assignedLabelsByFilter.clear();
  batch.first().m_deassignedLabelsByFilter.append(m_label);
  const auto removed = m_account->updateMessages(batch, m_feed, true, MessageUpdateDetail::Detailed);
  QVERIFY2(removed.succeeded(), qPrintable(removed.m_error));
  QCOMPARE(removed.labelChanges(), 1);
  QCOMPARE(m_label->countOfAllMessages(), 0);
  QCOMPARE(m_label->countOfUnreadMessages(), 0);

  model.processFeeds(&filter, m_account.get(), {m_feed});
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QVERIFY(model.resultForRow(0)->m_saveResult.has_value());
  QCOMPARE(model.resultForRow(0)->m_saveResult->m_outcome, MessageWriteOutcome::Skipped);
  QVERIFY(model.resultForRow(0)->m_saveResult->m_labelsChanged);
  QCOMPARE(m_label->countOfAllMessages(), 1);
}

void TestFilteringResults::cleanupIsReportedAfterWriting() {
  Message older = seedArticle(m_feed, QSL("older"));
  Message newer = seedArticle(m_feed, QSL("newer"));
  older.m_created = older.m_created.addDays(-1);
  older.m_overwrite = newer.m_overwrite = true;
  QList<Message> batch{older, newer};
  Feed::ArticleIgnoreLimit limit;
  limit.m_customizeLimitting = true;
  limit.m_keepCountOfArticles = 1;
  limit.m_doNotRemoveStarred = false;
  limit.m_doNotRemoveUnread = false;
  limit.m_moveToBinDontPurge = true;
  m_feed->setArticleIgnoreLimit(limit);
  const auto report = m_account->updateMessages(batch, m_feed, true, MessageUpdateDetail::Detailed);
  QVERIFY2(report.succeeded(), qPrintable(report.m_error));
  QCOMPARE(report.m_results.size(), 2);
  QCOMPARE(report.count(MessageWriteOutcome::Updated), 2);
  QVERIFY(report.m_results.at(0).m_removedByCleanup);
  QVERIFY(!report.m_results.at(1).m_removedByCleanup);
  QCOMPARE(report.m_all.size(), 1);
  QCOMPARE(report.m_all.first().m_id, newer.m_id);
  QVERIFY(storedArticle(older.m_id).m_isDeleted);
  QVERIFY(!storedArticle(newer.m_id).m_isDeleted);
}

void TestFilteringResults::purgePreviewHasNoStoredDetails() {
  const Message original = seedArticle(m_feed, QSL("purge-preview"));
  MessagesForFiltersModel model(qApp);
  model.setMessages({original});
  MessageFilter filter;
  setScript(filter, QSL("return Msg.Purge;"));
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QCOMPARE(model.resultForRow(0)->m_decision.value(), FilterMessage::FilteringAction::Purge);
  QVERIFY(!model.hasStoredValuesForRow(0));
  model.setShowStoredValues(false);
  QCOMPARE(model.messageDetailsForRow(0).m_id, original.m_id);
  model.setShowStoredValues(true);
  QCOMPARE(model.messageDetailsForRow(0).m_id, 0);
  QVERIFY(model.messageDetailsForRow(0).m_contents.isEmpty());
  QCOMPARE(storedArticle(original.m_id).m_id, original.m_id);

  model.processFeeds(&filter, m_account.get(), {m_feed});
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QCOMPARE(rowForId(model, original.m_id), 0);
  QVERIFY(model.resultForRow(0)->m_purged);
  setScript(filter, {});
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QVERIFY(!model.resultForRow(0)->m_decision.has_value());
  QVERIFY(!model.resultForRow(0)->m_purged);
  QCOMPARE(model.resultForRow(0)->m_saveResult->m_decision.m_reason, MessageSaveReason::MissingArticle);
  model.setShowStoredValues(true);
  QVERIFY(!model.hasStoredValuesForRow(0));
  QCOMPARE(model.messageDetailsForRow(0).m_id, 0);
}

void TestFilteringResults::failedReloadClearsPreviousResults() {
  seedArticle(m_feed, QSL("reload-first"));
  seedArticle(m_feed, QSL("reload-second"));
  MessagesForFiltersModel model(qApp);
  MessageFilter filter;
  setScript(filter, QSL("msg.score = 17; msg.overwrite = true;"));
  model.processFeeds(&filter, m_account.get(), {m_feed});
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QCOMPARE(model.rowCount({}), 2);
  QVERIFY(model.resultForRow(0)->m_saveResult.has_value());
  QVERIFY(model.resultForRow(1)->m_saveResult.has_value());

  // Temporarily make the genuine batch hydration query fail. Restore the table
  // before assertions so a regression does not poison subsequent fixture tests.
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("ALTER TABLE Messages RENAME TO unavailable_test_messages;"));
  });
  model.testFilter(&filter, m_account.get());
  qApp->database()->worker()->write([](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.exec(QSL("ALTER TABLE unavailable_test_messages RENAME TO Messages;"));
  });
  QVERIFY(!model.lastError().isEmpty());
  for (int row = 0; row < model.rowCount({}); ++row) {
    QVERIFY(!model.resultForRow(row)->m_decision.has_value());
    QVERIFY(!model.resultForRow(row)->m_saveResult.has_value());
    QVERIFY(!model.resultForRow(row)->m_purged);
  }
}

void TestFilteringResults::reinsertedArticleUsesReceiptIdentity() {
  const Message original = seedArticle(m_feed, QSL("reinserted-original"));
  seedArticle(m_secondFeed, QSL("keep-next-id-distinct"));
  MessagesForFiltersModel model(qApp);
  bool removed = false;
  connect(&model, &MessagesForFiltersModel::scriptLogged, &model, [&](const QString&) {
    if (!removed) {
      qApp->database()->worker()->write([&](const QSqlDatabase& db) {
        DatabaseQueries::purgeMessage(db, original.m_id);
      });
      removed = true;
    }
  });
  MessageFilter filter;
  setScript(filter,
            QSL("app.log('remove original'); msg.title = 'Reinserted title';"
                "msg.contents = 'Reinserted contents'; msg.overwrite = true;"));
  model.processFeeds(&filter, m_account.get(), {m_feed});
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QVERIFY(removed);
  QCOMPARE(model.rowCount({}), 1);
  QCOMPARE(rowForId(model, original.m_id), 0);
  const auto& receipt = model.resultForRow(0)->m_saveResult;
  QVERIFY(receipt.has_value());
  QCOMPARE(receipt->m_outcome, MessageWriteOutcome::Inserted);
  QVERIFY(receipt->m_id != original.m_id);
  const int inserted_id = receipt->m_id;
  QVERIFY(model.hasStoredValuesForRow(0));
  QCOMPARE(model.messageDetailsForRow(0).m_id, inserted_id);
  QCOMPARE(model.messageDetailsForRow(0).m_contents, QSL("Reinserted contents"));

  setScript(filter, {});
  model.testFilter(&filter, m_account.get());
  QVERIFY2(model.lastError().isEmpty(), qPrintable(model.lastError()));
  QCOMPARE(model.resultForRow(0)->m_original.m_id, inserted_id);
  QVERIFY(model.resultForRow(0)->m_saveResult.has_value());
  QCOMPARE(model.resultForRow(0)->m_saveResult->m_id, inserted_id);
  QCOMPARE(model.resultForRow(0)->m_saveResult->m_decision.m_reason, MessageSaveReason::NoUpdateTrigger);
}

int main(int argc, char* argv[]) {
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
  QTemporaryDir directory;
  if (!directory.isValid()) {
    return 1;
  }
  prepareProfile(directory.path());
  QStringList arguments{QString::fromLocal8Bit(argv[0]),
                        QSL("--data"),
                        directory.path(),
                        QSL("--no-single-instance"),
                        QSL("--force-text-browser")};
  Application application(QSL("rssguard-filtering-results-test"), argc, argv, arguments);
  application.setQuitOnLastWindowClosed(false);
  TestFilteringResults tests;
  return QTest::qExec(&tests, argc, argv);
}

#include "test_filteringresults.moc"
