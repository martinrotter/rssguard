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

  private:
    Ui::SearchTextWidget m_ui;
};

#endif // SEARCHTEXTWIDGET_H
