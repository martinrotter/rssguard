// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagetextbrowser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/externaltool.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"

#include <QContextMenuEvent>
#include <QFileIconProvider>

MessageTextBrowser::MessageTextBrowser(QWidget* parent) : QTextBrowser(parent) {
  setAutoFillBackground(true);
  setFrameShape(QFrame::Shape::StyledPanel);
  setFrameShadow(QFrame::Shadow::Plain);
  setTabChangesFocus(true);
  setOpenLinks(false);
  viewport()->setAutoFillBackground(true);
}

QVariant MessageTextBrowser::loadResource(int type, const QUrl& name) {
  Q_UNUSED(name)

  switch (type) {
    case QTextDocument::ResourceType::ImageResource: {
      if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayImagePlaceholders)).toBool()) {
        if (m_imagePlaceholder.isNull()) {
          m_imagePlaceholder = qApp->icons()->miscPixmap(QSL("image-placeholder")).scaledToWidth(20,
                                                                                                 Qt::TransformationMode::FastTransformation);
        }

        return m_imagePlaceholder;
      }
      else {
        return QVariant();
      }
    }

    default:
      return QTextBrowser::loadResource(type, name);
  }
}

QSize MessageTextBrowser::sizeHint() const {
  auto doc_size = document()->size().toSize();

  doc_size.setHeight(doc_size.height() + contentsMargins().top() + contentsMargins().bottom());
  return doc_size;
}

void MessageTextBrowser::contextMenuEvent(QContextMenuEvent* event) {
  event->accept();

  auto* menu = createStandardContextMenu();

  if (menu == nullptr) {
    return;
  }

  auto anchor = anchorAt(event->pos());

  if (!anchor.isEmpty()) {
    QFileIconProvider icon_provider;
    QMenu* menu_ext_tools = new QMenu(tr("Open with external tool"), menu);
    auto tools = ExternalTool::toolsFromSettings();

    menu_ext_tools->setIcon(qApp->icons()->fromTheme(QSL("document-open")));

    for (const ExternalTool& tool : qAsConst(tools)) {
      QAction* act_tool = new QAction(QFileInfo(tool.executable()).fileName(), menu_ext_tools);

      act_tool->setIcon(icon_provider.icon(tool.executable()));
      act_tool->setToolTip(tool.executable());
      act_tool->setData(QVariant::fromValue(tool));
      menu_ext_tools->addAction(act_tool);

      connect(act_tool, &QAction::triggered, this, [act_tool, anchor]() {
        act_tool->data().value<ExternalTool>().run(anchor);
      });
    }

    if (menu_ext_tools->actions().isEmpty()) {
      QAction* act_not_tools = new QAction("No external tools activated");

      act_not_tools->setEnabled(false);
      menu_ext_tools->addAction(act_not_tools);
    }

    menu->addMenu(menu_ext_tools);
  }

  menu->popup(event->globalPos());
}

void MessageTextBrowser::wheelEvent(QWheelEvent* event) {
  QTextBrowser::wheelEvent(event);
  qApp->settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, font().toString());
}

void MessageTextBrowser::resizeEvent(QResizeEvent* event) {
  // Notify parents about changed geometry.
  updateGeometry();
  QTextBrowser::resizeEvent(event);
}
