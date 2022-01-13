// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NODEJS_H
#define NODEJS_H

#include <QObject>

class NodeJs : public QObject {
  Q_OBJECT

  public:
    explicit NodeJs(QObject* parent = nullptr);
};

#endif // NODEJS_H
