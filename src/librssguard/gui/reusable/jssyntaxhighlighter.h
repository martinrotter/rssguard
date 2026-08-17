// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef JSSYNTAXHIGHLIGHTER_H
#define JSSYNTAXHIGHLIGHTER_H

#include <QSet>
#include <QStringList>
#include <QSyntaxHighlighter>

class JsSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

  public:
    explicit JsSyntaxHighlighter(QTextDocument* parent);

    QStringList jsKeywords() const;

  protected:
    void highlightBlock(const QString& text) override;

  private:
    QSet<QString> m_keywords;
    QSet<QString> m_literals;

    QTextCharFormat m_keywordFormat;
    QTextCharFormat m_classFormat;
    QTextCharFormat m_commentFormat;
    QTextCharFormat m_quotationFormat;
    QTextCharFormat m_functionFormat;
    QTextCharFormat m_numberFormat;
    QTextCharFormat m_regexFormat;
};

#endif // JSSYNTAXHIGHLIGHTER_H
