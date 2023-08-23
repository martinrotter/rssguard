// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SEARCHTEXTWIDGET_H
#define SEARCHTEXTWIDGET_H

#include <QWidget>

#include "ui_searchtextwidget.h"

namespace Ui {
  class SearchTextWidget;
}

class SearchTextWidget : public QWidget {
  Q_OBJECT

  public:
    explicit SearchTextWidget(QWidget* parent = nullptr);

  public slots:
    void clear();
    void cancelSearch();

  private slots:
    void onTextChanged(const QString& text);

  protected:
    void keyPressEvent(QKeyEvent* event);
    void focusInEvent(QFocusEvent* event);

  signals:
    void searchForText(QString text, bool search_backwards);
    void searchCancelled();

  private:
    Ui::SearchTextWidget m_ui;
};

#endif // SEARCHTEXTWIDGET_H
