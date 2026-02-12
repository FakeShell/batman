/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#ifndef MTK_H
#define MTK_H

#include <glib.h>

/**
 * MTK isolation context.
 *
 * Isolation is available when these nodes exist:
 *   /proc/perfmgr/boost_ctrl/eas_ctrl/set_sched_deisolation
 *   /proc/perfmgr/boost_ctrl/eas_ctrl/set_sched_isolation
 */
typedef struct {
    gboolean isolation_available;  /** True if isolation proc nodes exist */
} MtkContext;

/**
 * Detect MTK isolation availability.
 *
 * @param mtk context to fill
 */
void
mtk_init(MtkContext *mtk);

/**
 * Apply MTK CPU isolation for a range of cores.
 *
 * Writes each core id to:
 *   /proc/perfmgr/boost_ctrl/eas_ctrl/set_sched_isolation
 *
 * @param mtk mtk context
 * @param first_core first core id
 * @param last_core last core id
 */
void
mtk_isolate(const MtkContext *mtk, int first_core, int last_core);

/**
 * Remove MTK CPU isolation for a range of cores.
 *
 * Writes each core id to:
 * /proc/perfmgr/boost_ctrl/eas_ctrl/set_sched_deisolation
 *
 * @param mtk mtk context
 * @param first_core first core id
 * @param last_core last core id
 */
void
mtk_deisolate(const MtkContext *mtk, int first_core, int last_core);

#endif /* MTK_H */
