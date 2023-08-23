// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/singlenotificationeditor.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCompleter>
#include <QFileDialog>
#include <QFileSystemModel>

SingleNotificationEditor::SingleNotificationEditor(const Notification& notification, QWidget* parent)
  : QGroupBox(parent), m_notificationEvent(Notification::Event::NoEvent) {
  m_ui.setupUi(this);

#if defined(Q_OS_OS2)
  m_ui.m_wdgSound->setVisible(false);
#else
  m_ui.m_btnBrowseSound->setIcon(qApp->icons()->fromTheme(QSL("document-open")));
  m_ui.m_btnPlaySound->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start")));
#endif

  loadNotification(notification);

  connect(m_ui.m_btnPlaySound, &QPushButton::clicked, this, &SingleNotificationEditor::playSound);
  connect(m_ui.m_btnBrowseSound, &QPushButton::clicked, this, &SingleNotificationEditor::selectSoundFile);
  connect(m_ui.m_txtSound, &QLineEdit::textChanged, this, &SingleNotificationEditor::notificationChanged);
  connect(m_ui.m_cbBalloon, &QCheckBox::toggled, this, &SingleNotificationEditor::notificationChanged);
  connect(m_ui.m_slidVolume, &QSlider::valueChanged, this, &SingleNotificationEditor::notificationChanged);

  QCompleter* completer = new QCompleter(qApp->builtinSounds(), this);
  m_ui.m_txtSound->setCompleter(completer);

  setFixedHeight(sizeHint().height());
}

Notification SingleNotificationEditor::notification() const {
  return Notification(m_notificationEvent,
                      m_ui.m_cbBalloon->isChecked(),
                      m_ui.m_txtSound->text(),
                      m_ui.m_slidVolume->value());
}

void SingleNotificationEditor::selectSoundFile() {
  auto fil = QFileDialog::getOpenFileName(window(),
                                          tr("Select sound file"),
                                          qApp->homeFolder(),
                                          tr("WAV files (*.wav);;MP3 files (*.mp3)"));

  if (!fil.isEmpty()) {
    m_ui.m_txtSound->setText(fil);
  }
}

void SingleNotificationEditor::playSound() {
  notification().playSound(qApp);
}

void SingleNotificationEditor::loadNotification(const Notification& notification) {
  m_ui.m_txtSound->setText(notification.soundPath());
  m_ui.m_slidVolume->setValue(notification.volume());
  m_ui.m_cbBalloon->setChecked(notification.balloonEnabled());
  m_notificationEvent = notification.event();

  setTitle(Notification::nameForEvent(notification.event()));
}
