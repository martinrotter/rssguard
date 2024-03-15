// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GUIUTILITIES_H
#define GUIUTILITIES_H

#include <QIcon>
#include <QLabel>
#include <QWidget>

class RSSGUARD_DLLSPEC GuiUtilities {
  public:
    static void setLabelAsNotice(QLabel& label, bool is_warning, bool set_margins = true);
    static void applyDialogProperties(QWidget& widget, const QIcon& icon = QIcon(), const QString& title = QString());
    static void fixTooBigDialog(QWidget& widget, bool move_to_center = true);
    static void restoreState(QWidget* wdg, QByteArray state);
    static QByteArray saveState(QWidget* wdg);

    static void loadDialogSize(QWidget& wdg);
    static void saveSizeOnWidgetClosed(QWidget& wdg);

  private:
    explicit GuiUtilities();
};

inline GuiUtilities::GuiUtilities() {}

#endif // GUIUTILITIES_H
