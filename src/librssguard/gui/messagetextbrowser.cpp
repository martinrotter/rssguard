// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagetextbrowser.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/rootitem.h"

MessageTextBrowser::MessageTextBrowser(QWidget* parent) : QTextBrowser(parent) {
  setAutoFillBackground(true);
  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Plain);
  setTabChangesFocus(true);
  setOpenLinks(false);
  viewport()->setAutoFillBackground(true);
}

QString MessageTextBrowser::prepareHtmlForMessage(const Message& message) {
  QString html = QString("<h2 align=\"center\">%1</h2>").arg(message.m_title);

  if (!message.m_url.isEmpty()) {
    html += QString("[url] <a href=\"%1\">%1</a><br/>").arg(message.m_url);
  }

  for (const Enclosure& enc : message.m_enclosures) {
    QString enc_url;

    if (!enc.m_url.contains(QRegularExpression(QSL("^(http|ftp|\\/)")))) {
      enc_url = QString(INTERNAL_URL_PASSATTACHMENT) + QL1S("/?") + enc.m_url;
    }
    else {
      enc_url = enc.m_url;
    }

    html += QString("[%2] <a href=\"%1\">%1</a><br/>").arg(enc_url, enc.m_mimeType);
  }

  QRegularExpression imgTagRegex("\\<img[^\\>]*src\\s*=\\s*[\"\']([^\"\']*)[\"\'][^\\>]*\\>",
                                 QRegularExpression::PatternOption::CaseInsensitiveOption |
                                 QRegularExpression::PatternOption::InvertedGreedinessOption);
  QRegularExpressionMatchIterator i = imgTagRegex.globalMatch(message.m_contents);
  QString pictures_html;

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();

    m_pictures.append(match.captured(1));
    pictures_html += QString("<br/>[%1] <a href=\"%2\">%2</a>").arg(tr("image"), match.captured(1));
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayImagePlaceholders)).toBool()) {
    html += message.m_contents;
  }
  else {
    QString cnts = message.m_contents;

    html += cnts.replace(imgTagRegex, QString());
  }

  html += pictures_html;
  html = html
         .replace(QSL("\r\n"), QSL("\n"))
         .replace(QL1C('\r'), QL1C('\n'))
         .replace(QL1C('\n'), QSL("<br/>"));

  return html;
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

void MessageTextBrowser::clear() {
  QTextBrowser::clear();
  m_pictures.clear();
}

void MessageTextBrowser::reloadFontSettings() {
  const Settings* settings = qApp->settings();
  QFont fon;

  fon.fromString(settings->value(GROUP(Messages), SETTING(Messages::PreviewerFontStandard)).toString());
  setFont(fon);
}

void MessageTextBrowser::loadMessage(const Message& message, RootItem* root) {
  Q_UNUSED(root)

  setHtml(prepareHtmlForMessage(message));
}

void MessageTextBrowser::wheelEvent(QWheelEvent* e) {
  QTextBrowser::wheelEvent(e);
  qApp->settings()->setValue(GROUP(Messages), Messages::PreviewerFontStandard, font().toString());
}
