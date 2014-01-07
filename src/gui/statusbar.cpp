#include "gui/statusbar.h"

#include "gui/iconthemefactory.h"

#include <QToolButton>
#include <QLabel>
#include <QProgressBar>


StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent) {
  setSizeGripEnabled(false);
  setContentsMargins(3, 1, 3, 1);

  // Initializations of widgets for status bar.
  m_fullscreenSwitcher = new QToolButton(this);
  m_fullscreenSwitcher->setIcon(IconThemeFactory::getInstance()->fromTheme("view-fullscreen"));
  m_fullscreenSwitcher->setToolTip(tr("Switch application between fulscreen/normal states right from this status bar icon."));

  m_progressBar = new QProgressBar(this);
  m_progressBar->setTextVisible(false);
  m_progressBar->setFixedWidth(100);
  m_progressBar->setVisible(false);

  m_progressLabel = new QLabel(this);
  m_progressLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  m_progressLabel->setText("aaa");
  m_progressLabel->setVisible(false);

  // Add widgets.
  addPermanentWidget(m_progressLabel);
  addPermanentWidget(m_progressBar);
  addPermanentWidget(m_fullscreenSwitcher);
}

StatusBar::~StatusBar() {
  qDebug("Destroying StatusBar instance.");
}

QToolButton *StatusBar::fullscreenSwitcher() const {
  return m_fullscreenSwitcher;
}

void StatusBar::showProgress(int progress, const QString &label) {
  m_progressLabel->setVisible(true);
  m_progressBar->setVisible(true);

  m_progressLabel->setText(label);
  m_progressBar->setValue(progress);
}

void StatusBar::clearProgress() {
  m_progressLabel->setVisible(false);
  m_progressBar->setVisible(false);
}
