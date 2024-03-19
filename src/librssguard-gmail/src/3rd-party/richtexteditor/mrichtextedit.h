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

#ifndef MRICHTEXTEDIT_H_
#define MRICHTEXTEDIT_H_

#include "ui_mrichtextedit.h"

#include <QPointer>

class MRichTextEdit : public QWidget {
    Q_OBJECT
  public:
    MRichTextEdit(QWidget* parent = nullptr);

    QString toPlainText() const;
    QString toHtml() const;

    MTextEdit* editor() const;
    QTextDocument* document();
    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor& cursor);

  public slots:
    void setText(const QString& text);
    void clearSource();

  protected slots:
    void setPlainText(const QString& text);
    void setHtml(const QString& text);
    void textRemoveFormat();
    void textRemoveAllFormat();
    void setTextBold();
    void setTextUnderline();
    void setTextStrikeout();
    void setTextItalic();
    void textSize(const QString& p);
    void setTextLink(bool checked);
    void setTextStyle(int index);
    void textFgColor(const QColor& color);
    void textBgColor(const QColor& color);
    void listBullet(bool checked);
    void listOrdered(bool checked);
    void onCurrentCharFormatChanged(const QTextCharFormat& format);
    void onCursorPositionChanged();
    void onClipboardDataChanged();
    void increaseIndentation();
    void decreaseIndentation();
    void insertImage();
    void textSource();

  protected:
    void mergeFormatOnWordOrSelection(const QTextCharFormat& format);
    void fontChanged(const QFont& f);
    void list(bool checked, QTextListFormat::Style style);
    void indent(int delta);
    void focusInEvent(QFocusEvent* event);

  private:
    void setupIcons();

  private:
    int m_fontsize_h1;
    int m_fontsize_h2;
    int m_fontsize_h3;
    int m_fontsize_h4;

    enum ParagraphItems {
      ParagraphStandard = 0,
      ParagraphHeading1,
      ParagraphHeading2,
      ParagraphHeading3,
      ParagraphHeading4,
      ParagraphMonospace
    };

    QPointer<QTextList> m_lastBlockList;

  private:
    Ui::MRichTextEdit m_ui;
};

#endif
