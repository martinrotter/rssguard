// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSLOCALIZATION_H
#define SETTINGSLOCALIZATION_H

#include "gui/settings/settingspanel.h"

#include "ui_settingslocalization.h"

#include <QNetworkReply>

class SettingsLocalization : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsLocalization(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsLocalization();

    virtual void loadUi();
    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private slots:
    void langMetadataDownloaded(const QUrl& url,
                                QNetworkReply::NetworkError status,
                                int http_code,
                                QByteArray contents);

  private:
    Ui::SettingsLocalization* m_ui;
    QUrl m_urlPercentages;
    QUrl m_urlPeople;
    QByteArray m_dataPercentages;
    QByteArray m_dataPeople;
};

inline QString SettingsLocalization::title() const {
  return tr("Localization");
}

#endif // SETTINGSLOCALIZATION_H
