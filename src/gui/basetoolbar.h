#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>


class BaseToolBar : public QToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseToolBar(const QString &title, QWidget *parent = 0);
    virtual ~BaseToolBar();

    virtual QList<QAction*> changeableActions() const = 0;
    virtual void saveChangeableActions() const = 0;
    virtual void loadChangeableActions() = 0;

  signals:

  public slots:

};

#endif // TOOLBAR_H
