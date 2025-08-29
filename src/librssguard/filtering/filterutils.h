// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERUTILS_H
#define FILTERUTILS_H

#include <QDateTime>
#include <QDomElement>
#include <QObject>

class FilterUtils : public QObject {
    Q_OBJECT

  public:
    explicit FilterUtils(QObject* parent = nullptr);
    ~FilterUtils();

    // Returns hostname or empty string if failed.
    Q_INVOKABLE QString hostname() const;

    // Converts XML -> JSON or returns empty string if failed.
    Q_INVOKABLE QString fromXmlToJson(const QString& xml) const;

    // Parses string into date/time object.
    Q_INVOKABLE QDateTime parseDateTime(const QString& dat) const;
    Q_INVOKABLE QString runExecutableGetOutput(const QString& executable, const QStringList& arguments = {}) const;
    Q_INVOKABLE void runExecutable(const QString& executable,
                                   const QStringList& arguments = {},
                                   const QString& working_directory = {}) const;
};

#endif // FILTERUTILS_H
