// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef JSSYNTAXHIGHLIGHTER_H
#define JSSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QRegularExpression>

class JsSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

  public:
    JsSyntaxHighlighter(QTextDocument* parent);

    QStringList jsKeywords() const;

  protected:
    virtual void highlightBlock(const QString& text) override;

  private:
    struct HighlightingRule {
        QRegularExpression m_pattern;
        QTextCharFormat m_format;
    };

    QList<HighlightingRule> m_highlightingRules;

    QRegularExpression m_commentStartExpression;
    QRegularExpression m_commentEndExpression;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_classFormat;
    QTextCharFormat m_singleLineCommentFormat;
    QTextCharFormat m_multiLineCommentFormat;
    QTextCharFormat m_quotationFormat;
    QTextCharFormat m_functionFormat;
};

#endif // JSSYNTAXHIGHLIGHTER_H
