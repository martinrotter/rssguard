// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formupdate.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/downloader.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#include <QNetworkReply>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

FormUpdate::FormUpdate(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);
  m_ui.m_lblCurrentRelease->setText(QSL(APP_VERSION));
  m_ui.m_tabInfo->removeTab(1);
  m_ui.m_buttonBox->setEnabled(false);

  // Set flags and attributes.
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("help-about")));

  connect(&m_downloader, &Downloader::progress, this, &FormUpdate::updateProgress);
  connect(&m_downloader, &Downloader::completed, this, &FormUpdate::updateCompleted);

  if (isSelfUpdateSupported()) {
    m_btnUpdate = m_ui.m_buttonBox->addButton(tr("Download selected update"), QDialogButtonBox::ButtonRole::ActionRole);
    m_btnUpdate->setToolTip(tr("Download new installation files."));
  }
  else {
    m_btnUpdate =
      m_ui.m_buttonBox->addButton(tr("Go to application website"), QDialogButtonBox::ButtonRole::ActionRole);
    m_btnUpdate->setToolTip(tr("Go to application website to get update packages manually."));
  }

  m_btnUpdate->setVisible(false);
  connect(m_btnUpdate, &QPushButton::clicked, this, &FormUpdate::startUpdate);
  checkForUpdates();
}

bool FormUpdate::isSelfUpdateSupported() const {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
  return true;
#else
  return false;
#endif
}

void FormUpdate::checkForUpdates() {
  connect(qApp->system(),
          &SystemFactory::updatesChecked,
          this,
          [this](const QPair<QList<UpdateInfo>, QNetworkReply::NetworkError>& update) {
            m_ui.m_buttonBox->setEnabled(true);
            disconnect(qApp->system(), &SystemFactory::updatesChecked, nullptr, nullptr);

            if (update.second != QNetworkReply::NoError) {
              m_updateInfo = UpdateInfo();
              m_ui.m_tabInfo->setEnabled(false);

              //: Unknown release.
              m_ui.m_lblAvailableRelease->setText(tr("unknown"));
              m_ui.m_txtChanges->clear();
              m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Error,
                                          tr("Error: '%1'.").arg(NetworkFactory::networkErrorText(update.second)),
                                          tr("List with updates was not\ndownloaded successfully."));
            }
            else {
              const bool self_update_supported = isSelfUpdateSupported();
              m_updateInfo = update.first.at(0);
              m_ui.m_tabInfo->setEnabled(true);
              m_ui.m_lblAvailableRelease->setText(m_updateInfo.m_availableVersion);
              m_ui.m_txtChanges->setMarkdown(m_updateInfo.m_changes);

              if (SystemFactory::isVersionNewer(m_updateInfo.m_availableVersion, QSL(APP_VERSION))) {
                m_btnUpdate->setVisible(true);
                m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Ok,
                                            tr("New release available."),
                                            tr("This is new version which can be\ndownloaded."));

                if (self_update_supported) {
                  loadAvailableFiles();
                }
              }
              else {
                m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Warning,
                                            tr("No new release available."),
                                            tr("This release is not newer than\ncurrently installed one."));
              }
            }
          });

  qApp->system()->checkForUpdates();
}

void FormUpdate::updateProgress(qint64 bytes_received, qint64 bytes_total) {
  if (bytes_received - m_lastDownloadedBytes > 500000 || m_lastDownloadedBytes == 0) {
    m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Information,
                                tr("Downloaded %1% (update size is %2 kB).")
                                  .arg(QString::number(bytes_total == 0L ? 0L : (bytes_received * 100.0) / bytes_total,
                                                       'f',
                                                       2),
                                       QString::number(bytes_total / 1000.0, 'f', 2)),
                                tr("Downloading update..."));
    m_ui.m_lblStatus->repaint();
    m_lastDownloadedBytes = bytes_received;
  }
}

void FormUpdate::saveUpdateFile(const QByteArray& file_contents) {
  const QString url_file = m_ui.m_listFiles->currentItem()->data(Qt::UserRole).toString();
  const QString temp_directory = qApp->tempFolder();

  if (!temp_directory.isEmpty()) {
    const QString output_file_name = url_file.mid(url_file.lastIndexOf('/') + 1);
    QFile output_file(temp_directory + QDir::separator() + output_file_name);

    if (output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      qDebugNN << "Storing update file to temporary location"
               << QUOTE_W_SPACE_DOT(QDir::toNativeSeparators(output_file.fileName()));

      output_file.write(file_contents);
      output_file.flush();
      output_file.close();

      qDebugNN << "Update file contents was successfuly saved.";

      m_updateFilePath = output_file.fileName();
      m_readyToInstall = true;
    }
    else {
      qDebugNN << LOGSEC_GUI << "Cannot save downloaded update file because target temporary file '" << output_file_name
               << "' cannot be opened for writing.";
    }
  }
  else {
    qDebugNN << LOGSEC_GUI << "Cannot save downloaded update file because no TEMP directory is available.";
  }
}

void FormUpdate::loadAvailableFiles() {
  m_ui.m_listFiles->clear();

  for (const UpdateUrl& url : std::as_const(m_updateInfo.m_urls)) {
    if (SystemFactory::supportedUpdateFiles().match(url.m_name).hasMatch()) {
      QListWidgetItem* item = new QListWidgetItem(url.m_name + tr(" (size ") + url.m_size + QSL(")"));

      item->setData(Qt::ItemDataRole::UserRole, url.m_fileUrl);
      item->setToolTip(url.m_fileUrl);
      m_ui.m_listFiles->addItem(item);
    }
  }

  if (m_ui.m_listFiles->count() > 0) {
    m_ui.m_listFiles->setCurrentRow(0);
  }
  else {
    m_btnUpdate->setEnabled(false);
  }

  m_ui.m_tabInfo->addTab(m_ui.tabFiles, tr("Available update files"));
  m_ui.m_tabInfo->setCurrentIndex(1);
}

void FormUpdate::updateCompleted(const QUrl& url,
                                 QNetworkReply::NetworkError status,
                                 int http_code,
                                 const QByteArray& contents) {
  Q_UNUSED(url)
  Q_UNUSED(http_code)

  qDebugNN << LOGSEC_GUI << "Download of application update file was completed with code" << QUOTE_W_SPACE_DOT(status);

  switch (status) {
    case QNetworkReply::NetworkError::NoError:
      saveUpdateFile(contents);
      m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Downloaded successfully"),
                                  tr("Package was downloaded successfully.\nYou can install it now."));
      m_btnUpdate->setText(tr("Install"));
      m_btnUpdate->setEnabled(true);
      break;

    default:
      m_ui.m_lblStatus->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("Error occured"),
                                  tr("Error occured during downloading of the package."));
      m_btnUpdate->setText(tr("Error occured"));
      break;
  }
}

void FormUpdate::startUpdate() {
  QString url_file;
  const bool update_for_this_system = isSelfUpdateSupported();

  if (update_for_this_system && m_ui.m_listFiles->currentItem() != nullptr) {
    url_file = m_ui.m_listFiles->currentItem()->data(Qt::UserRole).toString();
    m_ui.m_listFiles->setEnabled(false);
  }
  else {
    url_file = QSL(APP_URL);
  }

  if (m_readyToInstall) {
    close();
    qDebugNN << LOGSEC_GUI << "Preparing to launch external installer '" << QDir::toNativeSeparators(m_updateFilePath)
             << "'.";

#if defined(Q_OS_WIN)
    HINSTANCE exec_result =
      ShellExecute(nullptr,
                   nullptr,
                   reinterpret_cast<const WCHAR*>(QDir::toNativeSeparators(m_updateFilePath).utf16()),
                   nullptr,
                   nullptr,
                   SW_NORMAL);

    if (exec_result <= HINSTANCE(32)) {
      qDebugNN << LOGSEC_GUI << "External updater was not launched due to error.";
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Cannot update application"),
                            tr("Cannot launch external updater. Update application manually."),
                            QSystemTrayIcon::MessageIcon::Warning},
                           {},
                           {},
                           this);
    }
    else {
      qApp->quit();
    }
#endif
  }
  else if (update_for_this_system) {
    updateProgress(0, 100);
    m_btnUpdate->setText(tr("Downloading update..."));
    m_btnUpdate->setEnabled(false);
    m_downloader.downloadFile(url_file);
  }
  else {
    // Self-update and package are not available.
    qApp->web()->openUrlInExternalBrowser(url_file);
  }
}
