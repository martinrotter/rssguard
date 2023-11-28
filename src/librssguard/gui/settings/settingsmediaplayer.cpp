// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/settings/settingsmediaplayer.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
#include <mpv/client.h>
#endif

SettingsMediaPlayer::SettingsMediaPlayer(Settings* settings, QWidget* parent) : SettingsPanel(settings, parent) {
  m_ui.setupUi(this);
}

void SettingsMediaPlayer::loadSettings() {
  onBeginLoadSettings();

#if defined(ENABLE_MEDIAPLAYER_LIBMPV)
  m_ui.m_txtBackend->setText(QSL("QtMultimedia"));
  m_ui.m_helpInfo->setHelpText(tr("You use modern libmpv-based media player backend with API version %1.")
                                 .arg(mpv_client_api_version()),
                               false);
#elif defined(ENABLE_MEDIAPLAYER_QTMULTIMEDIA)
  m_ui.m_txtBackend->setText(QSL("libmpv"));
  m_ui.m_helpInfo->setHelpText(tr("You use lightweight QtMultimedia-based media player backend. If some videos do not "
                                  "play, then you likely need to install some codec pack."),
                               false);
#else
  m_ui.m_txtBackend->setText(tr("no backend installed"));
  m_ui.m_helpInfo->setHelpText(tr("You do not have any media player available. Media player is only supported on "
                                  "modern platforms where needed libraries are available. You must manually recompile "
                                  "%1 to be able to use media player.")
                                 .arg(QSL(APP_NAME)),
                               true);
#endif

  onEndLoadSettings();
}

void SettingsMediaPlayer::saveSettings() {
  onBeginSaveSettings();

  onEndSaveSettings();
}
