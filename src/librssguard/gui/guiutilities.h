// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GUIUTILITIES_H
#define GUIUTILITIES_H

#include <QIcon>
#include <QLabel>
#include <QWidget>

class GuiUtilities {
  public:
    static void setLabelAsNotice(QLabel& label, bool is_warning, bool set_margins = true);
    static void applyDialogProperties(QWidget& widget, const QIcon& icon = QIcon(), const QString& title = QString());
    static void restoreState(QWidget* wdg, QByteArray state);
    static QByteArray saveState(QWidget* wdg);

  private:
    explicit GuiUtilities();
};

inline GuiUtilities::GuiUtilities() {}

#endif // GUIUTILITIES_H
