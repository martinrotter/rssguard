// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GEMINIPARSER_H
#define GEMINIPARSER_H

#include <QString>
#include <QRegularExpressionMatch>

class GeminiParser {
  public:
    QString geminiToHtml(const QByteArray& gemini_data);

  private:
    QString parseLink(const QRegularExpressionMatch& mtch) const;
    QString parseHeading(const QRegularExpressionMatch& mtch) const;
    QString parseQuote(const QRegularExpressionMatch& mtch) const;
    QString parseList(const QRegularExpressionMatch& mtch) const;
    QString parseTextInNormalMode(const QString& line) const;
    QString parseInPreMode(const QString& line) const;
};

#endif // GEMINIPARSER_H
