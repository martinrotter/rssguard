// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "core/message.h"

#include <QList>
#include <QPair>

struct ArticleCounts {
    int m_total = -1;
    int m_unread = -1;
};

struct UpdatedArticles {
    QList<Message> m_unread;
    QList<Message> m_all;
};

struct IconLocation {
    QString m_url;

    // The "bool" if true means that the URL is direct and download directly, if false then
    // only use its domain and download via 3rd-party service.
    bool m_isDirect;

    IconLocation(const QString& url, bool is_direct) : m_url(url), m_isDirect(is_direct) {}
};

#endif // TYPEDEFS_H
