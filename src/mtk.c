/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 Bardia Moshiri <bardia@furilabs.com>
 */

#include "mtk.h"
#include "utils.h"

#define ISO_NODE "/proc/perfmgr/boost_ctrl/eas_ctrl/set_sched_isolation"
#define DEISO_NODE "/proc/perfmgr/boost_ctrl/eas_ctrl/set_sched_deisolation"

void
mtk_init(MtkContext *mtk)
{
    if (mtk == NULL)
        return;

    mtk->isolation_available = FALSE;

    /* isolation is available only if both nodes exist */
    if (exists(ISO_NODE) && exists(DEISO_NODE))
        mtk->isolation_available = TRUE;

    g_debug("mtk: isolation_available=%d", mtk->isolation_available ? 1 : 0);
}

void
mtk_isolate(const MtkContext *mtk, int first_core, int last_core)
{
    int i;

    if (mtk == NULL)
        return;

    if (!mtk->isolation_available) {
        g_debug("mtk: isolate requested but isolation not available");
        return;
    }

    g_debug("mtk: isolating cores %d..%d", first_core, last_core);

    for (i = first_core; i <= last_core; i++)
        write_int(ISO_NODE, i);
}

void
mtk_deisolate(const MtkContext *mtk, int first_core, int last_core)
{
    int i;

    if (mtk == NULL)
        return;

    if (!mtk->isolation_available) {
        g_debug("mtk: deisolate requested but isolation not available");
        return;
    }

    g_debug("mtk: deisolating cores %d..%d", first_core, last_core);

    for (i = first_core; i <= last_core; i++)
        write_int(DEISO_NODE, i);
}
