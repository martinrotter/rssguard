// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ADBLOCKICON_H
#define ADBLOCKICON_H

#include <QAction>

class QMenu;
class AdBlockManager;

class AdBlockIcon : public QAction {
  Q_OBJECT

  public:
    explicit AdBlockIcon(AdBlockManager* parent = nullptr);
    virtual ~AdBlockIcon();

  public slots:
    void setIcon(bool adblock_enabled);

  private slots:
    void showMenu(const QPoint& pos);

  private:
    void createMenu(QMenu* menu = nullptr);

  private:
    AdBlockManager* m_manager;
};

#endif // ADBLOCKICON_H
