// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDFEEDSIMPORTEXPORTMODEL_H
#define STANDARDFEEDSIMPORTEXPORTMODEL_H

#include "services/abstract/accountcheckmodel.h"

#include <QDomElement>
#include <QFutureWatcher>
#include <QNetworkProxy>

class StandardFeed;
class StandardServiceRoot;

struct FeedLookup {
    RootItem* parent;
    QVariantMap custom_data;
    QString url;
    bool fetch_metadata_online;
    QNetworkProxy custom_proxy;
    QString post_process_script;
};

class FeedsImportExportModel : public AccountCheckSortedModel {
    Q_OBJECT

  public:
    enum class Mode { Import, Export };

    explicit FeedsImportExportModel(StandardServiceRoot* account, QObject* parent = nullptr);
    virtual ~FeedsImportExportModel();

    // Exports to OPML 2.0
    // NOTE: http://dev.opml.org/spec2.html
    bool exportToOMPL20(QByteArray& result, bool export_icons);
    void importAsOPML20(const QByteArray& data, bool fetch_metadata_online, const QString& post_process_script = {});

    // Exports to plain text format
    // where there is one feed URL per line.
    bool exportToTxtURLPerLine(QByteArray& result);
    void importAsTxtURLPerLine(const QByteArray& data,
                               bool fetch_metadata_online,
                               const QString& post_process_script = {});

    Mode mode() const;
    void setMode(Mode mode);

  signals:
    void parsingStarted();
    void parsingProgress(int completed, int total);
    void parsingFinished(int count_failed, int count_succeeded);

  private:
    bool produceFeed(const FeedLookup& feed_lookup);

  private:
    StandardServiceRoot* m_account;
    QMutex m_mtxLookup;
    QList<FeedLookup> m_lookup;
    RootItem* m_newRoot;
    QFutureWatcher<bool> m_watcherLookup;
    Mode m_mode;
};

#endif // STANDARDFEEDSIMPORTEXPORTMODEL_H
