// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/guinotificationcoordinator.h"

// The filtering tests run without a GUI. All filtering and storage code is real;
// requesting a notification is unexpected and must not silently succeed.
void GuiNotificationCoordinator::showGuiMessage(Notification::Event,
                                                const GuiMessage&,
                                                const GuiMessageDestination&,
                                                const GuiAction&,
                                                QWidget*) {
  qFatal("Unexpected GUI notification in filtering test.");
}
