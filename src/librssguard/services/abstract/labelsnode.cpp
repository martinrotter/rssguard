// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/labelsnode.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

LabelsNode::LabelsNode(RootItem* parent_item) : RootItem(parent_item), m_actLabelNew(nullptr) {
  setKind(RootItem::Kind::Labels);
  setId(ID_LABELS);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-important")));
  setTitle(tr("Labels"));
  setDescription(tr("You can see all your labels (tags) here."));
  setCreationDate(QDateTime::currentDateTime());
}

QList<QAction*> LabelsNode::contextMenuFeedsList() {
  if (m_actLabelNew == nullptr) {
    // Initialize it all.
    m_actLabelNew = new QAction(qApp->icons()->fromTheme("tag-new"), tr("New label"), this);
  }

  return QList<QAction*> {
    m_actLabelNew
  };
}
