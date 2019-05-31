// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagetextbrowser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"

MessageTextBrowser::MessageTextBrowser(QWidget* parent) : QTextBrowser(parent) {}

QVariant MessageTextBrowser::loadResource(int type, const QUrl& name) {
  Q_UNUSED(name)

  switch (type) {
    case QTextDocument::ImageResource: {
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

void MessageTextBrowser::wheelEvent(QWheelEvent* e) {
  QTextBrowser::wheelEvent(e);
  qApp->settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, font().toString());
}
