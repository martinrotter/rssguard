// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/gemini/geminiparser.h"

#include "definitions/definitions.h"
#include "miscellaneous/iofactory.h"

QString GeminiParser::geminiToHtml(const QByteArray& gemini_data) {
  QString html;
  QString gemini_hypertext =
    QString::fromUtf8(gemini_data).replace(QSL("\r\n"), QSL("\n")).replace(QSL("\r"), QSL("\n"));
  QStringList lines = gemini_hypertext.split(QL1C('\n'));
  bool normal_mode = true;

  static QRegularExpression exp_link(R"(^=>\s+([^\s]+)(?:\s+(\w.+))?$)");
  static QRegularExpression exp_heading(R"(^(#{1,6})\s+(.+)$)");
  static QRegularExpression exp_list(R"(^\*\s(.+)$)");
  static QRegularExpression exp_quote(R"((?:^>$|^>\s?(.+)$))");
  static QRegularExpression exp_pre(R"(^```.*$)");
  static QRegularExpression exp_text(R"()");

  QRegularExpressionMatch mtch;

  for (const QString& line : lines) {
    if ((mtch = exp_pre.match(line)).hasMatch()) {
      normal_mode = !normal_mode;
      continue;
    }

    if (normal_mode) {
      if ((mtch = exp_link.match(line)).hasMatch()) {
        html += parseLink(mtch);
      }
      else if ((mtch = exp_heading.match(line)).hasMatch()) {
        html += parseHeading(mtch);
      }
      else if ((mtch = exp_list.match(line)).hasMatch()) {
        html += parseList(mtch);
      }
      else if ((mtch = exp_quote.match(line)).hasMatch()) {
        html += parseQuote(mtch);
      }
      else {
        html += parseTextInNormalMode(line);
      }
    }
    else {
      html += parseInPreMode(line);
    }
  }

  IOFactory::writeFile("a.gmi", html.toUtf8());

  return html;
}

QString GeminiParser::parseLink(const QRegularExpressionMatch& mtch) const {
  QString link = mtch.captured(1);
  QString name = mtch.captured(2);

  return QSL("<p>🔗 <a href=\"%1\">%2</a></p>\n").arg(link, name.isEmpty() ? link : name);
}

QString GeminiParser::parseHeading(const QRegularExpressionMatch& mtch) const {
  int level = mtch.captured(1).size();
  QString header = mtch.captured(2);

  return QSL("<h%1>%2</h%1>\n").arg(QString::number(level), header);
}

QString GeminiParser::parseQuote(const QRegularExpressionMatch &mtch) const {
  QString text = mtch.captured(1);

  return QSL("<p align=\"center\" style=\""
             "background-color: #E1E5EE;"
             "font-style: italic;"
             "margin-left: 20px;"
             "margin-right: 20px;"
             "\">%1</p>\n").arg(text.isEmpty() ? QString() : QSL("“%1”").arg(text));
}

QString GeminiParser::parseList(const QRegularExpressionMatch &mtch) const {
  QString text = mtch.captured(1);

  return QSL("<p style=\""
             "margin-left: 20px;"
             "\">• %1</p>\n").arg(text);
}

QString GeminiParser::parseTextInNormalMode(const QString &line) const{
  return QSL("<p>%1</p>\n").arg(line);
}

QString GeminiParser::parseInPreMode(const QString& line) const {
  return QSL("<pre>%1</pre>\n").arg(line);
}
