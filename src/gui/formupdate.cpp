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

void FormUpdate::checkForUpdates() {
  QByteArray releases_xml;
  QNetworkReply::NetworkError download_result = NetworkFactory::downloadFeedFile(RELEASES_LIST,
                                                                                 5000,
                                                                                 releases_xml);

  QList<UpdateInfo> releases_list = SystemFactory::instance()->parseUpdatesFile(releases_xml);

  foreach (const UpdateInfo &release, releases_list) {
    m_ui->m_cmbAvailableRelease->comboBox()->addItem(release.m_availableVersion,
                                                     release);
  }

}
