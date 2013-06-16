#include <QDesktopServices>
#include <QUrl>
#include <QDesktopWidget>

#include "gui/formwelcome.h"
#include "core/defs.h"


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
  connect(m_ui->m_lblInfo, &QLabel::linkActivated, [=] (const QString &link) {
    QDesktopServices::openUrl(QUrl(link));
  });
}

FormWelcome::~FormWelcome() {
  delete m_ui;
}
