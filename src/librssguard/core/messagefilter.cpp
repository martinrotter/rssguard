// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagefilter.h"

MessageFilter::MessageFilter(QObject* parent) : QObject(parent) {}

FilteringAction MessageFilter::filterMessage() {
  return FilteringAction::Accept;
}
