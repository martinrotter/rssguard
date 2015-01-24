// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include "ui_downloadmanager.h"
#include "ui_downloaditem.h"

#include "gui/tabcontent.h"

#include <QNetworkReply>
#include <QFile>
#include <QDateTime>


class AutoSaver;
class DownloadModel;
class QFileIconProvider;
class QMimeData;

class DownloadItem : public QWidget {
    Q_OBJECT

    friend class DownloadManager;
    friend class DownloadModel;

  public:
    explicit DownloadItem(QNetworkReply *reply = 0, bool request_file_name = false, QWidget *parent = 0);
    virtual ~DownloadItem();

    bool downloading() const;
    bool downloadedSuccessfully() const;

    qint64 bytes_total() const;
    qint64 bytes_received() const;
    double remainingTime() const;
    double currentSpeed() const;

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
    void progress(qint64 bytes_received = 0, qint64 bytes_total = 0);
    void downloadFinished();

  private:
    void getFileName();
    void init();
    void updateInfoLabel();
    QString saveFileName(const QString &directory) const;

    Ui::DownloadItem *m_ui;
    QUrl m_url;
    QFile m_output;
    QNetworkReply *m_reply;
    qint64 m_bytesReceived;
    QTime m_downloadTime;
    QTime m_lastProgressTime;
    bool m_requestFileName;
    bool m_startedSaving;
    bool m_finishedDownloading;
    bool m_gettingFileName;
    bool m_canceledFileSelect;
};

class DownloadManager : public TabContent {
    Q_OBJECT
    Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy)
    Q_ENUMS(RemovePolicy)

    friend class DownloadModel;

  public:
    enum RemovePolicy {
      Never,
      Exit,
      SuccessFullDownload
    };

    explicit DownloadManager(QWidget *parent = 0);
    virtual ~DownloadManager();

    WebBrowser *webBrowser();
    QNetworkAccessManager *networkManager() const;

    int activeDownloads() const;
    bool allowQuit();

    RemovePolicy removePolicy() const;
    void setRemovePolicy(RemovePolicy policy);

    static QString timeString(double time_remaining);
    static QString dataString(qint64 size);

    void setDownloadDirectory(const QString &directory);
    QString downloadDirectory();

  public slots:
    void download(const QNetworkRequest &request, bool request_filename = false);
    void download(const QUrl &url, bool request_filename = false);
    void handleUnsupportedContent(QNetworkReply *reply, bool request_filename = false);
    void cleanup();

  private slots:
    void save() const;
    void updateRow(DownloadItem *item);
    void updateRow();
    void finished();

  private:
    void addItem(DownloadItem *item);
    void load();

    Ui::DownloadManager *m_ui;
    AutoSaver *m_autoSaver;
    DownloadModel *m_model;
    QNetworkAccessManager *m_networkManager;
    QFileIconProvider *m_iconProvider;
    QList<DownloadItem*> m_downloads;
    RemovePolicy m_removePolicy;
    QString m_downloadDirectory;
};

class DownloadModel : public QAbstractListModel {
    Q_OBJECT

    friend class DownloadManager;

  public:
    explicit DownloadModel(DownloadManager *download_manager, QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;

  private:
    DownloadManager *m_downloadManager;
};

#endif // DOWNLOADMANAGER_H
