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
#include "gui/formmain.h"
#include "gui/tabwidget.h"
#include "gui/messagebox.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webbrowsernetworkaccessmanager.h"

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


DownloadItem::DownloadItem(QNetworkReply *reply, bool request_file_name, QWidget *parent) : QWidget(parent),
  m_ui(new Ui::DownloadItem), m_reply(reply),
  m_bytesReceived(0), m_requestFileName(request_file_name), m_startedSaving(false), m_finishedDownloading(false),
  m_gettingFileName(false), m_canceledFileSelect(false) {
  m_ui->setupUi(this);
  m_ui->m_btnTryAgain->hide();

  connect(m_ui->m_btnStopDownload, SIGNAL(clicked()), this, SLOT(stop()));
  connect(m_ui->m_btnOpenFile, SIGNAL(clicked()), this, SLOT(openFile()));
  connect(m_ui->m_btnTryAgain, SIGNAL(clicked()), this, SLOT(tryAgain()));
  connect(m_ui->m_btnOpenFolder, SIGNAL(clicked()), this, SLOT(openFolder()));

  if (!request_file_name) {
    m_requestFileName = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::AlwaysPromptForFilename)).toBool();
  }

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
  QString default_filename = saveFileName(download_directory);
  QString chosen_filename = default_filename;

  if (m_requestFileName) {
    m_gettingFileName = true;
    chosen_filename = QFileDialog::getSaveFileName(this, tr("Select destination for downloaded file"), default_filename);
    m_gettingFileName = false;

    if (chosen_filename.isEmpty()) {
      stop();

      m_ui->m_progressDownload->setVisible(false);
      m_ui->m_lblFilename->setText(tr("Download for %1 cancelled").arg(QFileInfo(default_filename).fileName()));
      m_canceledFileSelect = true;
      return;
    }

    QFileInfo file_info = QFileInfo(chosen_filename);

    qApp->downloadManager()->setDownloadDirectory(file_info.absoluteDir().absolutePath());
    m_ui->m_lblFilename->setText(file_info.fileName());
  }

  m_output.setFileName(chosen_filename);

  // Check file path for saving.
  QDir save_dir = QFileInfo(m_output.fileName()).dir();

  if (!save_dir.exists() && !save_dir.mkpath(save_dir.absolutePath())) {
    stop();

    m_ui->m_progressDownload->setVisible(false);
    m_ui->m_lblInfoDownload->setText(tr("Download directory %1 couldn't be created").arg(QDir::toNativeSeparators(save_dir.absolutePath())));
    return;
  }

  m_ui->m_lblFilename->setText(QFileInfo(m_output.fileName()).fileName());

  if (m_requestFileName) {
    downloadReadyRead();
  }
}

QString DownloadItem::saveFileName(const QString &directory) const {
  QString path;

  if (m_reply->hasRawHeader("Content-Disposition")) {
    QString value = QLatin1String(m_reply->rawHeader("Content-Disposition"));
    int pos = value.indexOf(QLatin1String("filename="));

    if (pos != -1) {
      QString name = value.mid(pos + 9);

      if (name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"'))) {
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
    base_name = QLatin1String("unnamed_download");
  }

  if (!end_name.isEmpty()) {
    end_name = QLatin1Char('.') + end_name;
  }

  QString name = directory + base_name + end_name;
  if (!m_requestFileName && QFile::exists(name)) {
    // already exists, don't overwrite
    int i = 1;
    do {
      name = directory + base_name + QLatin1Char('-') + QString::number(i++) + end_name;
    } while (QFile::exists(name));
  }
  return name;
}

void DownloadItem::stop()
{
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
  QDesktopServices::openUrl(QUrl::fromLocalFile(m_output.fileName()));
}

void DownloadItem::openFolder() {
  if (m_output.exists()) {
    QString folder = QDir::toNativeSeparators(QFileInfo(m_output.fileName()).absoluteDir().absolutePath());

#if defined(Q_OS_WIN32)
    QString file = QDir::toNativeSeparators(m_output.fileName());

    if (!QProcess::startDetached(QString("explorer.exe /select, \"") + file + "\"")) {
      MessageBox::show(this, QMessageBox::Warning, tr("Cannot open folder"), tr("Cannot open output folder. Open it manually."), QString(), folder);
    }
#else
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(folder))) {
      MessageBox::show(this, QMessageBox::Warning, tr("Cannot open folder"), tr("Cannot open output folder. Open it manually."), QString(), folder);
    }
#endif
  }
}

void DownloadItem::tryAgain() {
  if (!m_ui->m_btnTryAgain->isEnabled())
    return;

  m_ui->m_btnTryAgain->setEnabled(false);
  m_ui->m_btnTryAgain->setVisible(false);
  m_ui->m_btnStopDownload->setEnabled(true);
  m_ui->m_btnStopDownload->setVisible(true);
  m_ui->m_progressDownload->setVisible(true);

  QNetworkReply *r = qApp->downloadManager()->networkManager()->get(QNetworkRequest(m_url));
  if (m_reply)
    m_reply->deleteLater();
  if (m_output.exists())
    m_output.remove();
  m_reply = r;
  init();
  emit statusChanged();
}

void DownloadItem::downloadReadyRead()
{
  if (m_requestFileName && m_output.fileName().isEmpty())
    return;
  if (!m_output.isOpen()) {
    // in case someone else has already put a file there
    if (!m_requestFileName)
      getFileName();
    if (!m_output.open(QIODevice::WriteOnly)) {
      m_ui->m_lblInfoDownload->setText(tr("Error opening output file: %1")
                                       .arg(m_output.errorString()));
      stop();
      emit statusChanged();
      return;
    }
    emit statusChanged();
  }
  if (-1 == m_output.write(m_reply->readAll())) {
    m_ui->m_lblInfoDownload->setText(tr("Error saving: %1")
                                     .arg(m_output.errorString()));
    m_ui->m_btnStopDownload->click();
  } else {
    m_startedSaving = true;
    if (m_finishedDownloading)
      finished();
  }
}

void DownloadItem::error(QNetworkReply::NetworkError)
{
  m_ui->m_lblInfoDownload->setText(tr("Error: %1").arg(m_reply->errorString()));
  m_ui->m_btnTryAgain->setEnabled(true);
  m_ui->m_btnTryAgain->setVisible(true);

  emit downloadFinished();
}

void DownloadItem::metaDataChanged()
{
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
  if (m_lastProgressTime.msecsTo(now) < 25)
    return;

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
  updateInfoLabel();
}

qint64 DownloadItem::bytes_total() const
{
  return m_reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
}

qint64 DownloadItem::bytes_received() const
{
  return m_bytesReceived;
}

double DownloadItem::remainingTime() const
{
  if (!downloading())
    return -1.0;

  double timeRemaining = ((double)(bytes_total() - bytes_received())) / currentSpeed();

  // When downloading the eta should never be 0
  if (timeRemaining == 0)
    timeRemaining = 1;

  return timeRemaining;
}

double DownloadItem::currentSpeed() const
{
  if (!downloading())
    return -1.0;

  return m_bytesReceived * 1000.0 / m_downloadTime.elapsed();
}

void DownloadItem::updateInfoLabel()
{
  if (m_reply->error() != QNetworkReply::NoError)
    return;

  qint64 bytesTotal = m_reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
  bool running = !downloadedSuccessfully();

  // update info label
  double speed = currentSpeed();
  double timeRemaining = remainingTime();

  QString info;
  if (running) {
    QString remaining;

    if (bytesTotal != 0) {
      remaining = DownloadManager::timeString(timeRemaining);
    }

    info = QString(tr("%1 of %2 (%3/sec) - %4"))
           .arg(DownloadManager::dataString(m_bytesReceived))
           .arg(bytesTotal == 0 ? tr("?") : DownloadManager::dataString(bytesTotal))
           .arg(DownloadManager::dataString((int)speed))
           .arg(remaining);
  } else {
    if (m_bytesReceived == bytesTotal)
      info = DownloadManager::dataString(m_output.size());
    else
      info = tr("%1 of %2 - Download Complete")
             .arg(DownloadManager::dataString(m_bytesReceived))
             .arg(DownloadManager::dataString(bytesTotal));
  }
  m_ui->m_lblInfoDownload->setText(info);
}

bool DownloadItem::downloading() const
{
  return (m_ui->m_progressDownload->isVisible());
}

bool DownloadItem::downloadedSuccessfully() const
{
  return (m_ui->m_btnStopDownload->isHidden() && m_ui->m_btnTryAgain->isHidden());
}

void DownloadItem::finished()
{
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
  updateInfoLabel();
  emit statusChanged();
  emit downloadFinished();
}

DownloadManager::DownloadManager(QWidget *parent) : TabContent(parent), m_ui(new Ui::DownloadManager),
  m_autoSaver(new AutoSaver(this)), m_model(new DownloadModel(this)),
  m_networkManager(WebBrowserNetworkAccessManager::instance()), m_iconProvider(0), m_removePolicy(Never) {
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

DownloadManager::~DownloadManager()
{
  m_autoSaver->changeOccurred();
  m_autoSaver->saveIfNeccessary();

  if (m_iconProvider) {
    delete m_iconProvider;
  }

  delete m_ui;
}

int DownloadManager::activeDownloads() const
{
  int count = 0;
  for (int i = 0; i < m_downloads.count(); ++i) {
    if (m_downloads.at(i)->m_ui->m_btnStopDownload->isEnabled())
      ++count;
  }
  return count;
}

void DownloadManager::download(const QNetworkRequest &request, bool request_filename)
{
  if (request.url().isEmpty())
    return;

  handleUnsupportedContent(m_networkManager->get(request), request_filename);
}

void DownloadManager::download(const QUrl &url, bool request_filename) {
  download(QNetworkRequest(url), request_filename);
}

void DownloadManager::handleUnsupportedContent(QNetworkReply *reply, bool request_filename)
{
  if (reply == NULL || reply->url().isEmpty()) {
    return;
  }

  QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);
  bool ok;
  int size = header.toInt(&ok);

  if (ok && size == 0) {
    return;
  }

  DownloadItem *item = new DownloadItem(reply, request_filename, this);
  addItem(item);

  if (item->m_canceledFileSelect) {
    return;
  }

  qApp->mainForm()->tabWidget()->showDownloadManager();
}

void DownloadManager::addItem(DownloadItem *item) {
  connect(item, SIGNAL(statusChanged()), this, SLOT(updateRow()));
  connect(item, SIGNAL(downloadFinished()), this, SLOT(finished()));

  int row = m_downloads.count();
  m_model->beginInsertRows(QModelIndex(), row, row);
  m_downloads.append(item);
  m_model->endInsertRows();
  m_ui->m_viewDownloads->setIndexWidget(m_model->index(row, 0), item);
  QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
  item->m_ui->m_lblFileIcon->setPixmap(icon.pixmap(48, 48));
  m_ui->m_viewDownloads->setRowHeight(row, item->sizeHint().height());
  updateRow(item); //incase download finishes before the constructor returns
}

QNetworkAccessManager *DownloadManager::networkManager() const {
  return m_networkManager;
}

void DownloadManager::finished()
{
  if (isVisible()) {
    QApplication::alert(this);
  }
}

void DownloadManager::updateRow()
{
  if (DownloadItem *item = qobject_cast<DownloadItem*>(sender()))
    updateRow(item);
}

void DownloadManager::updateRow(DownloadItem *item)
{
  int row = m_downloads.indexOf(item);
  if (-1 == row)
    return;
  if (!m_iconProvider)
    m_iconProvider = new QFileIconProvider();
  QIcon icon = m_iconProvider->icon(item->m_output.fileName());
  if (icon.isNull())
    icon = style()->standardIcon(QStyle::SP_FileIcon);
  item->m_ui->m_lblFileIcon->setPixmap(icon.pixmap(48, 48));

  int oldHeight = m_ui->m_viewDownloads->rowHeight(row);
  m_ui->m_viewDownloads->setRowHeight(row, qMax(oldHeight, item->minimumSizeHint().height()));

  bool remove = false;
  QWebSettings *globalSettings = QWebSettings::globalSettings();
  if (!item->downloading()
      && globalSettings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
    remove = true;

  if (item->downloadedSuccessfully()
      && removePolicy() == DownloadManager::SuccessFullDownload) {
    remove = true;
  }
  if (remove)
    m_model->removeRow(row);

  m_ui->m_btnCleanup->setEnabled(m_downloads.count() - activeDownloads() > 0);
}

DownloadManager::RemovePolicy DownloadManager::removePolicy() const
{
  return m_removePolicy;
}

void DownloadManager::setRemovePolicy(RemovePolicy policy)
{
  if (policy == m_removePolicy)
    return;
  m_removePolicy = policy;
  m_autoSaver->changeOccurred();
}

void DownloadManager::save() const
{
  QSettings settings;
  settings.beginGroup(QLatin1String("downloadmanager"));
  QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
  settings.setValue(QLatin1String("removeDownloadsPolicy"), QLatin1String(removePolicyEnum.valueToKey(m_removePolicy)));
  settings.setValue(QLatin1String("size"), size());
  if (m_removePolicy == Exit)
    return;

  for (int i = 0; i < m_downloads.count(); ++i) {
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    settings.setValue(key + QLatin1String("url"), m_downloads[i]->m_url);
    settings.setValue(key + QLatin1String("location"), QFileInfo(m_downloads[i]->m_output).filePath());
    settings.setValue(key + QLatin1String("done"), m_downloads[i]->downloadedSuccessfully());
  }
  int i = m_downloads.count();
  QString key = QString(QLatin1String("download_%1_")).arg(i);
  while (settings.contains(key + QLatin1String("url"))) {
    settings.remove(key + QLatin1String("url"));
    settings.remove(key + QLatin1String("location"));
    settings.remove(key + QLatin1String("done"));
    key = QString(QLatin1String("download_%1_")).arg(++i);
  }
}

void DownloadManager::load()
{
  QSettings settings;
  settings.beginGroup(QLatin1String("downloadmanager"));
  QSize size = settings.value(QLatin1String("size")).toSize();
  if (size.isValid())
    resize(size);
  QByteArray value = settings.value(QLatin1String("removeDownloadsPolicy"), QLatin1String("Never")).toByteArray();
  QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
  m_removePolicy = removePolicyEnum.keyToValue(value) == -1 ?
                     Never :
                     static_cast<RemovePolicy>(removePolicyEnum.keyToValue(value));

  int i = 0;
  QString key = QString(QLatin1String("download_%1_")).arg(i);
  while (settings.contains(key + QLatin1String("url"))) {
    QUrl url = settings.value(key + QLatin1String("url")).toUrl();
    QString fileName = settings.value(key + QLatin1String("location")).toString();
    bool done = settings.value(key + QLatin1String("done"), true).toBool();
    if (!url.isEmpty() && !fileName.isEmpty()) {
      DownloadItem *item = new DownloadItem(0, false, this);
      item->m_output.setFileName(fileName);
      item->m_ui->m_lblFilename->setText(QFileInfo(item->m_output.fileName()).fileName());
      item->m_url = url;
      item->m_ui->m_btnStopDownload->setVisible(false);
      item->m_ui->m_btnStopDownload->setEnabled(false);
      item->m_ui->m_btnTryAgain->setVisible(!done);
      item->m_ui->m_btnTryAgain->setEnabled(!done);
      item->m_ui->m_progressDownload->setVisible(false);
      addItem(item);
    }
    key = QString(QLatin1String("download_%1_")).arg(++i);
  }
  m_ui->m_btnCleanup->setEnabled(m_downloads.count() - activeDownloads() > 0);
}

void DownloadManager::cleanup()
{
  if (m_downloads.isEmpty())
    return;
  m_model->removeRows(0, m_downloads.count());
  if (m_downloads.isEmpty() && m_iconProvider) {
    delete m_iconProvider;
    m_iconProvider = 0;
  }
  m_autoSaver->changeOccurred();
}

void DownloadManager::setDownloadDirectory(const QString &directory)
{
  m_downloadDirectory = directory;
  if (!m_downloadDirectory.isEmpty())
    m_downloadDirectory += QLatin1Char('/');
}

QString DownloadManager::downloadDirectory()
{
  return m_downloadDirectory;
}

QString DownloadManager::timeString(double time_remaining)
{
  QString remaining;

  if (time_remaining > 60) {
    time_remaining = time_remaining / 60;
    time_remaining = floor(time_remaining);
    remaining = tr("%n minutes remaining", "", int(time_remaining));
  }
  else {
    time_remaining = floor(time_remaining);
    remaining = tr("%n seconds remaining", "", int(time_remaining));
  }

  return remaining;
}

QString DownloadManager::dataString(qint64 size)
{
  QString unit;
  double newSize;

  if (size < 1024) {
    newSize = size;
    unit = tr("bytes");
  } else if (size < 1024 * 1024) {
    newSize = (double)size / (double)1024;
    unit = tr("kB");
  } else if (size < 1024 * 1024 * 1024) {
    newSize = (double)size / (double)(1024 * 1024);
    unit = tr("MB");
  } else {
    newSize = (double)size / (double)(1024 * 1024 * 1024);
    unit = tr("GB");
  }

  return QString(QLatin1String("%1 %2")).arg(newSize, 0, 'f', 1).arg(unit);
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
