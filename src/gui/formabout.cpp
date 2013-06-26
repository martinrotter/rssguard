#include <QFile>
#include <QTextStream>

#include "core/datetime.h"
#include "gui/formabout.h"


FormAbout::FormAbout(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormAbout) {
  m_ui->setupUi(this);

  // Set flags and attributes.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);
  m_ui->m_lblIcon->setPixmap(QPixmap(APP_ICON_PATH));

  // Load information from embedded text files.
  QTextStream str;
  QFile file;
  str.setDevice(&file);

  file.setFileName(APP_INFO_PATH + "/COPYING_GNU_GPL_HTML");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtLicenseGnu->setText(str.readAll());
  }
  else {
    m_ui->m_txtLicenseGnu->setText(tr("License not found."));
  }
  file.close();

  file.setFileName(APP_INFO_PATH + "/COPYING_FLAGS");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtLicenseFlags->setText(str.readAll());
  }
  else {
    m_ui->m_txtLicenseFlags->setText(tr("License not found."));
  }
  file.close();

  file.setFileName(APP_INFO_PATH + "/CHANGELOG");
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_ui->m_txtChangelog->setText(str.readAll());
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
                                                                             DateTime::fromString(QString("%1 %2").arg(__DATE__,
                                                                                                                       __TIME__)).toString(Qt::DefaultLocaleShortDate),
                                                                             QT_VERSION_STR,
                                                                             qVersion(),
                                                                             APP_NAME));

  m_ui->m_txtThanks->setText(tr("<body>"
                                "Authors and contributors:"
                                "<ul>"
                                "<li>Martin Rotter (<a href=\"mailto://rotter.martinos@gmail.com\">rotter.martinos@gmail.com</a>)</li>"
                                "<li>KDE (author of Oxygen (GNU Lesser General Public License) icon theme)</li>"
                                "</ul>"
                                "</body>"));

  m_ui->m_txtInfo->setText(tr("<body>RSS Guard is a (very) tiny feed reader."
                              "<br><br>This software is distributed under the terms of GNU General Public License, version 3 or later."
                              "<br><br>Contacts:"
                              "<ul><li><a href=\"mailto://rotter.martinos@gmail.com\">rotter.martinos@gmail</a>  ~email</li>"
                              "<li><a href=\"http://www.rssguard.sf.net\">www.rssguard.sf.net</a> ~website</li></ul>"
                              "You can obtain source code for Qonverter from its website."
                              "<br><br><br>Copyright 2011-%1 Martin Rotter</body>").arg(QDateTime::currentDateTime().date().year()));
}

FormAbout::~FormAbout() {
  delete m_ui;
}
