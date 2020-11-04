// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagetextbrowser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"

MessageTextBrowser::MessageTextBrowser(QWidget* parent) : QTextBrowser(parent) {
  setAutoFillBackground(true);
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Plain);
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
          m_imagePlaceholder = qApp->icons()->miscPixmap(QSL("image-placeholder")).scaledToWidth(20, Qt::FastTransformation);
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

void MessageTextBrowser::wheelEvent(QWheelEvent* event) {
  QTextBrowser::wheelEvent(event);
  qApp->settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, font().toString());
}

void MessageTextBrowser::resizeEvent(QResizeEvent* event) {
  // Notify parents about changed geometry.
  updateGeometry();
  QTextBrowser::resizeEvent(event);
}
