// For license of this file, see <project-root-folder>/LICENSE.md.

#include "guinotificationstub.h"

#include "miscellaneous/guinotificationcoordinator.h"

#include <atomic>

namespace {

  std::atomic_bool allow_fetch_error{false};

} // namespace

void allowNextArticlesFetchingErrorNotification(bool allow) {
  allow_fetch_error = allow;
}

// Intercept notifications so the filtering tests can run without showing popups.
void GuiNotificationCoordinator::showGuiMessage(Notification::Event event,
                                                const GuiMessage&,
                                                const GuiMessageDestination&,
                                                const GuiAction&,
                                                QWidget*) {
  if (event == Notification::Event::ArticlesFetchingError && allow_fetch_error.exchange(false)) {
    return;
  }

  qFatal("Unexpected GUI notification in filtering test.");
}
