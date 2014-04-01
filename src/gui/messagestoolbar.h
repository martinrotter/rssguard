#ifndef NEWSTOOLBAR_H
#define NEWSTOOLBAR_H

#include "gui/basetoolbar.h"


class MessagesToolBar : public BaseToolBar {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessagesToolBar(const QString &title, QWidget *parent = 0);
    virtual ~MessagesToolBar();

    QList<QAction*> changeableActions() const {
      // TODO: Vracet akce, ktere muze uzivatel upravovat v tomto toolbaru.
      // nebudou se tedy vracet spacer widgety nebo lineedity a tak podobně,
      // proste jen akce ktere sou uzivatelsky upravitelne
      // http://stackoverflow.com/questions/5364957/in-qt-4-7-how-can-a-pop-up-menu-be-added-to-a-qtoolbar-button
      // http://www.qtcentre.org/threads/23840-how-align-some-buttons-in-QToolbar-from-right-to-left
      return QList<QAction*>();
    }

    void setChangeableActions(const QList<QAction *> actions) {
      // TODO: ulozit akce, ktere muze uzivatel upravovat do tohoto toolbaru
      // todle musi zachovat vsechny widgety na konci - treba filtrovaci
      // lineedit zprav
    }

    void saveChangeableActions() const {
    }

    void loadChangeableActions() {
    }

  signals:

  public slots:

};

#endif // NEWSTOOLBAR_H
