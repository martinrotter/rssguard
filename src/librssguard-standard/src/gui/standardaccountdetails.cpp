// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/standardaccountdetails.h"

#include "src/definitions.h"
#include "src/standardserviceentrypoint.h"

#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <qtlinq/qtlinq.h>

#include <QImageReader>

StandardAccountDetails::StandardAccountDetails(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);
  m_ui.m_spinFeedSpacing->setMaximum(MAX_SPACING_SECONDS);

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

  m_ui.m_helpFeedSpacing
    ->setHelpText(tr("When you fetch many feeds from same website/host, then %1 could be (likely "
                     "temporarily) banned for making too many network requests at once.\n\n"
                     "If that is the case, then you need to set some time gaps when fetching those feeds.")
                    .arg(QSL(APP_NAME)),
                  false);

  connect(m_ui.m_spinFeedSpacing,
          QOverload<int>::of(&QSpinBox::valueChanged),
          this,
          &StandardAccountDetails::onFeedSpacingChanged);

  onFeedSpacingChanged(m_ui.m_spinFeedSpacing->value());
}

void StandardAccountDetails::onLoadIconFromFile() {
  auto supported_formats = QImageReader::supportedImageFormats();
  auto list_formats = qlinq::from(supported_formats).select([](const QByteArray& frmt) {
    return QSL("*.%1").arg(QString::fromLocal8Bit(frmt));
  });

  QString fil = FileDialog::openFileName(this,
                                         tr("Select icon file for the account"),
                                         qApp->homeFolder(),
                                         {},
                                         tr("Images (%1)").arg(list_formats.toList().join(QL1C(' '))),
                                         nullptr,
                                         GENERAL_REMEMBERED_PATH);

  if (!fil.isEmpty()) {
    m_ui.m_btnIcon->setIcon(QIcon(fil));
  }
}

void StandardAccountDetails::onUseDefaultIcon() {
  m_ui.m_btnIcon->setIcon(StandardServiceEntryPoint().icon());
}

void StandardAccountDetails::onFeedSpacingChanged(int spacing) {
  if (spacing <= 0) {
    m_ui.m_spinFeedSpacing->setSuffix(tr(" = no spacing"));
  }
  else {
    m_ui.m_spinFeedSpacing->setSuffix(tr(" seconds", nullptr, spacing));
  }
}
