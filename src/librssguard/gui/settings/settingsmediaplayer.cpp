// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsmediaplayer.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
#include "gui/mediaplayer/libmpv/libmpvbackend.h"

#include <mpv/client.h>
#endif

SettingsMediaPlayer::SettingsMediaPlayer(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);
}

QIcon SettingsMediaPlayer::icon() const {
  return qApp->icons()->fromTheme(QSL("kmplayer"), QSL("smplayer"));
}

void SettingsMediaPlayer::loadSettings() {
  connect(m_ui.m_gbMpvCustomConfigFolder, &QGroupBox::toggled, this, &SettingsMediaPlayer::dirtifySettings);
  connect(m_ui.m_txtMpvConfigFolder, &QLineEdit::textChanged, this, &SettingsMediaPlayer::dirtifySettings);

  onBeginLoadSettings();

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
  m_ui.m_txtBackend->setText(QSL("libmpv"));
  m_ui.m_helpInfo->setHelpText(tr("You use modern libmpv-based media player backend with API version %1.")
                                 .arg(mpv_client_api_version()),
                               false);
  m_ui.m_stackedDetails->setCurrentWidget(m_ui.m_pageLibmpv);

  m_ui.m_gbMpvCustomConfigFolder
    ->setChecked(settings()->value(GROUP(VideoPlayer), SETTING(VideoPlayer::MpvUseCustomConfigFolder)).toBool());
  m_ui.m_txtMpvConfigFolder
    ->setText(settings()->value(GROUP(VideoPlayer), SETTING(VideoPlayer::MpvCustomConfigFolder)).toString());
#elif defined(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
  m_ui.m_txtBackend->setText(QSL("QtMultimedia"));
  m_ui.m_helpInfo->setHelpText(tr("You use lightweight QtMultimedia-based media player backend. If some videos do not "
                                  "play, then you likely need to install some codecs."),
                               false);
  m_ui.m_stackedDetails->setCurrentWidget(m_ui.m_pageQtMultimedia);
#else
  m_ui.m_txtBackend->setText(tr("no backend installed"));
  m_ui.m_helpInfo->setHelpText(tr("You do not have any media player available. Media player is only supported on "
                                  "modern platforms where needed libraries are available. You must manually recompile "
                                  "%1 to be able to use media player.")
                                 .arg(QSL(APP_NAME)),
                               true);
  m_ui.m_stackedDetails->setCurrentWidget(m_ui.m_pageNothing);
#endif

  onEndLoadSettings();
}

void SettingsMediaPlayer::saveSettings() {
  onBeginSaveSettings();

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
  settings()->setValue(GROUP(VideoPlayer),
                       VideoPlayer::MpvUseCustomConfigFolder,
                       m_ui.m_gbMpvCustomConfigFolder->isChecked());
  settings()->setValue(GROUP(VideoPlayer), VideoPlayer::MpvCustomConfigFolder, m_ui.m_txtMpvConfigFolder->text());

  if (m_ui.m_gbMpvCustomConfigFolder->isChecked()) {
    LibMpvBackend::installCustomConfig(m_ui.m_txtMpvConfigFolder->text());
  }
#elif defined(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)

#else

#endif

  onEndSaveSettings();
}
