// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/jssyntaxhighlighter.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"

JsSyntaxHighlighter::JsSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
  HighlightingRule rule;

  m_keywordFormat.setForeground(Qt::GlobalColor::magenta);
  m_keywordFormat.setFontWeight(QFont::Weight::Bold);

  QStringList keywords = jsKeywords();
  auto std_keywords = boolinq::from(keywords.begin(), keywords.end())
                        .select([](const QString& kw) {
                          return QSL("\\b%1\\b").arg(kw);
                        })
                        .toStdList();

  keywords = FROM_STD_LIST(QStringList, std_keywords);

  for (const QString& pattern : std::as_const(keywords)) {
    rule.m_pattern = QRegularExpression(pattern);
    rule.m_format = m_keywordFormat;

    m_highlightingRules.append(rule);
  }

  m_classFormat.setFontWeight(QFont::Weight::Bold);
  m_classFormat.setForeground(Qt::GlobalColor::darkMagenta);
  rule.m_pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
  rule.m_format = m_classFormat;

  m_highlightingRules.append(rule);

  m_singleLineCommentFormat.setForeground(Qt::GlobalColor::red);
  rule.m_pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
  rule.m_format = m_singleLineCommentFormat;

  m_highlightingRules.append(rule);

  m_multiLineCommentFormat.setForeground(Qt::GlobalColor::red);

  m_quotationFormat.setForeground(Qt::GlobalColor::darkGreen);
  rule.m_pattern = QRegularExpression(QStringLiteral("\".*\""));
  rule.m_format = m_quotationFormat;

  m_highlightingRules.append(rule);

  m_functionFormat.setFontItalic(true);
  m_functionFormat.setForeground(Qt::GlobalColor::green);
  rule.m_pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
  rule.m_format = m_functionFormat;

  m_highlightingRules.append(rule);

  m_commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
  m_commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

QStringList JsSyntaxHighlighter::jsKeywords() const {
  return {QSL("abstract"),   QSL("arguments"), QSL("await\\*"),     QSL("boolean"),    QSL("break"),  QSL("byte"),
          QSL("case"),       QSL("catch"),     QSL("char"),         QSL("class\\*"),   QSL("const"),  QSL("continue"),
          QSL("debugger"),   QSL("default"),   QSL("delete"),       QSL("do"),         QSL("double"), QSL("else"),
          QSL("enum\\*"),    QSL("eval"),      QSL("export\\*"),    QSL("extends\\*"), QSL("false"),  QSL("final"),
          QSL("finally"),    QSL("float"),     QSL("for"),          QSL("function"),   QSL("goto"),   QSL("if"),
          QSL("implements"), QSL("import\\*"), QSL("in"),           QSL("instanceof"), QSL("int"),    QSL("interface"),
          QSL("let\\*"),     QSL("long"),      QSL("native"),       QSL("new"),        QSL("null"),   QSL("package"),
          QSL("private"),    QSL("protected"), QSL("public"),       QSL("return"),     QSL("short"),  QSL("static"),
          QSL("super\\*"),   QSL("switch"),    QSL("synchronized"), QSL("this"),       QSL("throw"),  QSL("throws"),
          QSL("transient"),  QSL("true"),      QSL("try"),          QSL("typeof"),     QSL("var"),    QSL("void"),
          QSL("volatile"),   QSL("while"),     QSL("with"),         QSL("yield")};
}

void JsSyntaxHighlighter::highlightBlock(const QString& text) {
  for (const HighlightingRule& rule : std::as_const(m_highlightingRules)) {
    QRegularExpressionMatchIterator matchIterator = rule.m_pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.m_format);
    }
  }

  setCurrentBlockState(0);

  int start_index = 0;
  if (previousBlockState() != 1) {
    start_index = text.indexOf(m_commentStartExpression);
  }

  while (start_index >= 0) {
    QRegularExpressionMatch match = m_commentEndExpression.match(text, start_index);
    int end_index = match.capturedStart();
    int comment_length = 0;

    if (end_index == -1) {
      setCurrentBlockState(1);
      comment_length = text.length() - start_index;
    }
    else {
      comment_length = end_index - start_index + match.capturedLength();
    }

    setFormat(start_index, comment_length, m_multiLineCommentFormat);
    start_index = text.indexOf(m_commentStartExpression, start_index + comment_length);
  }
}
