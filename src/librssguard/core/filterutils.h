// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTERUTILS_H
#define FILTERUTILS_H

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
};

#endif // FILTERUTILS_H
