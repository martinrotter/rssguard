#include "gui/formupdate.h"

#include "core/defs.h"
#include "core/systemfactory.h"
#include "core/networkfactory.h"
#include "gui/iconthemefactory.h"

#if !defined(Q_OS_WIN)
#include "gui/messagebox.h"
#endif

#include <QNetworkReply>


FormUpdate::FormUpdate(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormUpdate) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::instance()->fromTheme("application-about"));

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  m_ui->m_lblCurrentRelease->setText(APP_VERSION);
  checkForUpdates();
}

FormUpdate::~FormUpdate() {
  delete m_ui;
}

// TODO: tady v update nacist do m_lblSupportedPlatforms
// seznam platform ktery danej release podporuje oddelenej carkama
// treba "Windows, OS2" atp atp.
// ten combobox se statusem previst na normalni combobox
// asi. jednotlivy URL souborů pro danej release
// sou dostupny v qhashi podle klice podle OS.

void FormUpdate::checkForUpdates() {
  QPair<UpdateInfo, QNetworkReply::NetworkError> update = SystemFactory::instance()->checkForUpdates();

  if (update.second != QNetworkReply::NoError) {
    m_ui->m_lblAvailableRelease->setText(tr("unknown"));
    m_ui->m_txtChanges->clear();
    m_ui->m_lblStatus->setStatus(WidgetWithStatus::Error,
                                 tr("Connection error occurred."),
                                 tr("List with updates was "
                                    "not\ndownloaded successfully."));
  }
  else {
    m_ui->m_lblAvailableRelease->setText(update.first.m_availableVersion);
    m_ui->m_txtChanges->setText(update.first.m_changes);

    if (update.first.m_availableVersion <= APP_VERSION) {
#if defined(Q_OS_WIN) || defined(Q_OS_OS2)
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
                                   tr("New release available."),
                                   tr("This is new version which can be\ndownloaded and installed."));
      // TODO: Display "update" button.
#else
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Ok,
                                   tr("New release available."),
                                   tr("This is new version. Upgrade to it manually or via your system package manager."));
#endif
    }
    else {
      m_ui->m_lblStatus->setStatus(WidgetWithStatus::Warning,
                                   tr("No new releases available."),
                                   tr("This release is not newer than\ncurrently installed one.."));
    }
  }
}
