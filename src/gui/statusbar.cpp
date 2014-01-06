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
  m_progressBar->setFixedWidth(100);

  m_progressLabel = new QLabel(this);
  m_progressLabel->setText("aaa");

  // Add widgets.
  addPermanentWidget(m_fullscreenSwitcher);
  addWidget(m_progressBar, 0);
  addWidget(m_progressLabel, 1);
}

StatusBar::~StatusBar() {
  qDebug("Destroying StatusBar instance.");
}

QToolButton *StatusBar::fullscreenSwitcher() const
{
  return m_fullscreenSwitcher;
}
