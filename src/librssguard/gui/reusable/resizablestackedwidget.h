// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef RESIZABLESTACKEDWIDGET_H
#define RESIZABLESTACKEDWIDGET_H

#include <QStackedWidget>

class RSSGUARD_DLLSPEC ResizableStackedWidget : public QStackedWidget {
  public:
    explicit ResizableStackedWidget(QWidget* parent = nullptr);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
};

#endif // RESIZABLESTACKEDWIDGET_H
