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

#include "network-web/downloadmanager.h"

#include "miscellaneous/autosaver.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "gui/messagebox.h"
#include "network-web/silentnetworkaccessmanager.h"

#include <math.h>

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QMetaEnum>
#include <QProcess>
#include <QSettings>
#include <QDebug>
#include <QWebSettings>


DownloadItem::DownloadItem(QNetworkReply *reply, QWidget *parent) : QWidget(parent),
  m_ui(new Ui::DownloadItem), m_reply(reply),
  m_bytesReceived(0), m_requestFileName(false), m_startedSaving(false), m_finishedDownloading(false),
  m_gettingFileName(false), m_canceledFileSelect(false) {
  m_ui->setupUi(this);
  m_ui->m_btnTryAgain->hide();
  m_requestFileName = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::AlwaysPromptForFilename)).toBool();

  connect(m_ui->m_btnStopDownload, SIGNAL(clicked()), this, SLOT(stop()));
  connect(m_ui->m_btnOpenFile, SIGNAL(clicked()), this, SLOT(openFile()));
  connect(m_ui->m_btnTryAgain, SIGNAL(clicked()), this, SLOT(tryAgain()));
  connect(m_ui->m_btnOpenFolder, SIGNAL(clicked()), this, SLOT(openFolder()));
  init();
}

DownloadItem::~DownloadItem() {
  delete m_ui;
}

void DownloadItem::init() {
  if (m_reply == NULL) {
    return;
  }

  m_startedSaving = false;
  m_finishedDownloading = false;
  m_ui->m_btnOpenFile->setEnabled(false);
  m_ui->m_btnOpenFolder->setEnabled(false);
  m_url = m_reply->url();
  m_reply->setParent(this);

  connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadReadyRead()));
  connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
  connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));
  connect(m_reply, SIGNAL(metaDataChanged()), this, SLOT(metaDataChanged()));
  connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));

  // Reset info.
  m_ui->m_lblInfoDownload->clear();
  m_ui->m_progressDownload->setValue(0);
  getFileName();

  // Start timer for the download estimation.
  m_downloadTime.start();

  if (m_reply->error() != QNetworkReply::NoError) {
    error(m_reply->error());
    finished();
  }
}

void DownloadItem::getFileName() {
  if (m_gettingFileName) {
    return;
  }

  QString download_directory = qApp->downloadManager()->downloadDirectory();
  QString chosen_filename = saveFileName(download_directory);
  QString filename_for_prompt = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetExplicitDirectory)).toString() +
                                QDir::separator() +
                                QFileInfo(chosen_filename).fileName();

  if (m_requestFileName) {
    // User must provide the path where he wants to save downloaded file in.
    m_gettingFileName = true;
    chosen_filename = QFileDialog::getSaveFileName(this, tr("Select destination for downloaded file"), filename_for_prompt);
    m_gettingFileName = false;

    if (chosen_filename.isEmpty()) {
      stop();

      m_ui->m_progressDownload->setVisible(false);
      m_ui->m_lblLocalFilename->setText(tr("Selection of local file cancelled."));
      m_canceledFileSelect = true;
      return;
    }

    QFileInfo file_info = QFileInfo(chosen_filename);

    qApp->settings()->setValue(GROUP(Downloads), Downloads::TargetExplicitDirectory,
                               QDir::toNativeSeparators(QFileInfo(chosen_filename).absolutePath()));
    qApp->downloadManager()->setDownloadDirectory(file_info.absoluteDir().absolutePath());
  }

  m_output.setFileName(chosen_filename);

  // Check file path for saving.
  QDir save_dir = QFileInfo(m_output.fileName()).dir();

  if (!save_dir.exists() && !save_dir.mkpath(save_dir.absolutePath())) {
    stop();

    m_ui->m_progressDownload->setVisible(false);
    m_ui->m_lblInfoDownload->setText(tr("Download directory couldn't be created"));
    return;
  }

  updateInfoAndUrlLabel();

  if (m_requestFileName) {
    downloadReadyRead();
  }
}

QString DownloadItem::saveFileName(const QString &directory) const {
  QString path;

  if (m_reply->hasRawHeader("Content-Disposition")) {
    QString value = QLatin1String(m_reply->rawHeader("Content-Disposition"));
    int pos = value.indexOf(QL1S("filename="));

    if (pos != -1) {
      QString name = value.mid(pos + 9);

      if (name.startsWith(QL1C('"')) && name.endsWith(QL1C('"'))) {
        name = name.mid(1, name.size() - 2);
      }

      path = name;
    }
  }

  if (path.isEmpty()) {
    path = m_url.path();
  }

  QFileInfo info(path);
  QString base_name = info.completeBaseName();
  QString end_name = info.suffix();

  if (base_name.isEmpty()) {
    base_name = QSL("unnamed_download");
  }

  if (!end_name.isEmpty()) {
    end_name = QL1C('.') + end_name;
  }

  QString name = directory + base_name + end_name;

  if (!m_requestFileName && QFile::exists(name)) {
    int i = 1;

    do {
      name = directory + base_name + QL1C('-') + QString::number(i++) + end_name;
    } while (QFile::exists(name));
  }

  return name;
}

void DownloadItem::stop() {
  setUpdatesEnabled(false);
  m_ui->m_btnStopDownload->setEnabled(false);
  m_ui->m_btnStopDownload->hide();
  m_ui->m_btnTryAgain->setEnabled(true);
  m_ui->m_btnTryAgain->show();
  setUpdatesEnabled(true);

  m_reply->abort();
  emit downloadFinished();
}

void DownloadItem::openFile() {
  if (!QDesktopServices::openUrl(QUrl::fromLocalFile(m_output.fileName()))) {
    MessageBox::show(this, QMessageBox::Warning, tr("Cannot open file"), tr("Cannot open output file. Open it manually."),
                     QString(), QDir::toNativeSeparators(m_output.fileName()));
  }
}

void DownloadItem::openFolder() {
  if (m_output.exists()) {
    QString folder = QDir::toNativeSeparators(QFileInfo(m_output.fileName()).absoluteDir().absolutePath());

#if defined(Q_OS_WIN32)
    QString file = QDir::toNativeSeparators(m_output.fileName());

    if (!QProcess::startDetached(QString("explorer.exe /select, \"") + file + "\"")) {
      MessageBox::show(this, QMessageBox::Warning, tr("Cannot open directory"), tr("Cannot open output directory. Open it manually."), QString(), folder);
    }
#else
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(folder))) {
      MessageBox::show(this, QMessageBox::Warning, tr("Cannot open directory"), tr("Cannot open output directory. Open it manually."), QString(), folder);
    }
#endif
  }
}

void DownloadItem::tryAgain() {
  if (!m_ui->m_btnTryAgain->isEnabled()) {
    return;
  }

  m_ui->m_btnTryAgain->setEnabled(false);
  m_ui->m_btnTryAgain->setVisible(false);
  m_ui->m_btnStopDownload->setEnabled(true);
  m_ui->m_btnStopDownload->setVisible(true);
  m_ui->m_progressDownload->setVisible(true);

  QNetworkReply *new_download = qApp->downloadManager()->networkManager()->get(QNetworkRequest(m_url));

  if (m_reply) {
    m_reply->deleteLater();
  }

  if (m_output.exists()) {
    m_output.remove();
  }

  m_reply = new_download;

  init();
  emit statusChanged();
}

void DownloadItem::downloadReadyRead() {
  if (m_requestFileName && m_output.fileName().isEmpty()) {
    return;
  }

  if (!m_output.isOpen()) {
    if (!m_requestFileName) {
      getFileName();
    }

    if (!m_output.open(QIODevice::WriteOnly)) {
      m_ui->m_lblInfoDownload->setText(tr("Error opening output file: %1").arg(m_output.errorString()));
      stop();

      emit statusChanged();
      return;
    }

    emit statusChanged();
  }

  if (-1 == m_output.write(m_reply->readAll())) {
    m_ui->m_lblInfoDownload->setText(tr("Error when saving file: %1").arg(m_output.errorString()));
    m_ui->m_btnStopDownload->click();
  }
  else {
    m_startedSaving = true;

    if (m_finishedDownloading) {
      finished();
    }
  }
}

void DownloadItem::error(QNetworkReply::NetworkError code) {
  Q_UNUSED(code)

  m_ui->m_lblInfoDownload->setText(tr("Error: %1").arg(m_reply->errorString()));
  m_ui->m_btnTryAgain->setEnabled(true);
  m_ui->m_btnTryAgain->setVisible(true);

  emit downloadFinished();
}

void DownloadItem::metaDataChanged() {
  QVariant locationHeader = m_reply->header(QNetworkRequest::LocationHeader);

  if (locationHeader.isValid()) {
    m_url = locationHeader.toUrl();
    m_reply->deleteLater();
    m_reply = qApp->downloadManager()->networkManager()->get(QNetworkRequest(m_url));

    init();
    return;
  }
}

void DownloadItem::downloadProgress(qint64 bytes_received, qint64 bytes_total) {
  QTime now = QTime::currentTime();

  if (m_lastProgressTime.msecsTo(now) < 25) {
    return;
  }

  m_lastProgressTime = now;
  m_bytesReceived = bytes_received;

  qint64 currentValue = 0;
  qint64 totalValue = 0;

  if (bytes_total > 0) {
    currentValue = bytes_received * 100 / bytes_total;
    totalValue = 100;
  }

  m_ui->m_progressDownload->setValue(currentValue);
  m_ui->m_progressDownload->setMaximum(totalValue);

  emit progress(currentValue, totalValue);
  updateDownloadInfoLabel();
}

qint64 DownloadItem::bytesTotal() const {
  return m_reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
}

qint64 DownloadItem::bytesReceived() const {
  return m_bytesReceived;
}

double DownloadItem::remainingTime() const {
  if (!downloading()) {
    return -1.0;
  }

  double time_remaining = ((double)(bytesTotal() - bytesReceived())) / currentSpeed();

  // When downloading the ETA should never be 0.
  if ((int) time_remaining == 0) {
    time_remaining = 1.0;
  }

  return time_remaining;
}

double DownloadItem::currentSpeed() const {
  if (!downloading()) {
    return -1.0;
  }
  else {
    return m_bytesReceived * 1000.0 / m_downloadTime.elapsed();
  }
}

void DownloadItem::updateDownloadInfoLabel() {
  if (m_reply->error() != QNetworkReply::NoError) {
    return;
  }

  qint64 bytesTotal = m_reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
  bool running = !downloadedSuccessfully();
  double speed = currentSpeed();
  double timeRemaining = remainingTime();

  QString info;

  if (running) {
    QString remaining;

    if (bytesTotal != 0) {
      remaining = DownloadManager::timeString(timeRemaining);
    }

    info = QString(tr("%1 of %2 (%3 per second) - %4")).arg(DownloadManager::dataString(m_bytesReceived),
                                                            bytesTotal == 0 ? QSL("?") : DownloadManager::dataString(bytesTotal),
                                                            DownloadManager::dataString((int)speed),
                                                            remaining);
  }
  else {
    if (m_bytesReceived == bytesTotal) {
      info = DownloadManager::dataString(m_output.size());
    }
    else {
      info = tr("%1 of %2 - download completed").arg(DownloadManager::dataString(m_bytesReceived),
                                                     DownloadManager::dataString(bytesTotal));
    }
  }

  m_ui->m_lblInfoDownload->setText(info);
}

bool DownloadItem::downloading() const {
  return (m_ui->m_progressDownload->isVisible());
}

bool DownloadItem::downloadedSuccessfully() const {
  return (m_ui->m_btnStopDownload->isHidden() && m_ui->m_btnTryAgain->isHidden());
}

void DownloadItem::finished() {
  m_finishedDownloading = true;

  if (!m_startedSaving) {
    return;
  }

  m_ui->m_progressDownload->hide();
  m_ui->m_btnStopDownload->setEnabled(false);
  m_ui->m_btnStopDownload->hide();
  m_ui->m_btnOpenFile->setEnabled(true);
  m_ui->m_btnOpenFolder->setEnabled(true);
  m_output.close();
  updateDownloadInfoLabel();

  emit statusChanged();
  emit downloadFinished();

  if (downloadedSuccessfully()) {
    qApp->showGuiMessage(tr("Download finished"),
                         tr("File '%1' is downloaded.\nClick here to open parent directory.").arg(QDir::toNativeSeparators(m_output.fileName())),
                         QSystemTrayIcon::Information, 0, false, QIcon(), this, SLOT(openFolder()));
  }
}

void DownloadItem::updateInfoAndUrlLabel() {
  m_ui->m_lblRemoteFilename->setText(tr("URL: %1").arg(m_url.toString()));
  m_ui->m_lblLocalFilename->setText(tr("Local file: %1").arg(QDir::toNativeSeparators(m_output.fileName())));
}

DownloadManager::DownloadManager(QWidget *parent) : TabContent(parent), m_ui(new Ui::DownloadManager),
  m_autoSaver(new AutoSaver(this)), m_model(new DownloadModel(this)),
  m_networkManager(SilentNetworkAccessManager::instance()), m_iconProvider(0), m_removePolicy(Never) {
  m_ui->setupUi(this);
  m_ui->m_viewDownloads->setShowGrid(false);
  m_ui->m_viewDownloads->verticalHeader()->hide();
  m_ui->m_viewDownloads->horizontalHeader()->hide();
  m_ui->m_viewDownloads->setAlternatingRowColors(true);
  m_ui->m_viewDownloads->horizontalHeader()->setStretchLastSection(true);
  m_ui->m_viewDownloads->setModel(m_model);

  setDownloadDirectory(qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetDirectory)).toString());
  connect(m_ui->m_btnCleanup, SIGNAL(clicked()), this, SLOT(cleanup()));
  load();
}

DownloadManager::~DownloadManager() {
  m_autoSaver->changeOccurred();
  m_autoSaver->saveIfNeccessary();

  if (m_iconProvider != NULL) {
    delete m_iconProvider;
  }

  delete m_ui;

  qDebug("Destroying DownloadManager instance.");
}

int DownloadManager::activeDownloads() const {
  int count = 0;

  foreach (DownloadItem *download, m_downloads) {
    if (download->downloading()) {
      count++;
    }
  }

  return count;
}

int DownloadManager::downloadProgress() const {
  qint64 bytes_total = 0;
  qint64 bytes_received = 0;

  foreach (DownloadItem *download, m_downloads) {
    if (download->downloading()) {
      bytes_total += download->bytesTotal();
      bytes_received += download->bytesReceived();
    }
  }

  if (bytes_total <= 0) {
    return -1;
  }
  else {
    return (bytes_received * 100.0) / bytes_total;
  }
}

void DownloadManager::download(const QNetworkRequest &request) {
  if (!request.url().isEmpty()) {
    handleUnsupportedContent(m_networkManager->get(request));
  }
}

void DownloadManager::download(const QUrl &url) {
  download(QNetworkRequest(url));
}

void DownloadManager::handleUnsupportedContent(QNetworkReply *reply) {
  if (reply == NULL || reply->url().isEmpty()) {
    return;
  }

  QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);
  bool ok;
  int size = header.toInt(&ok);

  if (ok && size == 0) {
    return;
  }

  DownloadItem *item = new DownloadItem(reply, this);
  addItem(item);

  if (!item->m_canceledFileSelect) {
    qApp->mainForm()->tabWidget()->showDownloadManager();
  }
}

void DownloadManager::addItem(DownloadItem *item) {
  connect(item, SIGNAL(statusChanged()), this, SLOT(updateRow()));
  connect(item, SIGNAL(progress(qint64,qint64)), this, SLOT(itemProgress()));
  connect(item, SIGNAL(downloadFinished()), this, SLOT(itemFinished()));

  int row = m_downloads.count();
  m_model->beginInsertRows(QModelIndex(), row, row);
  m_downloads.append(item);
  m_model->endInsertRows();
  m_ui->m_viewDownloads->setIndexWidget(m_model->index(row, 0), item);

  QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
  item->m_ui->m_lblFileIcon->setPixmap(icon.pixmap(DOWNLOADER_ICON_SIZE, DOWNLOADER_ICON_SIZE));
  m_ui->m_viewDownloads->setRowHeight(row, item->sizeHint().height());

  // Just in case of download finishes before it is actually added.
  updateRow(item);
}

QNetworkAccessManager *DownloadManager::networkManager() const {
  return m_networkManager;
}

void DownloadManager::itemFinished() {
  emit downloadFinished();
}

void DownloadManager::updateRow() {
  if (DownloadItem *item = qobject_cast<DownloadItem*>(sender())) {
    updateRow(item);
  }
}

void DownloadManager::itemProgress() {
  int progress = downloadProgress();

  if (progress < 0) {
    emit downloadFinished();
  }
  else {
    emit downloadProgress(progress, tr("Downloading %n file(s)...", "", activeDownloads()));
  }
}

void DownloadManager::updateRow(DownloadItem *item) {
  int row = m_downloads.indexOf(item);

  if (row == -1) {
    return;
  }

  if (!m_iconProvider) {
    m_iconProvider = new QFileIconProvider();
  }

  QIcon icon = m_iconProvider->icon(item->m_output.fileName());

  if (icon.isNull()) {
    icon = style()->standardIcon(QStyle::SP_FileIcon);
  }

  item->m_ui->m_lblFileIcon->setPixmap(icon.pixmap(DOWNLOADER_ICON_SIZE, DOWNLOADER_ICON_SIZE));

  int old_height = m_ui->m_viewDownloads->rowHeight(row);
  m_ui->m_viewDownloads->setRowHeight(row, qMax(old_height, item->minimumSizeHint().height()));
  QWebSettings *globalSettings = QWebSettings::globalSettings();

  // Remove the item if:
  // a) It is not downloading and private browsing is enabled.
  // OR
  // b) Item is already downloaded and it should be remove from downloader list.
  bool remove = (!item->downloading() && globalSettings->testAttribute(QWebSettings::PrivateBrowsingEnabled)) ||
                (item->downloadedSuccessfully() && removePolicy() == DownloadManager::OnSuccessfullDownload);

  if (remove) {
    m_model->removeRow(row);
  }

  m_ui->m_btnCleanup->setEnabled(m_downloads.count() - activeDownloads() > 0);
}

DownloadManager::RemovePolicy DownloadManager::removePolicy() const {
  return m_removePolicy;
}

void DownloadManager::setRemovePolicy(RemovePolicy policy) {
  if (policy != m_removePolicy) {
    m_removePolicy = policy;
    m_autoSaver->changeOccurred();
  }
}

void DownloadManager::save() const {
  if (m_removePolicy == OnExit) {
    // No saving.
    return;
  }

  Settings *settings = qApp->settings();
  QString key;
  settings->setValue(GROUP(Downloads), Downloads::RemovePolicy, (int) removePolicy());

  // Save all download items.
  for (int i = 0; i < m_downloads.count(); i++) {
    settings->setValue(GROUP(Downloads), QString(Downloads::ItemUrl).arg(i), m_downloads[i]->m_url);
    settings->setValue(GROUP(Downloads), QString(Downloads::ItemLocation).arg(i), QFileInfo(m_downloads[i]->m_output).filePath());
    settings->setValue(GROUP(Downloads), QString(Downloads::ItemDone).arg(i), m_downloads[i]->downloadedSuccessfully());
  }

  // Remove all redundant saved download items.
  int i = m_downloads.size();

  while (!(key = QString(Downloads::ItemUrl).arg(i)).isEmpty() && settings->contains(GROUP(Downloads), key)) {
    settings->remove(GROUP(Downloads), key);
    settings->remove(GROUP(Downloads), QString(Downloads::ItemLocation).arg(i));
    settings->remove(GROUP(Downloads), QString(Downloads::ItemDone).arg(i));

    i++;
  }
}

void DownloadManager::load() {
  Settings *settings = qApp->settings();
  int i = 0;

  // Restore the policy.
  m_removePolicy = static_cast<RemovePolicy>(settings->value(GROUP(Downloads), SETTING(Downloads::RemovePolicy)).toInt());

  // Restore downloads.
  while (settings->contains(GROUP(Downloads), QString(Downloads::ItemUrl).arg(i))) {
    QUrl url = settings->value(GROUP(Downloads), QString(Downloads::ItemUrl).arg(i)).toUrl();
    QString file_name = settings->value(GROUP(Downloads), QString(Downloads::ItemLocation).arg(i)).toString();
    bool done = settings->value(GROUP(Downloads), QString(Downloads::ItemDone).arg(i), true).toBool();

    if (!url.isEmpty() && !file_name.isEmpty()) {
      DownloadItem *item = new DownloadItem(0, this);
      item->m_output.setFileName(file_name);
      item->m_url = url;

      item->updateInfoAndUrlLabel();

      item->m_ui->m_btnStopDownload->setVisible(false);
      item->m_ui->m_btnStopDownload->setEnabled(false);
      item->m_ui->m_btnTryAgain->setVisible(!done);
      item->m_ui->m_btnTryAgain->setEnabled(!done);
      item->m_ui->m_progressDownload->setVisible(false);
      addItem(item);
    }

    i++;
  }

  m_ui->m_btnCleanup->setEnabled(m_downloads.size() - activeDownloads() > 0);
}

void DownloadManager::cleanup() {
  if (!m_downloads.isEmpty()) {
    m_model->removeRows(0, m_downloads.count());
    m_ui->m_btnCleanup->setEnabled(false);
  }
}

void DownloadManager::setDownloadDirectory(const QString &directory) {
  m_downloadDirectory = directory;

  if (!m_downloadDirectory.isEmpty() && !m_downloadDirectory.endsWith(QDir::separator())) {
    m_downloadDirectory += QDir::separator();
  }
}

QString DownloadManager::downloadDirectory() {
  return m_downloadDirectory;
}

QString DownloadManager::timeString(double time_remaining) {
  QString remaining;

  if (time_remaining > 60) {
    time_remaining = time_remaining / 60;
    time_remaining = floor(time_remaining);
    remaining = tr("%n minutes remaining", "", (int) time_remaining);
  }
  else {
    time_remaining = floor(time_remaining);
    remaining = tr("%n seconds remaining", "", (int) time_remaining);
  }

  return remaining;
}

QString DownloadManager::dataString(qint64 size) {
  QString unit;
  double new_size;

  if (size < 1024) {
    new_size = size;
    unit = tr("bytes");
  }
  else if (size < 1024 * 1024) {
    new_size = (double)size / (double)1024;
    unit = tr("kB");
  }
  else if (size < 1024 * 1024 * 1024) {
    new_size = (double)size / (double)(1024 * 1024);
    unit = tr("MB");
  }
  else {
    new_size = (double)size / (double)(1024 * 1024 * 1024);
    unit = tr("GB");
  }

  return QString(QL1S("%1 %2")).arg(new_size, 0, 'f', 1).arg(unit);
}

DownloadModel::DownloadModel(DownloadManager *download_manager, QObject *parent)
  : QAbstractListModel(parent), m_downloadManager(download_manager) {
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const {
  if (index.row() < 0 || index.row() >= rowCount(index.parent())) {
    return QVariant();
  }

  if (role == Qt::ToolTipRole) {
    if (!m_downloadManager->m_downloads.at(index.row())->downloadedSuccessfully()) {
      return m_downloadManager->m_downloads.at(index.row())->m_ui->m_lblInfoDownload->text();
    }
  }

  return QVariant();
}

int DownloadModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : m_downloadManager->m_downloads.count();
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (parent.isValid()) {
    return false;
  }

  int lastRow = row + count - 1;

  for (int i = lastRow; i >= row; --i) {
    if (m_downloadManager->m_downloads.at(i)->downloadedSuccessfully() ||
        m_downloadManager->m_downloads.at(i)->m_ui->m_btnTryAgain->isEnabled()) {
      beginRemoveRows(parent, i, i);
      m_downloadManager->m_downloads.takeAt(i)->deleteLater();
      endRemoveRows();
    }
  }

  m_downloadManager->m_autoSaver->changeOccurred();
  return true;
}

Qt::ItemFlags DownloadModel::flags(const QModelIndex &index) const {
  if (index.row() < 0 || index.row() >= rowCount(index.parent())) {
    return Qt::NoItemFlags;
  }

  Qt::ItemFlags default_flags = QAbstractItemModel::flags(index);
  DownloadItem *item = m_downloadManager->m_downloads.at(index.row());

  if (item->downloadedSuccessfully()) {
    return default_flags | Qt::ItemIsDragEnabled;
  }

  return default_flags;
}

QMimeData *DownloadModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *mimeData = new QMimeData();
  QList<QUrl> urls;

  foreach (const QModelIndex &index, indexes) {
    if (!index.isValid()) {
      continue;
    }

    urls.append(QUrl::fromLocalFile(QFileInfo(m_downloadManager->m_downloads.at(index.row())->m_output).absoluteFilePath()));
  }

  mimeData->setUrls(urls);
  return mimeData;
}

WebBrowser *DownloadManager::webBrowser() {
  return NULL;
}
