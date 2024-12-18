// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GEMINIPARSER_H
#define GEMINIPARSER_H

#include <QRegularExpressionMatch>
#include <QString>

class GeminiParser {
  public:
    QString geminiToHtml(const QByteArray& gemini_data);

  private:
    enum class State {
      // Regular state.
      Normal,

      // Inside list.
      List,

      // Inside quote.
      Quote,

      // Inside PRE.
      Pre
    };

    QString parseLink(const QRegularExpressionMatch& mtch) const;
    QString parseHeading(const QRegularExpressionMatch& mtch, QString* clean_header = nullptr) const;
    QString parseQuote(const QRegularExpressionMatch& mtch) const;
    QString parseList(const QRegularExpressionMatch& mtch) const;
    QString parseTextInNormalMode(const QString& line) const;
    QString parseInPreMode(const QString& line) const;

    QString beginBlock(State new_mode);
    QString endBlock(State new_mode);

  private:
    State mode;
};

#endif // GEMINIPARSER_H
