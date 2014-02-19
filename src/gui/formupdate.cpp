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

  connect(m_ui->m_cmbAvailableRelease->comboBox(), SIGNAL(currentIndexChanged(int)),
          this, SLOT(updateChanges(int)));

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

void FormUpdate::updateChanges(int new_release_index) {
  if (new_release_index >= 0) {
    UpdateInfo info = m_ui->m_cmbAvailableRelease->comboBox()->itemData(new_release_index).value<UpdateInfo>();

    m_ui->m_txtChanges->setText(info.m_changes);
  }
}

void FormUpdate::checkForUpdates() {
  QByteArray releases_xml;
  QNetworkReply::NetworkError download_result = NetworkFactory::downloadFeedFile(RELEASES_LIST,
                                                                                 5000,
                                                                                 releases_xml);

  QList<UpdateInfo> releases_list = SystemFactory::instance()->parseUpdatesFile(releases_xml);

  foreach (const UpdateInfo &release, releases_list) {
    m_ui->m_cmbAvailableRelease->comboBox()->addItem(release.m_availableVersion,
                                                     QVariant::fromValue(release));
  }

}
