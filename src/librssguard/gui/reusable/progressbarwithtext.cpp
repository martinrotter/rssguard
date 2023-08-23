// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/progressbarwithtext.h"

#include "definitions/definitions.h"

#include <QLocale>

ProgressBarWithText::ProgressBarWithText(QWidget* parent) : QProgressBar(parent) {}

QString ProgressBarWithText::text() const {
  qint64 total_steps = qint64(maximum()) - minimum();
  QString result = format();
  QLocale locale;

  locale.setNumberOptions(locale.numberOptions() | QLocale::OmitGroupSeparator);
  result.replace(QL1S("%m"), locale.toString(total_steps));
  result.replace(QL1S("%v"), locale.toString(value()));

  // If max and min are equal and we get this far, it means that the
  // progress bar has one step and that we are on that step. Return
  // 100% here in order to avoid division by zero further down.
  if (total_steps == 0) {
    result.replace(QL1S("%p"), locale.toString(100));
    return result;
  }

  const auto progress = static_cast<int>((qint64(value()) - minimum()) * 100.0 / total_steps);

  result.replace(QL1S("%p"), locale.toString(progress));

  // Now, shorten the text to fit the widget.
  bool elide = false;

  while (fontMetrics().boundingRect(result + QSL("...")).width() > width() - 30) {
    result.chop(1);
    elide = true;
  }

  return elide ? result + QSL("...") : result;
}
