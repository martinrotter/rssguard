// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(QLITEHTML_LIBRARY)
#  define QLITEHTML_EXPORT Q_DECL_EXPORT
#elif defined(QLITEHTML_STATIC_LIBRARY) // Abuse single files for manual tests
#  define QLITEHTML_EXPORT
#else
#  define QLITEHTML_EXPORT Q_DECL_IMPORT
#endif
