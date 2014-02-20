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

    if (info.m_availableVersion > APP_VERSION) {
      m_ui->m_cmbAvailableRelease->setStatus(WidgetWithStatus::Ok,
                                             tr("This is new version which can be\ndownloaded and installed."));
    }
    else {
      m_ui->m_cmbAvailableRelease->setStatus(WidgetWithStatus::Warning,
                                             tr("This release is not newer than\ncurrently installed one.."));
    }

    m_ui->m_lblPlatforms->setText(QStringList(info.m_urls.keys()).join(", "));

    // TODO: PODLE definice OS_ID (defs.h) se zjisti esli info.m_urls.keys()
    // obsahuje instalacni soubor pro danou platformu
    // a pokud ano tak se umoznuje update (jen windows a os2)
    // budou zahrnuty i "prazdne" soubory pro ostatni platformy
    // ale na tech nebude mozno updatovat.
  }
}

void FormUpdate::checkForUpdates() {
  QByteArray releases_xml;
  QNetworkReply::NetworkError download_result = NetworkFactory::downloadFeedFile(RELEASES_LIST,
                                                                                 5000,
                                                                                 releases_xml);

  m_ui->m_cmbAvailableRelease->comboBox()->clear();

  if (download_result != QNetworkReply::NoError) {
    m_ui->m_lblPlatforms->setText("-");
    m_ui->m_cmbAvailableRelease->setEnabled(false);
    m_ui->m_cmbAvailableRelease->setStatus(WidgetWithStatus::Error,
                                           tr("List with updates was not\ndownloaded successfully."));

    return;
  }

  QList<UpdateInfo> releases_list = SystemFactory::instance()->parseUpdatesFile(releases_xml);

  foreach (const UpdateInfo &release, releases_list) {
    m_ui->m_cmbAvailableRelease->comboBox()->addItem(release.m_availableVersion,
                                                     QVariant::fromValue(release));
  }
}
