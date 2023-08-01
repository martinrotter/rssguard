// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef JSSYNTAXHIGHLIGHTER_H
#define JSSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QRegularExpression>

class JsSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

  public:
    JsSyntaxHighlighter(QTextDocument *parent);

  protected:
    virtual void highlightBlock(const QString& text) override;

  private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QList<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
};

#endif // JSSYNTAXHIGHLIGHTER_H
