// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMUPDATE_H
#define FORMUPDATE_H

#include <QDialog>

#include "ui_formupdate.h"

#include "miscellaneous/systemfactory.h"
#include "network-web/downloader.h"

#include <QNetworkReply>
#include <QPushButton>

class RSSGUARD_DLLSPEC FormUpdate : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormUpdate(QWidget* parent);

    // Returns true if application can self-update
    // on current platform.
    bool isSelfUpdateSupported() const;

  private slots:

    // Check for updates and interprets the results.
    void checkForUpdates();
    void startUpdate();

    void updateProgress(qint64 bytes_received, qint64 bytes_total);
    void updateCompleted(const QUrl& url,
                         QNetworkReply::NetworkError status,
                         int http_code,
                         const QByteArray& contents);
    void saveUpdateFile(const QByteArray& file_contents);

  private:
    void loadAvailableFiles();

    Ui::FormUpdate m_ui;
    QPushButton* m_btnUpdate;
    Downloader m_downloader;
    QString m_updateFilePath;
    UpdateInfo m_updateInfo;
    bool m_readyToInstall = false;
    qint64 m_lastDownloadedBytes = 0;
};

#endif // FORMUPDATE_H
