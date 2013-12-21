#include <QFile>
#include <QTextStream>

#include "core/textfactory.h"
#include "gui/formabout.h"
#include "gui/iconthemefactory.h"


FormAbout::FormAbout(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormAbout) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  setWindowIcon(IconThemeFactory::getInstance()->fromTheme("help-about"));
  m_ui->m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH));

  // Load information from embedded text files.
  QTextStream text_stream;
  QFile file;
  text_stream.setDevice(&file);

  file.setFileName(APP_INFO_PATH + "/COPYING_GNU_GPL_HTML");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtLicenseGnu->setText(text_stream.readAll());
  }
  else {
    m_ui->m_txtLicenseGnu->setText(tr("License not found."));
  }
  file.close();

  file.setFileName(APP_INFO_PATH + "/COPYING_BSD");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtLicenseBsd->setText(text_stream.readAll());
  }
  else {
    m_ui->m_txtLicenseBsd->setText(tr("License not found."));
  }
  file.close();

  file.setFileName(APP_INFO_PATH + "/AUTHORS");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtThanks->setText(text_stream.readAll());
  }
  else {
    m_ui->m_txtThanks->setText(tr("Authors information not found."));
  }
  file.close();

  file.setFileName(APP_INFO_PATH + "/CHANGELOG");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtChangelog->setText(text_stream.readAll());
  }
  else {
    m_ui->m_txtChangelog->setText(tr("Changelog not found."));
  }
  file.close();

  // Set other informative texts.
  m_ui->m_lblDesc->setText(tr("<b>%8</b><br>"
                              "<b>Version:</b> %1 (build on %2 with CMake %3)<br>"
                              "<b>Revision:</b> %4<br>"
                              "<b>Build date:</b> %5<br>"
                              "<b>Qt:</b> %6 (compiled against %7)<br>").arg(qApp->applicationVersion(),
                                                                             CMAKE_SYSTEM,
                                                                             CMAKE_VERSION,
                                                                             APP_REVISION,
                                                                             TextFactory::parseDateTime(QString("%1 %2").arg(__DATE__,
                                                                                                                             __TIME__)).toString(Qt::DefaultLocaleShortDate),
                                                                             qVersion(),
                                                                             QT_VERSION_STR,
                                                                             APP_NAME));

  m_ui->m_txtInfo->setText(tr("<body>RSS Guard is a (very) tiny feed reader."
                              "<br><br>This software is distributed under the terms of GNU General Public License, version 3."
                              "<br><br>Contacts:"
                              "<ul><li><a href=\"mailto://%1\">%1</a> ~email</li>"
                              "<li><a href=\"%2\">%2</a> ~website</li></ul>"
                              "You can obtain source code for RSS Guard from its website."
                              "<br><br><br>Copyrigh (C) 2011-%3 %4</body>").arg(APP_EMAIL,
                                                                               APP_URL,
                                                                               QString::number(QDateTime::currentDateTime().date().year()),
                                                                               APP_AUTHOR));
}

FormAbout::~FormAbout() {
  delete m_ui;
}
