// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/dialogs/formupdate.h"

#include "definitions/definitions.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/iofactory.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "network-web/downloader.h"
#include "gui/messagebox.h"

#include <QNetworkReply>
#include <QProcess>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif


FormUpdate::FormUpdate(QWidget* parent)
	: QDialog(parent), m_downloader(nullptr), m_readyToInstall(false), m_ui(new Ui::FormUpdate), m_lastDownloadedBytes(0) {
	m_ui->setupUi(this);
	m_ui->m_lblCurrentRelease->setText(APP_VERSION);
	m_ui->m_tabInfo->removeTab(1);
	// Set flags and attributes.
	setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	setWindowIcon(qApp->icons()->fromTheme(QSL("help-about")));

	if (isSelfUpdateSupported()) {
		m_btnUpdate = m_ui->m_buttonBox->addButton(tr("Download selected update"), QDialogButtonBox::ActionRole);
		m_btnUpdate->setToolTip(tr("Download new installation files."));
	}

	else {
		m_btnUpdate = m_ui->m_buttonBox->addButton(tr("Go to application website"), QDialogButtonBox::ActionRole);
		m_btnUpdate->setToolTip(tr("Go to application website to get update packages manually."));
	}

	m_btnUpdate->setVisible(false);
	connect(m_btnUpdate, &QPushButton::clicked, this, &FormUpdate::startUpdate);
	checkForUpdates();
}

FormUpdate::~FormUpdate() {
}

bool FormUpdate::isSelfUpdateSupported() const {
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
	return true;
#else
	return false;
#endif
}

void FormUpdate::checkForUpdates() {
	const QPair<QList<UpdateInfo>, QNetworkReply::NetworkError> update = qApp->system()->checkForUpdates();

	if (update.second != QNetworkReply::NoError) {
		m_updateInfo = UpdateInfo();
		m_ui->m_tabInfo->setEnabled(false);
		//: Unknown release.
		m_ui->m_lblAvailableRelease->setText(tr("unknown"));
		m_ui->m_txtChanges->clear();
		m_ui->m_lblStatus->setStatus(WidgetWithStatus::Error,
		                             tr("Error: '%1'.").arg(NetworkFactory::networkErrorText(update.second)),
		                             tr("List with updates was not\ndownloaded successfully."));
	}

	else {
		const bool self_update_supported = isSelfUpdateSupported();
		m_updateInfo = update.first.at(0);
		m_ui->m_tabInfo->setEnabled(true);
		m_ui->m_lblAvailableRelease->setText(m_updateInfo.m_availableVersion);
		m_ui->m_txtChanges->setText(m_updateInfo.m_changes);

		if (SystemFactory::isVersionNewer(m_updateInfo.m_availableVersion, APP_VERSION)) {
			m_btnUpdate->setVisible(true);
			m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
			                             tr("New release available."),
			                             tr("This is new version which can be\ndownloaded."));

			if (self_update_supported) {
				loadAvailableFiles();
			}
		}

		else {
			m_ui->m_lblStatus->setStatus(WidgetWithStatus::Warning,
			                             tr("No new release available."),
			                             tr("This release is not newer than\ncurrently installed one."));
		}
	}
}

void FormUpdate::updateProgress(qint64 bytes_received, qint64 bytes_total) {
	if (bytes_received - m_lastDownloadedBytes > 500000 || m_lastDownloadedBytes == 0) {
		m_ui->m_lblStatus->setStatus(WidgetWithStatus::Information,
		                             tr("Downloaded %1% (update size is %2 kB).").arg(QString::number(bytes_total == 0 ? 0 : (bytes_received * 100.0) / bytes_total,
		                                     'f',
		                                     2),
		                                     QString::number(bytes_total / 1000,
		                                                     'f',
		                                                     2)),
		                             tr("Downloading update..."));
		m_ui->m_lblStatus->repaint();
		m_lastDownloadedBytes = bytes_received;
	}
}

void FormUpdate::saveUpdateFile(const QByteArray& file_contents) {
	const QString url_file = m_ui->m_listFiles->currentItem()->data(Qt::UserRole).toString();
	const QString temp_directory = qApp->getTempFolderPath();

	if (!temp_directory.isEmpty()) {
		const QString output_file_name = url_file.mid(url_file.lastIndexOf('/') + 1);
		QFile output_file(temp_directory + QDir::separator() + output_file_name);

		if (output_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
			qDebug("Storing update file to temporary location '%s'.",
			       qPrintable(QDir::toNativeSeparators(output_file.fileName())));
			output_file.write(file_contents);
			output_file.flush();
			output_file.close();
			qDebug("Update file contents was successfuly saved.");
			m_updateFilePath = output_file.fileName();
			m_readyToInstall = true;
		}

		else {
			qDebug("Cannot save downloaded update file because target temporary file '%s' cannot be "
			       "opened for writing.", qPrintable(output_file_name));
		}
	}

	else {
		qDebug("Cannot save downloaded update file because no TEMP directory is available.");
	}
}

void FormUpdate::loadAvailableFiles() {
	m_ui->m_listFiles->clear();

	foreach (const UpdateUrl& url, m_updateInfo.m_urls) {
		QListWidgetItem* item = new QListWidgetItem(url.m_name + tr(" (size ") + url.m_size + QSL(")"));
		item->setData(Qt::UserRole, url.m_fileUrl);
		item->setToolTip(url.m_fileUrl);
		m_ui->m_listFiles->addItem(item);
	}

	if (m_ui->m_listFiles->count() > 0) {
		m_ui->m_listFiles->setCurrentRow(0);
	}

	else {
		m_btnUpdate->setEnabled(false);
	}

	m_ui->m_tabInfo->addTab(m_ui->tabFiles, tr("Available update files"));
	m_ui->m_tabInfo->setCurrentIndex(1);
}

void FormUpdate::updateCompleted(QNetworkReply::NetworkError status, QByteArray contents) {
	qDebug("Download of application update file was completed with code '%d'.", status);

	switch (status) {
		case QNetworkReply::NoError:
			saveUpdateFile(contents);
			m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok, tr("Downloaded successfully"),
			                             tr("Package was downloaded successfully.\nYou can install it now."));
			m_btnUpdate->setText(tr("Install"));
			m_btnUpdate->setEnabled(true);
			break;

		default:
			m_ui->m_lblStatus->setStatus(WidgetWithStatus::Error, tr("Error occured"), tr("Error occured during downloading of the package."));
			m_btnUpdate->setText(tr("Error occured"));
			break;
	}
}

void FormUpdate::startUpdate() {
	QString url_file;
	const bool update_for_this_system = isSelfUpdateSupported();

	if (update_for_this_system && m_ui->m_listFiles->currentItem() != nullptr) {
		url_file = m_ui->m_listFiles->currentItem()->data(Qt::UserRole).toString();
		m_ui->m_listFiles->setEnabled(false);
	}

	else {
		url_file = APP_URL;
	}

	if (m_readyToInstall) {
		close();
		qDebug("Preparing to launch external installer '%s'.", qPrintable(QDir::toNativeSeparators(m_updateFilePath)));
#if defined(Q_OS_WIN)
		HINSTANCE exec_result = ShellExecute(nullptr,
		                                     nullptr,
		                                     reinterpret_cast<const WCHAR*>(QDir::toNativeSeparators(m_updateFilePath).utf16()),
		                                     nullptr,
		                                     nullptr,
		                                     SW_NORMAL);

		if (((int)exec_result) <= 32) {
			qDebug("External updater was not launched due to error.");
			qApp->showGuiMessage(tr("Cannot update application"),
			                     tr("Cannot launch external updater. Update application manually."),
			                     QSystemTrayIcon::Warning, this);
		}

		else {
			qApp->quit();
		}

#endif
	}

	else if (update_for_this_system) {
		// Nothing is downloaded yet, but update for this system
		// is available and self-update feature is present.
		if (m_downloader == nullptr) {
			// Initialie downloader.
			m_downloader = new Downloader(this);
			connect(m_downloader, &Downloader::progress, this, &FormUpdate::updateProgress);
			connect(m_downloader, &Downloader::completed, this, &FormUpdate::updateCompleted);
			updateProgress(0, 100);
		}

		m_btnUpdate->setText(tr("Downloading update..."));
		m_btnUpdate->setEnabled(false);
		m_downloader->downloadFile(url_file);
	}

	else {
		// Self-update and package are not available.
		if (!WebFactory::instance()->openUrlInExternalBrowser(url_file)) {
			qApp->showGuiMessage(tr("Cannot update application"),
			                     tr("Cannot navigate to installation file. Check new installation downloads manually on project website."),
			                     QSystemTrayIcon::Warning,
			                     this, true);
		}
	}
}
