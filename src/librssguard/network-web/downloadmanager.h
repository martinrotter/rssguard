// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "ui_downloaditem.h"
#include "ui_downloadmanager.h"

#include "gui/tabcontent.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QNetworkReply>

class AutoSaver;
class DownloadModel;
class QFileIconProvider;
class QMimeData;

class DownloadItem : public QWidget {
  Q_OBJECT

  friend class DownloadManager;
  friend class DownloadModel;

  public:
    explicit DownloadItem(QNetworkReply* reply = nullptr,
                          const QString& preferred_file_name = {},
                          const std::function<void (DownloadItem*)>& run_on_finish = {},
                          QWidget* parent = nullptr);
    virtual ~DownloadItem();

    bool downloading() const;
    bool downloadedSuccessfully() const;

    qint64 bytesTotal() const;
    qint64 bytesReceived() const;
    double remainingTime() const;
    double currentSpeed() const;
    const QFile& output() const;

  private slots:
    void stop();
    void tryAgain();
    void openFile();
    void openFolder();

    void downloadReadyRead();
    void error(QNetworkReply::NetworkError code);
    void downloadProgress(qint64 bytes_received, qint64 bytes_total);
    void metaDataChanged();
    void finished();

  signals:
    void statusChanged();
    void progress(qint64 bytes_received, qint64 bytes_total);
    void downloadFinished();

  private:
    void updateInfoAndUrlLabel();
    void getFileName();
    void init();
    void updateDownloadInfoLabel();
    QString saveFileName(const QString& directory) const;

    Ui::DownloadItem* m_ui;
    QUrl m_url;
    QFile m_output;
    QNetworkReply* m_reply;
    QString m_preferredFileName;
    std::function<void (DownloadItem*)> m_runOnFinish;
    qint64 m_bytesReceived;
    QElapsedTimer m_downloadTime;
    QTime m_lastProgressTime;
    bool m_requestFileName;
    bool m_startedSaving;
    bool m_finishedDownloading;
    bool m_gettingFileName;
    bool m_canceledFileSelect;
};

class WebBrowser;
class SilentNetworkAccessManager;

class DownloadManager : public TabContent {
  Q_OBJECT
  Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy NOTIFY removePolicyChanged)

  friend class DownloadModel;

  public:
    enum class RemovePolicy {
      Never,
      OnExit,
      OnSuccessfullDownload
    };

    Q_ENUM(RemovePolicy)

    explicit DownloadManager(QWidget* parent = nullptr);
    virtual ~DownloadManager();

    virtual WebBrowser* webBrowser() const;

    SilentNetworkAccessManager* networkManager() const;

    int totalDownloads() const;
    int activeDownloads() const;
    int downloadProgress() const;

    RemovePolicy removePolicy() const;
    void setRemovePolicy(RemovePolicy policy);

    void setDownloadDirectory(const QString& directory);
    QString downloadDirectory();

    static QString timeString(double time_remaining);
    static QString dataString(qint64 size);

  public slots:
    void download(const QNetworkRequest& request,
                  const QString& preferred_file_name = {},
                  const std::function<void(DownloadItem*)>& run_on_finish = {});
    void download(const QUrl& url);
    void cleanup();

  private slots:
    void save() const;
    void load();

    void updateRow(DownloadItem* item);
    void updateRow();
    void itemProgress();
    void itemFinished();

  signals:
    void removePolicyChanged();
    void downloadProgressed(int progress, const QString& description);
    void downloadFinished();

  private:
    void handleUnsupportedContent(QNetworkReply* reply,
                                  const QString& preferred_file_name,
                                  const std::function<void (DownloadItem*)>& run_on_finish);
    void addItem(DownloadItem* item);

    QScopedPointer<Ui::DownloadManager> m_ui;
    AutoSaver* m_autoSaver;
    DownloadModel* m_model;
    SilentNetworkAccessManager* m_networkManager;
    QScopedPointer<QFileIconProvider> m_iconProvider;
    QList<DownloadItem*> m_downloads;
    RemovePolicy m_removePolicy;
    QString m_downloadDirectory;
};

inline WebBrowser* DownloadManager::webBrowser() const {
  return nullptr;
}

class DownloadModel : public QAbstractListModel {
  Q_OBJECT

  friend class DownloadManager;

  public:
    explicit DownloadModel(DownloadManager* download_manager, QObject* parent = nullptr);

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
    virtual QMimeData* mimeData(const QModelIndexList& indexes) const;

  private:
    DownloadManager* m_downloadManager;
};

#endif // DOWNLOADMANAGER_H
