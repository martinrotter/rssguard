// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef PROGRESSBARWITHTEXT_H
#define PROGRESSBARWITHTEXT_H

#include <QProgressBar>

class ProgressBarWithText : public QProgressBar {
  public:
    explicit ProgressBarWithText(QWidget* parent = nullptr);

    // NOTE: Can be uncommented (along with constructor code)
    // to enable automatic fit-to-contents progress bars.
    //virtual QSize minimumSizeHint() const;
    //virtual QSize sizeHint() const;

    virtual QString text() const;
};

#endif // PROGRESSBARWITHTEXT_H
