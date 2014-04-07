#ifndef FEEDSTOOLBAR_H
#define FEEDSTOOLBAR_H

#include "gui/basetoolbar.h"


class FeedsToolBar : public BaseToolBar {
  public:
    // Constructors and destructors.
    explicit FeedsToolBar(const QString &title, QWidget *parent = 0);
    virtual ~FeedsToolBar();

    QHash<QString, QAction*> availableActions() const;
    QList<QAction*> changeableActions() const;
    void saveChangeableActions(const QStringList &actions);
    void loadChangeableActions();

    // Loads actions as specified by external actions list.
    // NOTE: This is used primarily for reloading actions
    // when they are changed from settings.
    void loadChangeableActions(const QStringList &actions);
};

#endif // FEEDSTOOLBAR_H
