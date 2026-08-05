// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APPLICATIONPATHS_H
#define APPLICATIONPATHS_H

#include <QString>
#include <QStringList>

class Settings;

class ApplicationPaths {
  public:
    explicit ApplicationPaths(Settings* settings);

    QString tempFolder() const;
    QString documentsFolder() const;
    QString homeFolder() const;
    QString applicationDirPath() const;
    QString configFolder() const;
    QString userDataAppFolder() const;
    QString userDataHomeFolder() const;
    QString customDataFolder() const;
    QString userDataFolder() const;
    QString replaceUserDataFolderPlaceholder(QString text, bool double_escape) const;
    QStringList replaceUserDataFolderPlaceholder(QStringList texts) const;
    bool setCustomDataFolder(const QString& data_folder);

    Settings* settings() const;
    void setSettings(Settings* newSettings);

  private:
    Settings* m_settings;
    QString m_customDataFolder;
};

#endif // APPLICATIONPATHS_H
