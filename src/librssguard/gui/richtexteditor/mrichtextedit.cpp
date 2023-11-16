// This work is derived from MRichTextEditor.
//
// For license of this file, see <project-root-folder>/resources/text/COPYING_GNU_LGPL_21.

/*
** Copyright (C) 2013 Jiří Procházka (Hobrasoft)
** Contact: http://www.hobrasoft.cz/
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file is under the terms of the GNU Lesser General Public License
** version 2.1 as published by the Free Software Foundation and appearing
** in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the
** GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
*/

#include "gui/richtexteditor/mrichtextedit.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QColorDialog>
#include <QDialog>
#include <QFileDialog>
#include <QFontDatabase>
#include <QImageReader>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTextList>
#include <QUrl>
#include <QtDebug>

MRichTextEdit::MRichTextEdit(QWidget* parent) : QWidget(parent) {
  m_ui.setupUi(this);
  m_lastBlockList = nullptr;

  setupIcons();

  connect(m_ui.f_textedit, &MTextEdit::currentCharFormatChanged, this, &MRichTextEdit::onCurrentCharFormatChanged);
  connect(m_ui.f_textedit, &MTextEdit::cursorPositionChanged, this, &MRichTextEdit::onCursorPositionChanged);

  m_fontsize_h1 = 18;
  m_fontsize_h2 = 16;
  m_fontsize_h3 = 14;
  m_fontsize_h4 = 12;

  // paragraph formatting
  m_ui.f_paragraph
    ->addItems({tr("Standard"), tr("Heading 1"), tr("Heading 2"), tr("Heading 3"), tr("Heading 4"), tr("Monospace")});

  connect(m_ui.f_paragraph, QOverload<int>::of(&QComboBox::activated), this, &MRichTextEdit::setTextStyle);

  // undo & redo
  m_ui.f_undo->setShortcut(QKeySequence::StandardKey::Undo);
  m_ui.f_redo->setShortcut(QKeySequence::StandardKey::Redo);

  connect(m_ui.f_textedit->document(), &QTextDocument::undoAvailable, m_ui.f_undo, &QWidget::setEnabled);
  connect(m_ui.f_textedit->document(), &QTextDocument::redoAvailable, m_ui.f_redo, &QWidget::setEnabled);

  m_ui.f_undo->setEnabled(m_ui.f_textedit->document()->isUndoAvailable());
  m_ui.f_redo->setEnabled(m_ui.f_textedit->document()->isRedoAvailable());

  connect(m_ui.f_undo, &PlainToolButton::clicked, m_ui.f_textedit, &QTextEdit::undo);
  connect(m_ui.f_redo, &PlainToolButton::clicked, m_ui.f_textedit, &QTextEdit::redo);

  // cut, copy & paste
  m_ui.f_cut->setShortcut(QKeySequence::StandardKey::Cut);
  m_ui.f_copy->setShortcut(QKeySequence::StandardKey::Copy);
  m_ui.f_paste->setShortcut(QKeySequence::StandardKey::Paste);

  m_ui.f_cut->setEnabled(false);
  m_ui.f_copy->setEnabled(false);

  connect(m_ui.f_cut, &PlainToolButton::clicked, m_ui.f_textedit, &QTextEdit::cut);
  connect(m_ui.f_copy, &PlainToolButton::clicked, m_ui.f_textedit, &QTextEdit::copy);
  connect(m_ui.f_paste, &PlainToolButton::clicked, m_ui.f_textedit, &QTextEdit::paste);

  connect(m_ui.f_textedit, &MTextEdit::copyAvailable, m_ui.f_cut, &QWidget::setEnabled);
  connect(m_ui.f_textedit, &MTextEdit::copyAvailable, m_ui.f_copy, &QWidget::setEnabled);

#ifndef QT_NO_CLIPBOARD
  connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &MRichTextEdit::onClipboardDataChanged);
#endif

  // link
  m_ui.f_link->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_L);

  connect(m_ui.f_link, &PlainToolButton::clicked, this, &MRichTextEdit::setTextLink);

  // bold, italic & underline
  m_ui.f_bold->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_B);
  m_ui.f_italic->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_I);
  m_ui.f_underline->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_U);

  connect(m_ui.f_bold, &PlainToolButton::clicked, this, &MRichTextEdit::setTextBold);
  connect(m_ui.f_italic, &PlainToolButton::clicked, this, &MRichTextEdit::setTextItalic);
  connect(m_ui.f_underline, &PlainToolButton::clicked, this, &MRichTextEdit::setTextUnderline);
  connect(m_ui.f_strikeout, &PlainToolButton::clicked, this, &MRichTextEdit::setTextStrikeout);

  QAction* removeFormat = new QAction(tr("Remove character formatting"), this);
  removeFormat->setShortcut(QKeySequence(QSL("CTRL+M")));
  connect(removeFormat, &QAction::triggered, this, &MRichTextEdit::textRemoveFormat);
  m_ui.f_textedit->addAction(removeFormat);

  QAction* removeAllFormat = new QAction(tr("Remove all formatting"), this);
  connect(removeAllFormat, &QAction::triggered, this, &MRichTextEdit::textRemoveAllFormat);
  m_ui.f_textedit->addAction(removeAllFormat);

  QAction* textsource = new QAction(tr("Edit document source"), this);
  textsource->setShortcut(QKeySequence(QSL("CTRL+O")));
  connect(textsource, &QAction::triggered, this, &MRichTextEdit::textSource);
  m_ui.f_textedit->addAction(textsource);

  QAction* clearText = new QAction(tr("Clear all content"), this);
  connect(clearText, &QAction::triggered, this, &MRichTextEdit::clearSource);
  m_ui.f_textedit->addAction(clearText);

  QMenu* menu = new QMenu(this);
  menu->addAction(removeAllFormat);
  menu->addAction(removeFormat);
  menu->addAction(textsource);
  menu->addAction(clearText);
  m_ui.f_menu->setMenu(menu);
  m_ui.f_menu->setPopupMode(QToolButton::InstantPopup);

  // lists

  m_ui.f_list_bullet->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_Minus);
  m_ui.f_list_ordered->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_Equal);

  connect(m_ui.f_list_bullet, &PlainToolButton::clicked, this, &MRichTextEdit::listBullet);
  connect(m_ui.f_list_ordered, &PlainToolButton::clicked, this, &MRichTextEdit::listOrdered);

  // indentation

  m_ui.f_indent_dec->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_Comma);
  m_ui.f_indent_inc->setShortcut(Qt::Modifier::CTRL | Qt::Key::Key_Period);

  connect(m_ui.f_indent_inc, &PlainToolButton::clicked, this, &MRichTextEdit::increaseIndentation);
  connect(m_ui.f_indent_dec, &PlainToolButton::clicked, this, &MRichTextEdit::decreaseIndentation);

  // font size
  for (int size : QFontDatabase::standardSizes()) {
    m_ui.f_fontsize->addItem(QString::number(size));
  }

  connect(m_ui.f_fontsize, &QComboBox::textActivated, this, &MRichTextEdit::textSize);
  m_ui.f_fontsize->setCurrentIndex(m_ui.f_fontsize->findText(QString::number(QApplication::font().pointSize())));

  // text foreground color
  m_ui.f_fgcolor->setAlternateColor(m_ui.f_textedit->textColor());
  m_ui.f_fgcolor->setColor(m_ui.f_textedit->textColor());
  connect(m_ui.f_fgcolor, &ColorToolButton::colorChanged, this, &MRichTextEdit::textFgColor);

  // text background color
  auto bg_col = m_ui.f_textedit->textBackgroundColor();

  bg_col.setAlpha(255);

  m_ui.f_bgcolor->setAlternateColor(bg_col);
  m_ui.f_bgcolor->setColor(bg_col);
  connect(m_ui.f_bgcolor, &ColorToolButton::colorChanged, this, &MRichTextEdit::textBgColor);

  // images
  connect(m_ui.f_image, &PlainToolButton::clicked, this, &MRichTextEdit::insertImage);
}

QString MRichTextEdit::toPlainText() const {
  return m_ui.f_textedit->toPlainText();
}

void MRichTextEdit::textSource() {
  QDialog* dialog = new QDialog(this);
  QPlainTextEdit* pte = new QPlainTextEdit(dialog);
  pte->setPlainText(m_ui.f_textedit->toHtml());
  QGridLayout* gl = new QGridLayout(dialog);
  gl->addWidget(pte, 0, 0, 1, 1);
  dialog->setWindowTitle(tr("Document source"));
  dialog->setMinimumWidth(400);
  dialog->setMinimumHeight(600);
  dialog->exec();

  m_ui.f_textedit->setHtml(pte->toPlainText());

  delete dialog;
}

void MRichTextEdit::clearSource() {
  m_ui.f_textedit->clear();
}

void MRichTextEdit::setPlainText(const QString& text) {
  m_ui.f_textedit->setPlainText(text);
}

void MRichTextEdit::setHtml(const QString& text) {
  m_ui.f_textedit->setHtml(text);
}

void MRichTextEdit::textRemoveFormat() {
  QTextCharFormat fmt;
  fmt.setFontWeight(QFont::Normal);
  fmt.setFontUnderline(false);
  fmt.setFontStrikeOut(false);
  fmt.setFontItalic(false);
  fmt.setFontPointSize(9);
  //  fmt.setFontFamily     ("Helvetica");
  //  fmt.setFontStyleHint  (QFont::SansSerif);
  //  fmt.setFontFixedPitch (true);

  m_ui.f_bold->setChecked(false);
  m_ui.f_underline->setChecked(false);
  m_ui.f_italic->setChecked(false);
  m_ui.f_strikeout->setChecked(false);
  m_ui.f_fontsize->setCurrentIndex(m_ui.f_fontsize->findText("9"));

  //  QTextBlockFormat bfmt = cursor.blockFormat();
  //  bfmt->setIndent(0);

  fmt.clearBackground();

  mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textRemoveAllFormat() {
  m_ui.f_bold->setChecked(false);
  m_ui.f_underline->setChecked(false);
  m_ui.f_italic->setChecked(false);
  m_ui.f_strikeout->setChecked(false);
  m_ui.f_fontsize->setCurrentIndex(m_ui.f_fontsize->findText("9"));
  QString text = m_ui.f_textedit->toPlainText();
  m_ui.f_textedit->setPlainText(text);
}

void MRichTextEdit::setTextBold() {
  QTextCharFormat fmt;
  fmt.setFontWeight(m_ui.f_bold->isChecked() ? QFont::Bold : QFont::Normal);
  mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::focusInEvent(QFocusEvent*) {
  m_ui.f_textedit->setFocus(Qt::TabFocusReason);
}

void MRichTextEdit::setupIcons() {
  auto* ic = qApp->icons();

  m_ui.f_bgcolor->setIcon(ic->fromTheme(QSL("format-text-color")));
  m_ui.f_bold->setIcon(ic->fromTheme(QSL("format-text-bold")));
  m_ui.f_copy->setIcon(ic->fromTheme(QSL("edit-copy")));
  m_ui.f_cut->setIcon(ic->fromTheme(QSL("edit-cut")));
  m_ui.f_fgcolor->setIcon(ic->fromTheme(QSL("format-text-color")));
  m_ui.f_image->setIcon(ic->fromTheme(QSL("image-x-generic")));
  m_ui.f_indent_dec->setIcon(ic->fromTheme(QSL("format-indent-less")));
  m_ui.f_indent_inc->setIcon(ic->fromTheme(QSL("format-indent-more")));
  m_ui.f_italic->setIcon(ic->fromTheme(QSL("format-text-italic")));
  m_ui.f_link->setIcon(ic->fromTheme(QSL("insert-link")));
  m_ui.f_list_bullet->setIcon(ic->fromTheme(QSL("format-list-unordered")));
  m_ui.f_list_ordered->setIcon(ic->fromTheme(QSL("format-list-ordered")));
  m_ui.f_menu->setIcon(ic->fromTheme(QSL("go-home")));
  m_ui.f_paste->setIcon(ic->fromTheme(QSL("edit-paste")));
  m_ui.f_redo->setIcon(ic->fromTheme(QSL("edit-redo")));
  m_ui.f_strikeout->setIcon(ic->fromTheme(QSL("format-text-strikethrough")));
  m_ui.f_underline->setIcon(ic->fromTheme(QSL("format-text-underline")));
  m_ui.f_undo->setIcon(ic->fromTheme(QSL("edit-undo")));
}

void MRichTextEdit::setTextUnderline() {
  QTextCharFormat fmt;
  fmt.setFontUnderline(m_ui.f_underline->isChecked());
  mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::setTextItalic() {
  QTextCharFormat fmt;
  fmt.setFontItalic(m_ui.f_italic->isChecked());
  mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::setTextStrikeout() {
  QTextCharFormat fmt;
  fmt.setFontStrikeOut(m_ui.f_strikeout->isChecked());
  mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textSize(const QString& p) {
  qreal pointSize = p.toDouble();
  if (p.toFloat() > 0) {
    QTextCharFormat fmt;
    fmt.setFontPointSize(pointSize);
    mergeFormatOnWordOrSelection(fmt);
  }
}

void MRichTextEdit::setTextLink(bool checked) {
  bool unlink = false;
  QTextCharFormat fmt;
  if (checked) {
    QString url = m_ui.f_textedit->currentCharFormat().anchorHref();
    bool ok;
    QString newUrl = QInputDialog::getText(this, tr("Create a link"), tr("Link URL:"), QLineEdit::Normal, url, &ok);
    if (ok) {
      fmt.setAnchor(true);
      fmt.setAnchorHref(newUrl);
      fmt.setForeground(QApplication::palette().color(QPalette::Link));
      fmt.setFontUnderline(true);
    }
    else {
      unlink = true;
    }
  }
  else {
    unlink = true;
  }
  if (unlink) {
    fmt.setAnchor(false);
    fmt.setForeground(QApplication::palette().color(QPalette::Text));
    fmt.setFontUnderline(false);
  }
  mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::setTextStyle(int index) {
  QTextCursor cursor = m_ui.f_textedit->textCursor();
  cursor.beginEditBlock();

  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::SelectionType::BlockUnderCursor);
  }

  QTextCharFormat fmt;
  cursor.setCharFormat(fmt);
  m_ui.f_textedit->setCurrentCharFormat(fmt);

  if (index == ParagraphItems::ParagraphHeading1 || index == ParagraphItems::ParagraphHeading2 ||
      index == ParagraphItems::ParagraphHeading3 || index == ParagraphItems::ParagraphHeading4) {
    if (index == ParagraphHeading1) {
      fmt.setFontPointSize(m_fontsize_h1);
    }

    if (index == ParagraphHeading2) {
      fmt.setFontPointSize(m_fontsize_h2);
    }

    if (index == ParagraphHeading3) {
      fmt.setFontPointSize(m_fontsize_h3);
    }

    if (index == ParagraphHeading4) {
      fmt.setFontPointSize(m_fontsize_h4);
    }

    if (index == ParagraphItems::ParagraphHeading2 || index == ParagraphItems::ParagraphHeading4) {
      fmt.setFontItalic(true);
    }

    fmt.setFontWeight(QFont::Bold);
  }

  if (index == ParagraphItems::ParagraphMonospace) {
    fmt = cursor.charFormat();
    fmt.setFontFamilies({QSL("Monospace")});
    fmt.setFontStyleHint(QFont::StyleHint::Monospace);
    fmt.setFontFixedPitch(true);
  }

  cursor.setCharFormat(fmt);
  m_ui.f_textedit->setCurrentCharFormat(fmt);

  cursor.endEditBlock();
}

void MRichTextEdit::textFgColor(const QColor& color) {
  QTextCursor cursor = m_ui.f_textedit->textCursor();

  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::SelectionType::WordUnderCursor);
  }

  QTextCharFormat fmt = cursor.charFormat();

  if (color.isValid()) {
    fmt.setForeground(color);
  }
  else {
    fmt.clearForeground();
  }

  cursor.setCharFormat(fmt);
  m_ui.f_textedit->setCurrentCharFormat(fmt);
}

void MRichTextEdit::textBgColor(const QColor& color) {
  QTextCursor cursor = m_ui.f_textedit->textCursor();

  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::SelectionType::WordUnderCursor);
  }

  QTextCharFormat fmt = cursor.charFormat();

  if (color.isValid()) {
    fmt.setBackground(color);
  }
  else {
    fmt.clearBackground();
  }

  cursor.setCharFormat(fmt);
  m_ui.f_textedit->setCurrentCharFormat(fmt);
}

void MRichTextEdit::listBullet(bool checked) {
  if (checked) {
    m_ui.f_list_ordered->setChecked(false);
  }
  list(checked, QTextListFormat::ListDisc);
}

void MRichTextEdit::listOrdered(bool checked) {
  if (checked) {
    m_ui.f_list_bullet->setChecked(false);
  }
  list(checked, QTextListFormat::ListDecimal);
}

void MRichTextEdit::list(bool checked, QTextListFormat::Style style) {
  QTextCursor cursor = m_ui.f_textedit->textCursor();
  cursor.beginEditBlock();
  if (!checked) {
    QTextBlockFormat obfmt = cursor.blockFormat();
    QTextBlockFormat bfmt;
    bfmt.setIndent(obfmt.indent());
    cursor.setBlockFormat(bfmt);
  }
  else {
    QTextListFormat listFmt;
    if (cursor.currentList()) {
      listFmt = cursor.currentList()->format();
    }
    listFmt.setStyle(style);
    cursor.createList(listFmt);
  }
  cursor.endEditBlock();
}

void MRichTextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat& format) {
  QTextCursor cursor = m_ui.f_textedit->textCursor();
  if (!cursor.hasSelection()) {
    cursor.select(QTextCursor::WordUnderCursor);
  }
  cursor.mergeCharFormat(format);
  m_ui.f_textedit->mergeCurrentCharFormat(format);
  m_ui.f_textedit->setFocus(Qt::TabFocusReason);
}

void MRichTextEdit::onCursorPositionChanged() {
  QTextList* l = m_ui.f_textedit->textCursor().currentList();

  if (m_lastBlockList && (l == m_lastBlockList || (l != nullptr && m_lastBlockList != nullptr &&
                                                   l->format().style() == m_lastBlockList->format().style()))) {
    return;
  }

  m_lastBlockList = l;

  if (l != nullptr) {
    QTextListFormat lfmt = l->format();

    if (lfmt.style() == QTextListFormat::Style::ListDisc) {
      m_ui.f_list_bullet->setChecked(true);
      m_ui.f_list_ordered->setChecked(false);
    }
    else if (lfmt.style() == QTextListFormat::Style::ListDecimal) {
      m_ui.f_list_bullet->setChecked(false);
      m_ui.f_list_ordered->setChecked(true);
    }
    else {
      m_ui.f_list_bullet->setChecked(false);
      m_ui.f_list_ordered->setChecked(false);
    }
  }
  else {
    m_ui.f_list_bullet->setChecked(false);
    m_ui.f_list_ordered->setChecked(false);
  }
}

void MRichTextEdit::fontChanged(const QFont& f) {
  m_ui.f_fontsize->setCurrentIndex(m_ui.f_fontsize->findText(QString::number(f.pointSize())));
  m_ui.f_bold->setChecked(f.bold());
  m_ui.f_italic->setChecked(f.italic());
  m_ui.f_underline->setChecked(f.underline());
  m_ui.f_strikeout->setChecked(f.strikeOut());

  if (f.pointSize() == m_fontsize_h1) {
    m_ui.f_paragraph->setCurrentIndex(ParagraphHeading1);
  }
  else if (f.pointSize() == m_fontsize_h2) {
    m_ui.f_paragraph->setCurrentIndex(ParagraphHeading2);
  }
  else if (f.pointSize() == m_fontsize_h3) {
    m_ui.f_paragraph->setCurrentIndex(ParagraphHeading3);
  }
  else if (f.pointSize() == m_fontsize_h4) {
    m_ui.f_paragraph->setCurrentIndex(ParagraphHeading4);
  }
  else {
    if (f.fixedPitch() && f.family() == "Monospace") {
      m_ui.f_paragraph->setCurrentIndex(ParagraphMonospace);
    }
    else {
      m_ui.f_paragraph->setCurrentIndex(ParagraphStandard);
    }
  }
  if (m_ui.f_textedit->textCursor().currentList()) {
    QTextListFormat lfmt = m_ui.f_textedit->textCursor().currentList()->format();
    if (lfmt.style() == QTextListFormat::ListDisc) {
      m_ui.f_list_bullet->setChecked(true);
      m_ui.f_list_ordered->setChecked(false);
    }
    else if (lfmt.style() == QTextListFormat::ListDecimal) {
      m_ui.f_list_bullet->setChecked(false);
      m_ui.f_list_ordered->setChecked(true);
    }
    else {
      m_ui.f_list_bullet->setChecked(false);
      m_ui.f_list_ordered->setChecked(false);
    }
  }
  else {
    m_ui.f_list_bullet->setChecked(false);
    m_ui.f_list_ordered->setChecked(false);
  }
}

void MRichTextEdit::onCurrentCharFormatChanged(const QTextCharFormat& format) {
  fontChanged(format.font());
  m_ui.f_bgcolor->setColor(format.background().isOpaque() ? format.background().color()
                                                          : m_ui.f_bgcolor->alternateColor(),
                           false);
  m_ui.f_fgcolor->setColor(format.foreground().isOpaque() ? format.foreground().color()
                                                          : m_ui.f_fgcolor->alternateColor(),
                           false);
  m_ui.f_link->setChecked(format.isAnchor());
}

void MRichTextEdit::onClipboardDataChanged() {
#ifndef QT_NO_CLIPBOARD
  if (const QMimeData* md = QApplication::clipboard()->mimeData()) {
    m_ui.f_paste->setEnabled(md->hasText());
  }
#endif
}

QString MRichTextEdit::toHtml() const {
  QString s = m_ui.f_textedit->toHtml();
  // convert emails to links
  s = s.replace(QRegularExpression("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)([a-zA-Z\\d]+@[a-zA-Z\\d]+\\.[a-zA-Z]+)"),
                "\\1<a href=\"mailto:\\2\">\\2</a>");
  // convert links
  s = s.replace(QRegularExpression("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)((?:https?|ftp|file)://[^\\s'\"<>]+)"),
                "\\1<a href=\"\\2\">\\2</a>");
  // see also: Utils::linkify()
  return s;
}

MTextEdit* MRichTextEdit::editor() const {
  return m_ui.f_textedit;
}

QTextDocument* MRichTextEdit::document() {
  return m_ui.f_textedit->document();
}

QTextCursor MRichTextEdit::textCursor() const {
  return m_ui.f_textedit->textCursor();
}

void MRichTextEdit::setTextCursor(const QTextCursor& cursor) {
  m_ui.f_textedit->setTextCursor(cursor);
}

void MRichTextEdit::increaseIndentation() {
  indent(+1);
}

void MRichTextEdit::decreaseIndentation() {
  indent(-1);
}

void MRichTextEdit::indent(int delta) {
  QTextCursor cursor = m_ui.f_textedit->textCursor();
  cursor.beginEditBlock();
  QTextBlockFormat bfmt = cursor.blockFormat();
  int ind = bfmt.indent();
  if (ind + delta >= 0) {
    bfmt.setIndent(ind + delta);
  }
  cursor.setBlockFormat(bfmt);
  cursor.endEditBlock();
}

void MRichTextEdit::setText(const QString& text) {
  if (text.isEmpty()) {
    setPlainText(text);
    return;
  }
  if (text[0] == '<') {
    setHtml(text);
  }
  else {
    setPlainText(text);
  }
}

void MRichTextEdit::insertImage() {
  QSettings s;
  QString attdir = s.value("general/filedialog-path").toString();
  QString file = QFileDialog::getOpenFileName(this,
                                              tr("Select an image"),
                                              attdir,
                                              tr("JPEG (*.jpg);; GIF (*.gif);; PNG (*.png);; BMP (*.bmp);; All (*)"));
  QImage image = QImageReader(file).read();

  m_ui.f_textedit->dropImage(image, QFileInfo(file).suffix().toUpper().toLocal8Bit().data());
}
