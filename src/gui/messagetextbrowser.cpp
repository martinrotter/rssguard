#include "gui/messagetextbrowser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


MessageTextBrowser::MessageTextBrowser(QWidget *parent) : QTextBrowser(parent) {
}

MessageTextBrowser::~MessageTextBrowser() {
}

QVariant MessageTextBrowser::loadResource(int type, const QUrl &name) {
  Q_UNUSED(name)

  switch (type) {
    case QTextDocument::ImageResource: {
      if (m_imagePlaceholder.isNull()) {
        m_imagePlaceholder = qApp->icons()->pixmap(QSL("image-placeholder")).scaledToWidth(20, Qt::FastTransformation);
      }

      return m_imagePlaceholder;
    }

    default:
      return QVariant();
  }
}
