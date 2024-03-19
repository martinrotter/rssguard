#ifndef SETTINGSPROPERTIES_H
#define SETTINGSPROPERTIES_H

#include <QString>

// Describes possible types of loaded settings.

// Describes characteristics of settings.
struct SettingsProperties {
    enum class SettingsType {
      Portable,
      NonPortable,
      Custom
    };

    SettingsType m_type;
    QString m_baseDirectory;
    QString m_settingsSuffix;
    QString m_absoluteSettingsFileName;
};

#endif // SETTINGSPROPERTIES_H
