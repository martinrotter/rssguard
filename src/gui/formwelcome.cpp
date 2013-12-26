#include "gui/formwelcome.h"

#include "core/defs.h"

#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QDesktopWidget>


FormWelcome::FormWelcome(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormWelcome) {
  m_ui->setupUi(this);

  // Set flags.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);

  // Set icon.
  setWindowIcon(QIcon(APP_ICON_PATH));
  m_ui->m_lblLogo->setPixmap(QPixmap(APP_ICON_PATH));

  // Move the dialog into the middle of the screen.
  QRect screen = qApp->desktop()->screenGeometry();
  move(screen.center() - rect().center());

  // Make sure that clicked hyperlinks are opened in defult web browser.
  connect(m_ui->m_lblInfo, SIGNAL(linkActivated(QString)),
          this, SLOT(openLink(QString)));

  // Setup the text.
  m_ui->m_lblInfo->setText(
        tr("<p>RSS Guard is a (very) easy-to-use feed reader. "
           "It supports all major feed formats, including RSS, "
           "ATOM and RDF.</p>"
           "<p>Make sure you explore all available features. "
           "If you find a bug or if you want to propose new "
           "feature, then create new "
           "<a href=\"%1\"><span "
           "style=\"text-decoration: underline; color:#0000ff;\">issue "
           "report</span></a>.</p>"
           "<p>RSS Guard can be translated to any language. "
           "Contact its <a href=\"mailto:%2\"><span "
           "style=\"text-decoration: underline; color:#0000ff;\">author</span></a> "
           "in case of your interest.</p><p><br/></p>").arg(APP_URL_ISSUES,
                                                            APP_EMAIL));
}

void FormWelcome::openLink(const QString &link) {
  QDesktopServices::openUrl(QUrl(link));
}

FormWelcome::~FormWelcome() {
  delete m_ui;
}
