// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDRTLBEHAVIOR_H
#define FEEDRTLBEHAVIOR_H

#include <QMetaType>

enum class RtlBehavior {
  NoRtl = 0,                   // The item is not RTL.
  Everywhere = 1,              // RTL is applied everwhere (feed list, article list, article viewer).
  OnlyViewer = 2,              // Use RTL only in article view, but not in article/feed list.
  EverywhereExceptFeedList = 4 // Use RTL everywhere, but not in the feed list.
};

Q_DECLARE_METATYPE(RtlBehavior)

#endif // FEEDRTLBEHAVIOR_H
