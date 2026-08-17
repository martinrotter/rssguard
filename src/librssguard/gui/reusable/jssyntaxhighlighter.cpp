// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/jssyntaxhighlighter.h"

#include "definitions/definitions.h"

#include <QGuiApplication>
#include <QPalette>
#include <QTextDocument>

namespace {
  constexpr int STATE_NORMAL = 0;
  constexpr int STATE_BLOCK_COMMENT = 1;
  constexpr int STATE_SINGLE_QUOTED_STRING = 2;
  constexpr int STATE_DOUBLE_QUOTED_STRING = 3;
  constexpr int STATE_TEMPLATE_STRING = 4;

  constexpr int STATE_TEMPLATE_EXPRESSION = 100;
  constexpr int STATE_TEMPLATE_BLOCK_COMMENT = 10000;
  constexpr int STATE_TEMPLATE_SINGLE_QUOTED_STRING = 20000;
  constexpr int STATE_TEMPLATE_DOUBLE_QUOTED_STRING = 30000;
  constexpr int STATE_NESTED_TEMPLATE_STRING = 40000;
  constexpr int MAX_TEMPLATE_DEPTH = 9000;

  QColor syntaxColor(const QPalette& palette, int hue, int saturation = 180) {
    const bool dark_background = palette.color(QPalette::ColorRole::Base).lightness() < 128;
    return QColor::fromHsl(hue, saturation, dark_background ? 185 : 85);
  }

  QColor commentColor(const QPalette& palette) {
    const QColor text = palette.color(QPalette::ColorRole::Text);
    const QColor background = palette.color(QPalette::ColorRole::Base);

    return QColor((text.red() * 55 + background.red() * 45) / 100,
                  (text.green() * 55 + background.green() * 45) / 100,
                  (text.blue() * 55 + background.blue() * 45) / 100);
  }

  bool isIdentifierStart(QChar character) {
    return character.isLetter() || character == QLatin1Char('_') || character == QLatin1Char('$');
  }

  bool isIdentifierPart(QChar character) {
    return character.isLetterOrNumber() || character == QLatin1Char('_') || character == QLatin1Char('$') ||
           character.category() == QChar::Category::Mark_NonSpacing ||
           character.category() == QChar::Category::Mark_SpacingCombining;
  }

  int encodedState(int state_base, int template_depth) {
    return state_base + qBound(1, template_depth, MAX_TEMPLATE_DEPTH);
  }

  int scanNumber(const QString& text, int start) {
    const int length = text.size();
    int index = start;

    if (text.at(index) == QLatin1Char('0') && index + 1 < length) {
      const QChar prefix = text.at(index + 1).toLower();

      if (prefix == QLatin1Char('x') || prefix == QLatin1Char('b') || prefix == QLatin1Char('o')) {
        index += 2;

        while (index < length && (text.at(index).isLetterOrNumber() || text.at(index) == QLatin1Char('_'))) {
          ++index;
        }

        if (index < length && text.at(index) == QLatin1Char('n')) {
          ++index;
        }

        return index;
      }
    }

    bool decimal_point_seen = false;

    if (text.at(index) == QLatin1Char('.')) {
      decimal_point_seen = true;
      ++index;
    }

    while (index < length && (text.at(index).isDigit() || text.at(index) == QLatin1Char('_'))) {
      ++index;
    }

    if (!decimal_point_seen && index < length && text.at(index) == QLatin1Char('.')) {
      decimal_point_seen = true;
      ++index;

      while (index < length && (text.at(index).isDigit() || text.at(index) == QLatin1Char('_'))) {
        ++index;
      }
    }

    if (index < length && (text.at(index) == QLatin1Char('e') || text.at(index) == QLatin1Char('E'))) {
      int exponent_index = index + 1;

      if (exponent_index < length &&
          (text.at(exponent_index) == QLatin1Char('+') || text.at(exponent_index) == QLatin1Char('-'))) {
        ++exponent_index;
      }

      const int exponent_start = exponent_index;

      while (exponent_index < length &&
             (text.at(exponent_index).isDigit() || text.at(exponent_index) == QLatin1Char('_'))) {
        ++exponent_index;
      }

      if (exponent_index > exponent_start) {
        index = exponent_index;
      }
    }

    if (!decimal_point_seen && index < length && text.at(index) == QLatin1Char('n')) {
      ++index;
    }

    return index;
  }

  int scanRegularExpression(const QString& text, int start) {
    bool escaped = false;
    bool in_character_class = false;

    for (int index = start + 1; index < text.size(); ++index) {
      const QChar character = text.at(index);

      if (escaped) {
        escaped = false;
      }
      else if (character == QLatin1Char('\\')) {
        escaped = true;
      }
      else if (character == QLatin1Char('[')) {
        in_character_class = true;
      }
      else if (character == QLatin1Char(']')) {
        in_character_class = false;
      }
      else if (character == QLatin1Char('/') && !in_character_class) {
        ++index;

        while (index < text.size() && isIdentifierPart(text.at(index))) {
          ++index;
        }

        return index;
      }
    }

    return start + 1;
  }
} // namespace

JsSyntaxHighlighter::JsSyntaxHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
  for (const QString& keyword : jsKeywords()) {
    m_keywords.insert(keyword);
  }

  m_literals = {QSL("false"), QSL("Infinity"), QSL("NaN"), QSL("null"), QSL("true"), QSL("undefined")};

  const QPalette palette = QGuiApplication::palette();

  m_keywordFormat.setForeground(syntaxColor(palette, 300));
  m_keywordFormat.setFontWeight(QFont::Weight::Bold);

  m_classFormat.setForeground(syntaxColor(palette, 270));
  m_classFormat.setFontWeight(QFont::Weight::Bold);

  m_commentFormat.setForeground(commentColor(palette));
  m_quotationFormat.setForeground(syntaxColor(palette, 135));

  m_functionFormat.setForeground(syntaxColor(palette, 190));
  m_functionFormat.setFontItalic(true);

  m_numberFormat.setForeground(syntaxColor(palette, 40));
  m_regexFormat.setForeground(syntaxColor(palette, 15));
}

QStringList JsSyntaxHighlighter::jsKeywords() const {
  return {QSL("as"),     QSL("async"),   QSL("await"),      QSL("break"),     QSL("case"),    QSL("catch"),
          QSL("class"),  QSL("const"),   QSL("continue"),   QSL("debugger"),  QSL("default"), QSL("delete"),
          QSL("do"),     QSL("else"),    QSL("enum"),       QSL("export"),    QSL("extends"), QSL("finally"),
          QSL("for"),    QSL("from"),    QSL("function"),   QSL("get"),       QSL("if"),      QSL("implements"),
          QSL("import"), QSL("in"),      QSL("instanceof"), QSL("interface"), QSL("let"),     QSL("new"),
          QSL("of"),     QSL("package"), QSL("private"),    QSL("protected"), QSL("public"),  QSL("return"),
          QSL("set"),    QSL("static"),  QSL("super"),      QSL("switch"),    QSL("this"),    QSL("throw"),
          QSL("try"),    QSL("typeof"),  QSL("var"),        QSL("void"),      QSL("while"),   QSL("with"),
          QSL("yield")};
}

void JsSyntaxHighlighter::highlightBlock(const QString& text) {
  setCurrentBlockState(STATE_NORMAL);

  int index = 0;
  int template_depth = 0;
  int continuation_state = previousBlockState();

  if (continuation_state >= STATE_NESTED_TEMPLATE_STRING) {
    template_depth = continuation_state - STATE_NESTED_TEMPLATE_STRING;
    continuation_state = STATE_NESTED_TEMPLATE_STRING;
  }
  else if (continuation_state >= STATE_TEMPLATE_DOUBLE_QUOTED_STRING) {
    template_depth = continuation_state - STATE_TEMPLATE_DOUBLE_QUOTED_STRING;
    continuation_state = STATE_DOUBLE_QUOTED_STRING;
  }
  else if (continuation_state >= STATE_TEMPLATE_SINGLE_QUOTED_STRING) {
    template_depth = continuation_state - STATE_TEMPLATE_SINGLE_QUOTED_STRING;
    continuation_state = STATE_SINGLE_QUOTED_STRING;
  }
  else if (continuation_state >= STATE_TEMPLATE_BLOCK_COMMENT) {
    template_depth = continuation_state - STATE_TEMPLATE_BLOCK_COMMENT;
    continuation_state = STATE_BLOCK_COMMENT;
  }
  else if (continuation_state >= STATE_TEMPLATE_EXPRESSION) {
    template_depth = continuation_state - STATE_TEMPLATE_EXPRESSION;
    continuation_state = STATE_NORMAL;
  }
  else if (continuation_state < STATE_NORMAL) {
    continuation_state = STATE_NORMAL;
  }

  auto continueBlockComment = [&](int start) {
    const int end = text.indexOf(QSL("*/"), start);

    if (end < 0) {
      setFormat(start, text.size() - start, m_commentFormat);
      setCurrentBlockState(template_depth > 0 ? encodedState(STATE_TEMPLATE_BLOCK_COMMENT, template_depth)
                                              : STATE_BLOCK_COMMENT);
      index = text.size();
      return false;
    }

    index = end + 2;
    setFormat(start, index - start, m_commentFormat);
    return true;
  };

  auto continueQuotedString = [&](int start, QChar quote, bool starts_with_quote) {
    bool escaped = false;
    int cursor = start + int(starts_with_quote);

    while (cursor < text.size()) {
      const QChar character = text.at(cursor);

      if (escaped) {
        escaped = false;
      }
      else if (character == QLatin1Char('\\')) {
        escaped = true;
      }
      else if (character == quote) {
        ++cursor;
        setFormat(start, cursor - start, m_quotationFormat);
        index = cursor;
        return true;
      }

      ++cursor;
    }

    setFormat(start, text.size() - start, m_quotationFormat);
    index = text.size();

    if (escaped) {
      const int string_state = quote == QLatin1Char('\'') ? STATE_SINGLE_QUOTED_STRING : STATE_DOUBLE_QUOTED_STRING;
      const int template_string_state =
        quote == QLatin1Char('\'') ? STATE_TEMPLATE_SINGLE_QUOTED_STRING : STATE_TEMPLATE_DOUBLE_QUOTED_STRING;

      setCurrentBlockState(template_depth > 0 ? encodedState(template_string_state, template_depth) : string_state);
    }
    else if (template_depth > 0) {
      setCurrentBlockState(encodedState(STATE_TEMPLATE_EXPRESSION, template_depth));
    }

    return false;
  };

  auto continueNestedTemplate = [&](int start, bool starts_with_quote) {
    bool escaped = false;
    int cursor = start + int(starts_with_quote);

    while (cursor < text.size()) {
      const QChar character = text.at(cursor);

      if (escaped) {
        escaped = false;
      }
      else if (character == QLatin1Char('\\')) {
        escaped = true;
      }
      else if (character == QLatin1Char('`')) {
        ++cursor;
        setFormat(start, cursor - start, m_quotationFormat);
        index = cursor;
        return true;
      }

      ++cursor;
    }

    setFormat(start, text.size() - start, m_quotationFormat);
    setCurrentBlockState(encodedState(STATE_NESTED_TEMPLATE_STRING, template_depth));
    index = text.size();
    return false;
  };

  auto continueTemplate = [&](int start, bool starts_with_quote) {
    bool escaped = false;
    int cursor = start + int(starts_with_quote);

    while (cursor < text.size()) {
      const QChar character = text.at(cursor);

      if (escaped) {
        escaped = false;
      }
      else if (character == QLatin1Char('\\')) {
        escaped = true;
      }
      else if (character == QLatin1Char('`')) {
        ++cursor;
        setFormat(start, cursor - start, m_quotationFormat);
        index = cursor;
        return true;
      }
      else if (character == QLatin1Char('$') && cursor + 1 < text.size() && text.at(cursor + 1) == QLatin1Char('{')) {
        cursor += 2;
        setFormat(start, cursor - start, m_quotationFormat);
        template_depth = 1;
        index = cursor;
        return true;
      }

      ++cursor;
    }

    setFormat(start, text.size() - start, m_quotationFormat);
    setCurrentBlockState(STATE_TEMPLATE_STRING);
    index = text.size();
    return false;
  };

  bool regular_expression_allowed = true;

  if (continuation_state == STATE_BLOCK_COMMENT && !continueBlockComment(0)) {
    return;
  }
  else if (continuation_state == STATE_SINGLE_QUOTED_STRING) {
    if (!continueQuotedString(0, QLatin1Char('\''), false)) {
      return;
    }

    regular_expression_allowed = false;
  }
  else if (continuation_state == STATE_DOUBLE_QUOTED_STRING) {
    if (!continueQuotedString(0, QLatin1Char('"'), false)) {
      return;
    }

    regular_expression_allowed = false;
  }
  else if (continuation_state == STATE_TEMPLATE_STRING) {
    if (!continueTemplate(0, false)) {
      return;
    }

    regular_expression_allowed = template_depth > 0;
  }
  else if (continuation_state == STATE_NESTED_TEMPLATE_STRING) {
    if (!continueNestedTemplate(0, false)) {
      return;
    }

    regular_expression_allowed = false;
  }

  static const QSet<QString> expression_prefix_keywords = {QSL("await"),
                                                           QSL("case"),
                                                           QSL("delete"),
                                                           QSL("do"),
                                                           QSL("else"),
                                                           QSL("in"),
                                                           QSL("instanceof"),
                                                           QSL("new"),
                                                           QSL("return"),
                                                           QSL("throw"),
                                                           QSL("typeof"),
                                                           QSL("void"),
                                                           QSL("yield")};

  while (index < text.size()) {
    const QChar character = text.at(index);

    if (character.isSpace()) {
      ++index;
      continue;
    }

    if (template_depth > 0 && character == QLatin1Char('{')) {
      ++template_depth;
      ++index;
      regular_expression_allowed = true;
      continue;
    }

    if (template_depth > 0 && character == QLatin1Char('}')) {
      --template_depth;
      ++index;

      if (template_depth == 0) {
        setFormat(index - 1, 1, m_quotationFormat);

        if (!continueTemplate(index, false)) {
          return;
        }
      }

      regular_expression_allowed = template_depth > 0;
      continue;
    }

    if (character == QLatin1Char('/') && index + 1 < text.size()) {
      const QChar next_character = text.at(index + 1);

      if (next_character == QLatin1Char('/')) {
        setFormat(index, text.size() - index, m_commentFormat);

        if (template_depth > 0) {
          setCurrentBlockState(encodedState(STATE_TEMPLATE_EXPRESSION, template_depth));
        }

        return;
      }

      if (next_character == QLatin1Char('*')) {
        if (!continueBlockComment(index)) {
          return;
        }

        continue;
      }

      if (regular_expression_allowed) {
        const int end = scanRegularExpression(text, index);

        if (end > index + 1) {
          setFormat(index, end - index, m_regexFormat);
          index = end;
          regular_expression_allowed = false;
          continue;
        }
      }
    }

    if (character == QLatin1Char('\'') || character == QLatin1Char('"')) {
      if (!continueQuotedString(index, character, true)) {
        return;
      }

      regular_expression_allowed = false;
      continue;
    }

    if (character == QLatin1Char('`')) {
      if (template_depth > 0) {
        if (!continueNestedTemplate(index, true)) {
          return;
        }

        regular_expression_allowed = false;
      }
      else {
        if (!continueTemplate(index, true)) {
          return;
        }

        regular_expression_allowed = template_depth > 0;
      }

      continue;
    }

    if (character.isDigit() ||
        (character == QLatin1Char('.') && index + 1 < text.size() && text.at(index + 1).isDigit())) {
      const int end = scanNumber(text, index);
      setFormat(index, end - index, m_numberFormat);
      index = end;
      regular_expression_allowed = false;
      continue;
    }

    if (isIdentifierStart(character)) {
      const int start = index++;

      while (index < text.size() && isIdentifierPart(text.at(index))) {
        ++index;
      }

      const QString identifier = text.mid(start, index - start);

      if (m_keywords.contains(identifier)) {
        setFormat(start, index - start, m_keywordFormat);
        regular_expression_allowed = expression_prefix_keywords.contains(identifier);
      }
      else if (m_literals.contains(identifier)) {
        setFormat(start, index - start, m_numberFormat);
        regular_expression_allowed = false;
      }
      else {
        int next_non_space = index;

        while (next_non_space < text.size() && text.at(next_non_space).isSpace()) {
          ++next_non_space;
        }

        if (next_non_space < text.size() && text.at(next_non_space) == QLatin1Char('(')) {
          setFormat(start, index - start, m_functionFormat);
        }
        else if (identifier.at(0).isUpper()) {
          setFormat(start, index - start, m_classFormat);
        }

        regular_expression_allowed = false;
      }

      continue;
    }

    if ((character == QLatin1Char('+') || character == QLatin1Char('-')) && index + 1 < text.size() &&
        text.at(index + 1) == character) {
      index += 2;
      regular_expression_allowed = false;
      continue;
    }

    regular_expression_allowed = character != QLatin1Char(')') && character != QLatin1Char(']') &&
                                 character != QLatin1Char('}') && character != QLatin1Char('.');
    ++index;
  }

  if (template_depth > 0) {
    setCurrentBlockState(encodedState(STATE_TEMPLATE_EXPRESSION, template_depth));
  }
}
