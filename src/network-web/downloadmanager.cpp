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

#include "downloadmanager.h"

#include "miscellaneous/autosaver.h"

#include "miscellaneous/application.h"

#include "network-web/silentnetworkaccessmanager.h"

#include <math.h>

#include <qdesktopservices.h>
#include <qfiledialog.h>
#include <qfileiconprovider.h>
#include <qheaderview.h>
#include <qmessagebox.h>
#include <qmetaobject.h>
#include <qmimedata.h>
#include <qprocess.h>
#include <qsettings.h>

#include <qdebug.h>

#include <qwebsettings.h>

//#define DOWNLOADMANAGER_DEBUG

/*!
    DownloadItem is a widget that is displayed in the download manager list.
    It moves the data from the QNetworkReply into the QFile as well
    as update the information/progressbar and report errors.
 */
DownloadItem::DownloadItem(QNetworkReply *reply, bool requestFileName, QWidget *parent)
  : QWidget(parent)
  , m_reply(reply)
  , m_requestFileName(requestFileName)
  , m_bytesReceived(0)
  , m_startedSaving(false)
  , m_finishedDownloading(false)
  , m_gettingFileName(false)
  , m_canceledFileSelect(false)
{
  setupUi(this);
  QPalette p = downloadInfoLabel->palette();
  p.setColor(QPalette::Text, Qt::darkGray);
  downloadInfoLabel->setPalette(p);
  progressBar->setMaximum(0);
  tryAgainButton->hide();
  connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
  connect(openButton, SIGNAL(clicked()), this, SLOT(open()));
  connect(tryAgainButton, SIGNAL(clicked()), this, SLOT(tryAgain()));

  if (!requestFileName) {
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    m_requestFileName = settings.value(QLatin1String("alwaysPromptForFileName"), false).toBool();
  }

  init();
}

void DownloadItem::init()
{
  if (!m_reply)
    return;

  m_startedSaving = false;
  m_finishedDownloading = false;

  openButton->setEnabled(false);

  // attach to the m_reply
  m_url = m_reply->url();
  m_reply->setParent(this);
  connect(m_reply, SIGNAL(readyRead()), this, SLOT(downloadReadyRead()));
  connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)),
          this, SLOT(error(QNetworkReply::NetworkError)));
  connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)),
          this, SLOT(downloadProgress(qint64, qint64)));
  connect(m_reply, SIGNAL(metaDataChanged()),
          this, SLOT(metaDataChanged()));
  connect(m_reply, SIGNAL(finished()),
          this, SLOT(finished()));

  // reset info
  downloadInfoLabel->clear();
  progressBar->setValue(0);
  getFileName();

  // start timer for the download estimation
  m_downloadTime.start();

  if (m_reply->error() != QNetworkReply::NoError) {
    error(m_reply->error());
    finished();
  }
}

void DownloadItem::getFileName()
{
  if (m_gettingFileName)
    return;

  QString downloadDirectory = qApp->downloadManager()->downloadDirectory();

  QString defaultFileName = saveFileName(downloadDirectory);
  QString fileName = defaultFileName;
  if (m_requestFileName) {
    m_gettingFileName = true;
    fileName = QFileDialog::getSaveFileName(this, tr("Save File"), defaultFileName);
    m_gettingFileName = false;
    if (fileName.isEmpty()) {
      progressBar->setVisible(false);
      stop();
      fileNameLabel->setText(tr("Download canceled: %1").arg(QFileInfo(defaultFileName).fileName()));
      m_canceledFileSelect = true;
      return;
    }
    QFileInfo fileInfo = QFileInfo(fileName);
    qApp->downloadManager()->setDownloadDirectory(fileInfo.absoluteDir().absolutePath());
    fileNameLabel->setText(fileInfo.fileName());
  }
  m_output.setFileName(fileName);

  // Check file path for saving.
  QDir saveDirPath = QFileInfo(m_output.fileName()).dir();
  if (!saveDirPath.exists()) {
    if (!saveDirPath.mkpath(saveDirPath.absolutePath())) {
      progressBar->setVisible(false);
      stop();
      downloadInfoLabel->setText(tr("Download directory (%1) couldn't be created.").arg(saveDirPath.absolutePath()));
      return;
    }
  }

  fileNameLabel->setText(QFileInfo(m_output.fileName()).fileName());
  if (m_requestFileName)
    downloadReadyRead();
}

QString DownloadItem::saveFileName(const QString &directory) const
{
  // Move this function into QNetworkReply to also get file name sent from the server
  QString path;
  if (m_reply->hasRawHeader("Content-Disposition")) {
    QString value = QLatin1String(m_reply->rawHeader("Content-Disposition"));
    int pos = value.indexOf(QLatin1String("filename="));
    if (pos != -1) {
      QString name = value.mid(pos + 9);
      if (name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"')))
        name = name.mid(1, name.size() - 2);
      path = name;
    }
  }
  if (path.isEmpty())
    path = m_url.path();

  QFileInfo info(path);
  QString baseName = info.completeBaseName();
  QString endName = info.suffix();

  if (baseName.isEmpty()) {
    baseName = QLatin1String("unnamed_download");

#ifdef DOWNLOADMANAGER_DEBUG
    qDebug() << "DownloadItem::" << __FUNCTION__ << "downloading unknown file:" << m_url;
#endif
  }

  if (!endName.isEmpty())
    endName = QLatin1Char('.') + endName;

  QString name = directory + baseName + endName;
  if (!m_requestFileName && QFile::exists(name)) {
    // already exists, don't overwrite
    int i = 1;
    do {
      name = directory + baseName + QLatin1Char('-') + QString::number(i++) + endName;
    } while (QFile::exists(name));
  }
  return name;
}

void DownloadItem::stop()
{
  setUpdatesEnabled(false);
  stopButton->setEnabled(false);
  stopButton->hide();
  tryAgainButton->setEnabled(true);
  tryAgainButton->show();
  setUpdatesEnabled(true);
  m_reply->abort();
  emit downloadFinished();
}

void DownloadItem::open()
{
  QFileInfo info(m_output);
  QUrl url = QUrl::fromLocalFile(info.absoluteFilePath());
  QDesktopServices::openUrl(url);
}

void DownloadItem::tryAgain()
{
  if (!tryAgainButton->isEnabled())
    return;

  tryAgainButton->setEnabled(false);
  tryAgainButton->setVisible(false);
  stopButton->setEnabled(true);
  stopButton->setVisible(true);
  progressBar->setVisible(true);

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
      downloadInfoLabel->setText(tr("Error opening output file: %1")
                                 .arg(m_output.errorString()));
      stop();
      emit statusChanged();
      return;
    }
    emit statusChanged();
  }
  if (-1 == m_output.write(m_reply->readAll())) {
    downloadInfoLabel->setText(tr("Error saving: %1")
                               .arg(m_output.errorString()));
    stopButton->click();
  } else {
    m_startedSaving = true;
    if (m_finishedDownloading)
      finished();
  }
}

void DownloadItem::error(QNetworkReply::NetworkError)
{
#ifdef DOWNLOADMANAGER_DEBUG
  qDebug() << "DownloadItem::" << __FUNCTION__ << m_reply->errorString() << m_url;
#endif

  downloadInfoLabel->setText(tr("Network Error: %1").arg(m_reply->errorString()));
  tryAgainButton->setEnabled(true);
  tryAgainButton->setVisible(true);
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

#ifdef DOWNLOADMANAGER_DEBUG
  qDebug() << "DownloadItem::" << __FUNCTION__ << "not handled.";
#endif
}

void DownloadItem::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
  QTime now = QTime::currentTime();
  if (m_lastProgressTime.msecsTo(now) < 200)
    return;

  m_lastProgressTime = now;

  m_bytesReceived = bytesReceived;
  qint64 currentValue = 0;
  qint64 totalValue = 0;
  if (bytesTotal > 0) {
    currentValue = bytesReceived * 100 / bytesTotal;
    totalValue = 100;
  }
  progressBar->setValue(currentValue);
  progressBar->setMaximum(totalValue);

  emit progress(currentValue, totalValue);
  updateInfoLabel();
}

qint64 DownloadItem::bytesTotal() const
{
  return m_reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
}

qint64 DownloadItem::bytesReceived() const
{
  return m_bytesReceived;
}

double DownloadItem::remainingTime() const
{
  if (!downloading())
    return -1.0;

  double timeRemaining = ((double)(bytesTotal() - bytesReceived())) / currentSpeed();

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
  downloadInfoLabel->setText(info);
}

bool DownloadItem::downloading() const
{
  return (progressBar->isVisible());
}

bool DownloadItem::downloadedSuccessfully() const
{
  return (stopButton->isHidden() && tryAgainButton->isHidden());
}

void DownloadItem::finished()
{
  m_finishedDownloading = true;
  if (!m_startedSaving) {
    return;
  }
  progressBar->hide();
  stopButton->setEnabled(false);
  stopButton->hide();
  openButton->setEnabled(true);
  m_output.close();
  updateInfoLabel();
  emit statusChanged();
  emit downloadFinished();
}

/*!
    DownloadManager is a Dialog that contains a list of DownloadItems

    It is a basic download manager.  It only downloads the file, doesn't do BitTorrent,
    extract zipped files or anything fancy.
  */
DownloadManager::DownloadManager(QWidget *parent)
  : TabContent(parent)
  , m_autoSaver(new AutoSaver(this))
  , m_model(new DownloadModel(this))
  , m_networkManager(new SilentNetworkAccessManager(this))
  , m_iconProvider(0)
  , m_removePolicy(Never)
{
  setupUi(this);

  QSettings settings;
  settings.beginGroup(QLatin1String("downloadmanager"));
  QString defaultLocation = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
  setDownloadDirectory(settings.value(QLatin1String("downloadDirectory"), defaultLocation).toString());

  downloadsView->setShowGrid(false);
  downloadsView->verticalHeader()->hide();
  downloadsView->horizontalHeader()->hide();
  downloadsView->setAlternatingRowColors(true);
  downloadsView->horizontalHeader()->setStretchLastSection(true);
  downloadsView->setModel(m_model);
  connect(cleanupButton, SIGNAL(clicked()), this, SLOT(cleanup()));
  load();
}

DownloadManager::~DownloadManager()
{
  m_autoSaver->changeOccurred();
  m_autoSaver->saveIfNeccessary();
  if (m_iconProvider)
    delete m_iconProvider;
}

int DownloadManager::activeDownloads() const
{
  int count = 0;
  for (int i = 0; i < m_downloads.count(); ++i) {
    if (m_downloads.at(i)->stopButton->isEnabled())
      ++count;
  }
  return count;
}

bool DownloadManager::allowQuit()
{
  if (activeDownloads() >= 1) {
    int choice = QMessageBox::warning(this, QString(),
                                      tr("There are %1 downloads in progress\n"
                                         "Do you want to quit anyway?").arg(activeDownloads()),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No);
    if (choice == QMessageBox::No) {
      show();
      return false;
    }
  }

  return true;
}

void DownloadManager::download(const QNetworkRequest &request, bool requestFileName)
{
  if (request.url().isEmpty())
    return;

  handleUnsupportedContent(m_networkManager->get(request), requestFileName);
}

void DownloadManager::download(const QUrl &url, bool requestFileName) {
  download(QNetworkRequest(url), requestFileName);
}

void DownloadManager::handleUnsupportedContent(QNetworkReply *reply, bool requestFileName)
{
  if (!reply || reply->url().isEmpty())
    return;

  QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);
  bool ok;
  int size = header.toInt(&ok);
  if (ok && size == 0)
    return;

#ifdef DOWNLOADMANAGER_DEBUG
  qDebug() << "DownloadManager::" << __FUNCTION__ << reply->url() << "requestFileName" << requestFileName;
#endif

  DownloadItem *item = new DownloadItem(reply, requestFileName, this);
  addItem(item);

  if (item->m_canceledFileSelect)
    return;
  /*
    if (!isVisible())
        show();

    activateWindow();
    raise();*/
}

void DownloadManager::addItem(DownloadItem *item)
{
  connect(item, SIGNAL(statusChanged()), this, SLOT(updateRow()));
  connect(item, SIGNAL(downloadFinished()), this, SLOT(finished()));
  int row = m_downloads.count();
  m_model->beginInsertRows(QModelIndex(), row, row);
  m_downloads.append(item);
  m_model->endInsertRows();
  downloadsView->setIndexWidget(m_model->index(row, 0), item);
  QIcon icon = style()->standardIcon(QStyle::SP_FileIcon);
  item->fileIcon->setPixmap(icon.pixmap(48, 48));
  downloadsView->setRowHeight(row, item->sizeHint().height());
  updateRow(item); //incase download finishes before the constructor returns
  updateActiveItemCount();
}

void DownloadManager::updateActiveItemCount()
{
  int acCount = activeDownloads();
  if (acCount > 0) {
    setWindowTitle(QApplication::translate("DownloadDialog", "Downloading %1", 0, QApplication::UnicodeUTF8).arg(acCount));
  } else {
    setWindowTitle(QApplication::translate("DownloadDialog", "Downloads", 0, QApplication::UnicodeUTF8));
  }
}
QNetworkAccessManager *DownloadManager::networkManager() const
{
  return m_networkManager;
}

void DownloadManager::finished()
{
  updateActiveItemCount();
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
  item->fileIcon->setPixmap(icon.pixmap(48, 48));

  int oldHeight = downloadsView->rowHeight(row);
  downloadsView->setRowHeight(row, qMax(oldHeight, item->minimumSizeHint().height()));

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

  cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
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
      DownloadItem *item = new DownloadItem(0, this);
      item->m_output.setFileName(fileName);
      item->fileNameLabel->setText(QFileInfo(item->m_output.fileName()).fileName());
      item->m_url = url;
      item->stopButton->setVisible(false);
      item->stopButton->setEnabled(false);
      item->tryAgainButton->setVisible(!done);
      item->tryAgainButton->setEnabled(!done);
      item->progressBar->setVisible(false);
      addItem(item);
    }
    key = QString(QLatin1String("download_%1_")).arg(++i);
  }
  cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
  updateActiveItemCount();
}

void DownloadManager::cleanup()
{
  if (m_downloads.isEmpty())
    return;
  m_model->removeRows(0, m_downloads.count());
  updateActiveItemCount();
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

QString DownloadManager::timeString(double timeRemaining)
{
  QString remaining;

  if (timeRemaining > 60) {
    timeRemaining = timeRemaining / 60;
    timeRemaining = floor(timeRemaining);
    remaining = tr("%n minutes remaining", "", int(timeRemaining));
  }
  else {
    timeRemaining = floor(timeRemaining);
    remaining = tr("%n seconds remaining", "", int(timeRemaining));
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

DownloadModel::DownloadModel(DownloadManager *downloadManager, QObject *parent)
  : QAbstractListModel(parent)
  , m_downloadManager(downloadManager)
{
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
  if (index.row() < 0 || index.row() >= rowCount(index.parent()))
    return QVariant();
  if (role == Qt::ToolTipRole)
    if (!m_downloadManager->m_downloads.at(index.row())->downloadedSuccessfully())
      return m_downloadManager->m_downloads.at(index.row())->downloadInfoLabel->text();
  return QVariant();
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
  return (parent.isValid()) ? 0 : m_downloadManager->m_downloads.count();
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex &parent)
{
  if (parent.isValid())
    return false;

  int lastRow = row + count - 1;
  for (int i = lastRow; i >= row; --i) {
    if (m_downloadManager->m_downloads.at(i)->downloadedSuccessfully()
        || m_downloadManager->m_downloads.at(i)->tryAgainButton->isEnabled()) {
      beginRemoveRows(parent, i, i);
      m_downloadManager->m_downloads.takeAt(i)->deleteLater();
      endRemoveRows();
    }
  }
  m_downloadManager->m_autoSaver->changeOccurred();
  return true;
}

Qt::ItemFlags DownloadModel::flags(const QModelIndex &index) const
{
  if (index.row() < 0 || index.row() >= rowCount(index.parent()))
    return 0;

  Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

  DownloadItem *item = m_downloadManager->m_downloads.at(index.row());
  if (item->downloadedSuccessfully())
    return defaultFlags | Qt::ItemIsDragEnabled;

  return defaultFlags;
}

QMimeData *DownloadModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QList<QUrl> urls;
  foreach (const QModelIndex &index, indexes) {
    if (!index.isValid())
      continue;
    DownloadItem *item = m_downloadManager->m_downloads.at(index.row());
    urls.append(QUrl::fromLocalFile(QFileInfo(item->m_output).absoluteFilePath()));
  }
  mimeData->setUrls(urls);
  return mimeData;
}

WebBrowser *DownloadManager::webBrowser() {
  return NULL;
}
