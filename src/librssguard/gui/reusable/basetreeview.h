// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include <QTreeView>

class BaseTreeView : public QTreeView {
  Q_OBJECT

  public:
    explicit BaseTreeView(QWidget* parent = nullptr);

    bool keyboardShortcutsLimited() const;
    void setKeyboardShortcutsLimited(bool limited);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    bool m_keyboardShortcutsLimited;
    QList<int> m_allowedKeyboardKeys;
};

#endif // BASETREEVIEW_H
