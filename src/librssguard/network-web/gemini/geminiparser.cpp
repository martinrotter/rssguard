// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/gemini/geminiparser.h"

#include "definitions/definitions.h"
#include "miscellaneous/iofactory.h"

QString GeminiParser::geminiToHtml(const QByteArray& gemini_data) {
  QString html;
  QString gemini_hypertext =
    QString::fromUtf8(gemini_data).replace(QSL("\r\n"), QSL("\n")).replace(QSL("\r"), QSL("\n"));
  QStringList lines = gemini_hypertext.split(QL1C('\n'));
  mode = State::Normal;

  static QRegularExpression exp_link(R"(^=>\s+([^\s]+)(?:\s+(\S.+))?$)");
  static QRegularExpression exp_heading(R"(^(#{1,6})\s+(.+)$)");
  static QRegularExpression exp_list(R"(^\*\s(.+)$)");
  static QRegularExpression exp_quote(R"((?:^>$|^>\s?(.+)$))");
  static QRegularExpression exp_pre(R"(^```.*$)");

  static QString rich_style = QString::fromUtf8(IOFactory::readFile(QSL(":/scripts/gemini/style.css")));

  QRegularExpressionMatch mtch;
  QString title;

  for (const QString& line : lines) {
    if ((mtch = exp_pre.match(line)).hasMatch()) {
      // Begin or end PRE block.
      switch (mode) {
        case State::Pre:
          // Ending of a PRE block.
          html += endBlock(State::Normal);
          break;

        default:
          // Beginning of a PRE block.
          html += endBlock(State::Normal);
          html += beginBlock(State::Pre);
          break;
      }
      continue;
    }

    if (mode != State::Pre) {
      if ((mtch = exp_link.match(line)).hasMatch()) {
        html += endBlock(State::Normal);
        html += parseLink(mtch);
      }
      else if ((mtch = exp_heading.match(line)).hasMatch()) {
        html += endBlock(State::Normal);
        html += parseHeading(mtch, title.isEmpty() ? &title : nullptr);
      }
      else if ((mtch = exp_list.match(line)).hasMatch()) {
        html += beginBlock(State::List);
        html += parseList(mtch);
      }
      else if ((mtch = exp_quote.match(line)).hasMatch()) {
        html += beginBlock(State::Quote);
        html += parseQuote(mtch);
      }
      else {
        html += endBlock(State::Normal);
        html += parseTextInNormalMode(line);
      }
    }
    else {
      // Add new line in PRE mode.
      html += parseInPreMode(line);
    }
  }

  html += endBlock(State::Normal);

  return QSL("<!DOCTYPE html>"
             "<html>"
             "<head>"
             "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
             "<meta charset=utf-8>"
             "<title>%1</title>"
             "<style>%3</style>"
             "</head>"
             "<body>%2</body>"
             "</html>")
    .arg(title, html, m_richHtml ? rich_style : QString());
}

QString GeminiParser::beginBlock(State new_mode) {
  if (new_mode != mode) {
    mode = new_mode;

    switch (new_mode) {
      case State::List:
        return "<ul>\n";

      case State::Quote:
        return QSL("<%1 style=\""
                   "background-color: #E1E5EE;"
                   "font-style: italic;"
                   "margin-left: 20px;"
                   "margin-right: 20px;\">\n")
          .arg(m_richHtml ? QSL("blockquote") : QSL("div"));

      case State::Pre:
        return "<pre style=\"background-color: #E1E5EE;\">\n";
    }
  }

  return QString();
}

QString GeminiParser::endBlock(State new_mode) {
  QString to_return;

  if (new_mode != mode) {
    switch (mode) {
      case State::List:
        to_return = "</ul>\n";
        break;

      case State::Quote:
        to_return = QSL("</%1>\n").arg(m_richHtml ? QSL("blockquote") : QSL("div"));
        break;

      case State::Pre:
        to_return = "</pre>\n";
        break;
    }

    mode = new_mode;
  }

  return to_return;
}

GeminiParser::GeminiParser(bool rich_html) : m_richHtml(rich_html) {}

QString GeminiParser::parseLink(const QRegularExpressionMatch& mtch) const {
  QString link = mtch.captured(1);
  QString name = mtch.captured(2);

  return QSL("<p>&#128279; <a href=\"%1\">%2</a></p>\n").arg(link, name.isEmpty() ? link : name);
}

QString GeminiParser::parseHeading(const QRegularExpressionMatch& mtch, QString* clean_header) const {
  int level = mtch.captured(1).size();
  QString header = mtch.captured(2);

  if (!header.isEmpty() && clean_header != nullptr) {
    clean_header->clear();
    clean_header->append(header);
  }

  return QSL("<h%1>%2</h%1>\n").arg(QString::number(level), header);
}

QString GeminiParser::parseQuote(const QRegularExpressionMatch& mtch) const {
  QString text = mtch.captured(1);
  QString element = m_richHtml ? QSL("p") : QSL("div");

  return QSL("<%2>%1</%2>\n")
    .arg(text.simplified().isEmpty() ? QString() : (m_richHtml ? text : QSL("&#8220;%1&#8221;").arg(text)), element);
}

QString GeminiParser::parseList(const QRegularExpressionMatch& mtch) const {
  QString text = mtch.captured(1);

  return QSL("<li>%1</li>\n").arg(text);
}

QString GeminiParser::parseTextInNormalMode(const QString& line) const {
  return QSL("<p>%1</p>\n").arg(line);
}

QString GeminiParser::parseInPreMode(const QString& line) const {
  return QSL("%1\n").arg(line.toHtmlEscaped());
}
