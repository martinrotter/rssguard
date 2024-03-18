// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/standardaccountdetails.h"

#include <librssguard/3rd-party/boolinq/boolinq.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include "src/standardserviceentrypoint.h"

#include <QFileDialog>
#include <QImageReader>

StandardAccountDetails::StandardAccountDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);

  QMenu* icon_menu = new QMenu(tr("Icon selection"), this);
  auto* action_load_icon_from_file =
    new QAction(qApp->icons()->fromTheme(QSL("image-x-generic")), tr("Load icon from file..."), this);
  auto* action_default_icon =
    new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")), tr("Use default icon from icon theme"), this);

  connect(action_load_icon_from_file, &QAction::triggered, this, &StandardAccountDetails::onLoadIconFromFile);
  connect(action_default_icon, &QAction::triggered, this, &StandardAccountDetails::onUseDefaultIcon);

  icon_menu->addAction(action_load_icon_from_file);
  icon_menu->addAction(action_default_icon);

  m_ui.m_btnIcon->setMenu(icon_menu);
}

void StandardAccountDetails::onLoadIconFromFile() {
  auto supported_formats = QImageReader::supportedImageFormats();
  auto prefixed_formats = boolinq::from(supported_formats)
                            .select([](const QByteArray& frmt) {
                              return QSL("*.%1").arg(QString::fromLocal8Bit(frmt));
                            })
                            .toStdList();

  QStringList list_formats = FROM_STD_LIST(QStringList, prefixed_formats);

  QFileDialog dialog(this,
                     tr("Select icon file for the account"),
                     qApp->homeFolder(),
                     tr("Images (%1)").arg(list_formats.join(QL1C(' '))));

  dialog.setFileMode(QFileDialog::FileMode::ExistingFile);
  dialog.setWindowIcon(qApp->icons()->fromTheme(QSL("image-x-generic")));
  dialog.setOptions(QFileDialog::Option::DontUseNativeDialog | QFileDialog::Option::ReadOnly);
  dialog.setViewMode(QFileDialog::ViewMode::Detail);
  dialog.setLabelText(QFileDialog::DialogLabel::Accept, tr("Select icon"));
  dialog.setLabelText(QFileDialog::DialogLabel::Reject, tr("Cancel"));

  //: Label for field with icon file name textbox for selection dialog.
  dialog.setLabelText(QFileDialog::DialogLabel::LookIn, tr("Look in:"));
  dialog.setLabelText(QFileDialog::DialogLabel::FileName, tr("Icon name:"));
  dialog.setLabelText(QFileDialog::DialogLabel::FileType, tr("Icon type:"));

  if (dialog.exec() == QDialog::DialogCode::Accepted) {
    m_ui.m_btnIcon->setIcon(QIcon(dialog.selectedFiles().value(0)));
  }
}

void StandardAccountDetails::onUseDefaultIcon() {
  m_ui.m_btnIcon->setIcon(StandardServiceEntryPoint().icon());
}
