// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef PROGRESSBARWITHTEXT_H
#define PROGRESSBARWITHTEXT_H

#include <QProgressBar>

class ProgressBarWithText : public QProgressBar {
  public:
    explicit ProgressBarWithText(QWidget* parent = nullptr);

    virtual QString text() const;
};

#endif // PROGRESSBARWITHTEXT_H
