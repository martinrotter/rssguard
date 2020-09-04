// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/labelsnode.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

LabelsNode::LabelsNode(RootItem* parent_item) : RootItem(parent_item) {
  setKind(RootItem::Kind::Labels);
  setId(ID_LABELS);
  setIcon(qApp->icons()->fromTheme(QSL("mail-mark-important")));
  setTitle(tr("Labels"));
  setDescription(tr("You can see all your labels (tags) here."));
  setCreationDate(QDateTime::currentDateTime());
}
