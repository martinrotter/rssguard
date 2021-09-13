// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/downloadmanager.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/autosaver.h"
#include "miscellaneous/settings.h"
#include "network-web/silentnetworkaccessmanager.h"

#include <cmath>

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMimeData>
#include <QSettings>

DownloadItem::DownloadItem(QNetworkReply* reply, QWidget* parent) : QWidget(parent),
  m_ui(new Ui::DownloadItem), m_reply(reply),
  m_bytesReceived(0), m_requestFileName(false), m_startedSaving(false), m_finishedDownloading(false),
  m_gettingFileName(false), m_canceledFileSelect(false) {
  m_ui->setupUi(this);
  m_ui->m_btnTryAgain->hide();
  m_requestFileName = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::AlwaysPromptForFilename)).toBool();

  m_ui->m_btnTryAgain->setIcon(qApp->icons()->fromTheme(QSL("view-refresh")));
  m_ui->m_btnOpenFile->setIcon(qApp->icons()->fromTheme(QSL("document-open")));
  m_ui->m_btnOpenFolder->setIcon(qApp->icons()->fromTheme(QSL("folder")));
  m_ui->m_btnStopDownload->setIcon(qApp->icons()->fromTheme(QSL("process-stop")));

  connect(m_ui->m_btnStopDownload, &QPushButton::clicked, this, &DownloadItem::stop);
  connect(m_ui->m_btnOpenFile, &QPushButton::clicked, this, &DownloadItem::openFile);
  connect(m_ui->m_btnTryAgain, &QPushButton::clicked, this, &DownloadItem::tryAgain);
  connect(m_ui->m_btnOpenFolder, &QPushButton::clicked, this, &DownloadItem::openFolder);
  init();
}

DownloadItem::~DownloadItem() {
  delete m_ui;
}

void DownloadItem::init() {
  if (m_reply == nullptr) {
    return;
  }

  m_startedSaving = false;
  m_finishedDownloading = false;
  m_ui->m_btnOpenFile->setEnabled(false);
  m_ui->m_btnOpenFolder->setEnabled(false);
  m_url = m_reply->url();
  m_reply->setParent(this);

  connect(m_reply, &QNetworkReply::readyRead, this, &DownloadItem::downloadReadyRead);

#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
  connect(m_reply, &QNetworkReply::errorOccurred, this, &DownloadItem::error);
#else
  connect(m_reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &DownloadItem::error);
#endif

  connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadItem::downloadProgress);
  connect(m_reply, &QNetworkReply::metaDataChanged, this, &DownloadItem::metaDataChanged);
  connect(m_reply, &QNetworkReply::finished, this, &DownloadItem::finished);

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

  const QString download_directory = qApp->downloadManager()->downloadDirectory();
  QString chosen_filename = saveFileName(download_directory);
  const QString filename_for_prompt = qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetExplicitDirectory)).toString() +
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

    const QFileInfo file_info = QFileInfo(chosen_filename);

    qApp->settings()->setValue(GROUP(Downloads), Downloads::TargetExplicitDirectory,
                               QDir::toNativeSeparators(QFileInfo(chosen_filename).absolutePath()));
    qApp->downloadManager()->setDownloadDirectory(file_info.absoluteDir().absolutePath());
  }

  m_output.setFileName(chosen_filename);

  // Check file path for saving.
  const QDir save_dir = QFileInfo(m_output.fileName()).dir();

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

QString DownloadItem::saveFileName(const QString& directory) const {
  QString path;

  if (m_reply->hasRawHeader("Content-Disposition")) {
    QString value = QLatin1String(m_reply->rawHeader("Content-Disposition"));
    QRegularExpression exp(QSL(".*filename=?\"([^\"]+)\"?"));
    QRegularExpressionMatch match = exp.match(value);

    if (match.isValid()) {
      QString name = match.captured(1);

      path = QUrl::fromPercentEncoding(name.toLocal8Bit());
    }
  }

  if (path.isEmpty()) {
    path = m_url.path();
  }

  const QFileInfo info(path);
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
    }
    while (QFile::exists(name));
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
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         tr("Cannot open file"),
                         tr("Cannot open output file. Open it manually."),
                         QSystemTrayIcon::MessageIcon::Warning,
                         true);
  }
}

void DownloadItem::openFolder() {
  if (m_output.exists()) {
    if (!SystemFactory::openFolderFile(m_output.fileName())) {
      MessageBox::show(this,
                       QMessageBox::Icon::Warning,
                       tr("Cannot open directory"),
                       tr("Cannot open output directory. Open it manually."),
                       QString(),
                       m_output.fileName());
    }
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
  QNetworkReply* new_download = qApp->downloadManager()->networkManager()->get(QNetworkRequest(m_url));

  if (m_reply != nullptr) {
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
  QVariant locationHeader = m_reply->header(QNetworkRequest::KnownHeaders::LocationHeader);

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

  if (m_lastProgressTime.isValid() && m_lastProgressTime.msecsTo(now) < 25) {
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

  const qint64 bytes_total = m_reply->header(QNetworkRequest::ContentLengthHeader).toULongLong();
  bool running = !downloadedSuccessfully();
  double speed = currentSpeed();
  double time_remaining = remainingTime();
  QString info;

  if (running) {
    QString remaining;

    if (bytes_total != 0) {
      remaining = DownloadManager::timeString(time_remaining);
    }

    info = QString(tr("%1 of %2 (%3 per second) - %4")).arg(DownloadManager::dataString(m_bytesReceived),
                                                            bytes_total == 0 ? QSL("?") : DownloadManager::dataString(bytes_total),
                                                            DownloadManager::dataString((int)speed),
                                                            remaining);
  }
  else {
    if (m_bytesReceived == bytes_total) {
      info = DownloadManager::dataString(m_output.size());
    }
    else {
      info = tr("%1 of %2 - download completed").arg(DownloadManager::dataString(m_bytesReceived),
                                                     DownloadManager::dataString(m_bytesReceived));
    }
  }

  m_ui->m_lblInfoDownload->setText(info);
}

bool DownloadItem::downloading() const {
  return !m_finishedDownloading;

  //return (m_ui->m_progressDownload->isVisible());
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
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         tr("Download finished"),
                         tr("File '%1' is downloaded.\nClick here to open parent directory.").arg(QDir::toNativeSeparators(
                                                                                                    m_output.fileName())),
                         QSystemTrayIcon::MessageIcon::Information,
                         {},
                         {},
                         tr("Open folder"),
                         [this] {
      openFolder();
    });
  }
}

void DownloadItem::updateInfoAndUrlLabel() {
  m_ui->m_lblRemoteFilename->setText(tr("URL: %1").arg(m_url.toString()));
  m_ui->m_lblLocalFilename->setText(tr("Local file: %1").arg(QDir::toNativeSeparators(m_output.fileName())));
}

DownloadManager::DownloadManager(QWidget* parent) : TabContent(parent), m_ui(new Ui::DownloadManager),
  m_autoSaver(new AutoSaver(this)), m_model(new DownloadModel(this)),
  m_networkManager(new SilentNetworkAccessManager(this)), m_iconProvider(nullptr), m_removePolicy(RemovePolicy::Never) {
  m_ui->setupUi(this);
  m_ui->m_viewDownloads->setShowGrid(false);
  m_ui->m_viewDownloads->verticalHeader()->hide();
  m_ui->m_viewDownloads->horizontalHeader()->hide();
  m_ui->m_viewDownloads->setAlternatingRowColors(true);
  m_ui->m_viewDownloads->horizontalHeader()->setStretchLastSection(true);
  m_ui->m_viewDownloads->setModel(m_model);

  m_ui->m_btnCleanup->setIcon(qApp->icons()->fromTheme(QSL("edit-clear")));

  setDownloadDirectory(qApp->settings()->value(GROUP(Downloads), SETTING(Downloads::TargetDirectory)).toString());
  connect(m_ui->m_btnCleanup, &QPushButton::clicked, this, &DownloadManager::cleanup);
  load();
}

DownloadManager::~DownloadManager() {
  m_autoSaver->changeOccurred();
  m_autoSaver->saveIfNeccessary();
  qDebugNN << LOGSEC_NETWORK << "Destroying DownloadManager instance.";
}

int DownloadManager::activeDownloads() const {
  int count = 0;

  for (const DownloadItem* download : m_downloads) {
    if (download->downloading()) {
      count++;
    }
  }

  return count;
}

int DownloadManager::downloadProgress() const {
  qint64 bytes_total = 0;
  qint64 bytes_received = 0;

  for (const DownloadItem* download : m_downloads) {
    if (download->downloading()) {
      bytes_total += download->bytesTotal();
      bytes_received += download->bytesReceived();
    }
  }

  if (bytes_total <= 0) {
    return -1;
  }
  else {
    return int((bytes_received * 100.0) / bytes_total);
  }
}

void DownloadManager::download(const QNetworkRequest& request) {
  if (!request.url().isEmpty()) {
    handleUnsupportedContent(m_networkManager->get(request));
  }
}

void DownloadManager::download(const QUrl& url) {
  download(QNetworkRequest(url));
}

void DownloadManager::handleUnsupportedContent(QNetworkReply* reply) {
  if (reply == nullptr || reply->url().isEmpty()) {
    return;
  }

  const QVariant header = reply->header(QNetworkRequest::KnownHeaders::ContentLengthHeader);
  bool ok;
  const int size = header.toInt(&ok);

  if (ok && size == 0) {
    return;
  }

  auto* item = new DownloadItem(reply, this);

  addItem(item);

  if (!item->m_canceledFileSelect && qApp->settings()->value(GROUP(Downloads),
                                                             SETTING(Downloads::ShowDownloadsWhenNewDownloadStarts)).toBool()) {
    qApp->mainForm()->tabWidget()->showDownloadManager();
  }
}

void DownloadManager::addItem(DownloadItem* item) {
  connect(item, &DownloadItem::statusChanged, this, static_cast<void (DownloadManager::*)()>(&DownloadManager::updateRow));
  connect(item, &DownloadItem::progress, this, &DownloadManager::itemProgress);
  connect(item, &DownloadItem::downloadFinished, this, &DownloadManager::itemFinished);

  const int row = m_downloads.count();

  m_model->beginInsertRows(QModelIndex(), row, row);
  m_downloads.append(item);
  m_model->endInsertRows();
  m_ui->m_viewDownloads->setIndexWidget(m_model->index(row, 0), item);
  QIcon icon = style()->standardIcon(QStyle::StandardPixmap::SP_FileIcon);

  item->m_ui->m_lblFileIcon->setPixmap(icon.pixmap(DOWNLOADER_ICON_SIZE, DOWNLOADER_ICON_SIZE));
  m_ui->m_viewDownloads->setRowHeight(row, item->sizeHint().height());

  // Just in case of download finishes before it is actually added.
  updateRow(item);
}

SilentNetworkAccessManager* DownloadManager::networkManager() const {
  return m_networkManager;
}

int DownloadManager::totalDownloads() const {
  return m_downloads.size();
}

void DownloadManager::itemFinished() {
  emit downloadFinished();
}

void DownloadManager::updateRow() {
  if (auto* item = qobject_cast<DownloadItem*>(sender())) {
    updateRow(item);
  }
}

void DownloadManager::itemProgress() {
  int progress = downloadProgress();

  if (progress < 0) {
    emit downloadFinished();
  }
  else {
    emit downloadProgressed(progress, tr("Downloading %n file(s)...", "", activeDownloads()));
  }
}

void DownloadManager::updateRow(DownloadItem* item) {
  const int row = m_downloads.indexOf(item);

  if (row == -1) {
    return;
  }

  if (m_iconProvider.isNull()) {
    m_iconProvider.reset(new QFileIconProvider());
  }

  QIcon icon = m_iconProvider->icon(item->m_output.fileName());

  if (icon.isNull()) {
    icon = style()->standardIcon(QStyle::StandardPixmap::SP_FileIcon);
  }

  item->m_ui->m_lblFileIcon->setPixmap(icon.pixmap(DOWNLOADER_ICON_SIZE, DOWNLOADER_ICON_SIZE));
  int old_height = m_ui->m_viewDownloads->rowHeight(row);

  m_ui->m_viewDownloads->setRowHeight(row, qMax(old_height, item->minimumSizeHint().height()));

  // Remove the item if:
  // a) It is not downloading and private browsing is enabled.
  // OR
  // b) Item is already downloaded and it should be remove from downloader list.
  bool remove = item->downloadedSuccessfully() && removePolicy() == RemovePolicy::OnSuccessfullDownload;

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

    emit removePolicyChanged();
  }
}

void DownloadManager::save() const {
  if (m_removePolicy == RemovePolicy::OnExit) {
    // No saving.
    return;
  }

  Settings* settings = qApp->settings();
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
  const Settings* settings = qApp->settings();
  int i = 0;

  // Restore the policy.
  m_removePolicy = static_cast<RemovePolicy>(settings->value(GROUP(Downloads), SETTING(Downloads::RemovePolicy)).toInt());

  // Restore downloads.
  while (settings->contains(GROUP(Downloads), QString(Downloads::ItemUrl).arg(i))) {
    QUrl url = settings->value(GROUP(Downloads), QString(Downloads::ItemUrl).arg(i)).toUrl();
    QString file_name = settings->value(GROUP(Downloads), QString(Downloads::ItemLocation).arg(i)).toString();
    bool done = settings->value(GROUP(Downloads), QString(Downloads::ItemDone).arg(i), true).toBool();

    if (!url.isEmpty() && !file_name.isEmpty()) {
      auto* item = new DownloadItem(nullptr, this);

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

void DownloadManager::setDownloadDirectory(const QString& directory) {
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

  return QSL("%1 %2").arg(new_size, 0, 'f', 1).arg(unit);
}

DownloadModel::DownloadModel(DownloadManager* download_manager, QObject* parent)
  : QAbstractListModel(parent), m_downloadManager(download_manager) {}

QVariant DownloadModel::data(const QModelIndex& index, int role) const {
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

int DownloadModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : m_downloadManager->m_downloads.count();
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex& parent) {
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

  if (m_downloadManager->totalDownloads() == 0) {
    m_downloadManager->m_ui->m_btnCleanup->setEnabled(false);
  }

  return true;
}

Qt::ItemFlags DownloadModel::flags(const QModelIndex& index) const {
  if (index.row() < 0 || index.row() >= rowCount(index.parent())) {
    return Qt::ItemFlag::NoItemFlags;
  }

  Qt::ItemFlags default_flags = QAbstractListModel::flags(index);
  DownloadItem* item = m_downloadManager->m_downloads.at(index.row());

  if (item->downloadedSuccessfully()) {
    return default_flags | Qt::ItemFlag::ItemIsDragEnabled;
  }

  return default_flags;
}

QMimeData* DownloadModel::mimeData(const QModelIndexList& indexes) const {
  auto* mimeData = new QMimeData();
  QList<QUrl> urls;

  for (const QModelIndex& index : indexes) {
    if (!index.isValid()) {
      continue;
    }

    urls.append(QUrl::fromLocalFile(QFileInfo(m_downloadManager->m_downloads.at(index.row())->m_output).absoluteFilePath()));
  }

  mimeData->setUrls(urls);
  return mimeData;
}
