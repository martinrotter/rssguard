// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/jssyntaxhighlighter.h"

#include "definitions/definitions.h"

JsSyntaxHighlighter::JsSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
  HighlightingRule rule;

  keywordFormat.setForeground(Qt::GlobalColor::magenta);
  keywordFormat.setFontWeight(QFont::Weight::Bold);

  const QString keywordPatterns[] = {
    QSL("\\babstract\\b"),  QSL("\\barguments\\b"),  QSL("\\bawait\\*\\b"),     QSL("\\bboolean\\b"),
    QSL("\\bbreak\\b"),     QSL("\\bbyte\\b"),       QSL("\\bcase\\b"),         QSL("\\bcatch\\b"),
    QSL("\\bchar\\b"),      QSL("\\bclass\\*\\b"),   QSL("\\bconst\\b"),        QSL("\\bcontinue\\b"),
    QSL("\\bdebugger\\b"),  QSL("\\bdefault\\b"),    QSL("\\bdelete\\b"),       QSL("\\bdo\\b"),
    QSL("\\bdouble\\b"),    QSL("\\belse\\b"),       QSL("\\benum\\*\\b"),      QSL("\\beval\\b"),
    QSL("\\bexport\\*\\b"), QSL("\\bextends\\*\\b"), QSL("\\bfalse\\b"),        QSL("\\bfinal\\b"),
    QSL("\\bfinally\\b"),   QSL("\\bfloat\\b"),      QSL("\\bfor\\b"),          QSL("\\bfunction\\b"),
    QSL("\\bgoto\\b"),      QSL("\\bif\\b"),         QSL("\\bimplements\\b"),   QSL("\\bimport\\*\\b"),
    QSL("\\bin\\b"),        QSL("\\binstanceof\\b"), QSL("\\bint\\b"),          QSL("\\binterface\\b"),
    QSL("\\blet\\*\\b"),    QSL("\\blong\\b"),       QSL("\\bnative\\b"),       QSL("\\bnew\\b"),
    QSL("\\bnull\\b"),      QSL("\\bpackage\\b"),    QSL("\\bprivate\\b"),      QSL("\\bprotected\\b"),
    QSL("\\bpublic\\b"),    QSL("\\breturn\\b"),     QSL("\\bshort\\b"),        QSL("\\bstatic\\b"),
    QSL("\\bsuper\\*\\b"),  QSL("\\bswitch\\b"),     QSL("\\bsynchronized\\b"), QSL("\\bthis\\b"),
    QSL("\\bthrow\\b"),     QSL("\\bthrows\\b"),     QSL("\\btransient\\b"),    QSL("\\btrue\\b"),
    QSL("\\btry\\b"),       QSL("\\btypeof\\b"),     QSL("\\bvar\\b"),          QSL("\\bvoid\\b"),
    QSL("\\bvolatile\\b"),  QSL("\\bwhile\\b"),      QSL("\\bwith\\b"),         QSL("\\byield\\b")};

  for (const QString& pattern : keywordPatterns) {
    rule.pattern = QRegularExpression(pattern);
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }

  classFormat.setFontWeight(QFont::Bold);
  classFormat.setForeground(Qt::darkMagenta);
  rule.pattern = QRegularExpression(QStringLiteral("\\bQ[A-Za-z]+\\b"));
  rule.format = classFormat;
  highlightingRules.append(rule);

  singleLineCommentFormat.setForeground(Qt::red);
  rule.pattern = QRegularExpression(QStringLiteral("//[^\n]*"));
  rule.format = singleLineCommentFormat;
  highlightingRules.append(rule);

  multiLineCommentFormat.setForeground(Qt::red);

  quotationFormat.setForeground(Qt::darkGreen);
  rule.pattern = QRegularExpression(QStringLiteral("\".*\""));
  rule.format = quotationFormat;
  highlightingRules.append(rule);

  functionFormat.setFontItalic(true);
  functionFormat.setForeground(Qt::GlobalColor::green);
  rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\()"));
  rule.format = functionFormat;
  highlightingRules.append(rule);

  commentStartExpression = QRegularExpression(QStringLiteral("/\\*"));
  commentEndExpression = QRegularExpression(QStringLiteral("\\*/"));
}

void JsSyntaxHighlighter::highlightBlock(const QString& text) {
  for (const HighlightingRule& rule : std::as_const(highlightingRules)) {
    QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
    while (matchIterator.hasNext()) {
      QRegularExpressionMatch match = matchIterator.next();
      setFormat(match.capturedStart(), match.capturedLength(), rule.format);
    }
  }

  setCurrentBlockState(0);

  int startIndex = 0;
  if (previousBlockState() != 1)
    startIndex = text.indexOf(commentStartExpression);

  while (startIndex >= 0) {
    QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
    int endIndex = match.capturedStart();
    int commentLength = 0;
    if (endIndex == -1) {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    }
    else {
      commentLength = endIndex - startIndex + match.capturedLength();
    }
    setFormat(startIndex, commentLength, multiLineCommentFormat);
    startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
  }
}
