//----------------------------------------------------------------------------
// ZetaScale
// Copyright (c) 2016, SanDisk Corp. and/or all its affiliates.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published by the Free
// Software Foundation;
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License v2.1 for more details.
//
// A copy of the GNU Lesser General Public License v2.1 is provided with this package and
// can also be found at: http://opensource.org/licenses/LGPL-2.1
// You should have received a copy of the GNU Lesser General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307 USA.
//----------------------------------------------------------------------------

/**
 * File: ht_snapshot.h
 * Description: hotkey snapshot
 *      process snapshot from hotkey buckets.
 * Author: Norman Xu Hickey Liu Mingqiang Zhuang
 */

#ifndef HT_SNAPSHOT_H
#define HT_SNAPSHOT_H

#include "ht_types.h"
#include "ht_report.h"

/**
 * Snapshot structure
 */
typedef struct ReportSnapShot {
    uint16_t        *winner_sorted;
    uint16_t        total;     
    uint16_t        used;
    uint16_t        cur;                  
} ReportSnapShot_t;

/**
 * use to copy the winner list
 */
typedef struct ReportCopyWinner {
    ReportWinnerList_t      *winner_head;
    ReportEntry_t           *winners;
    char                    *key_table;
    struct ReportSnapShot   snapshot;
    struct ReportSortEntry  *ref_sort;
} ReportCopyWinner_t;

/**
 * build snapshot for one instance
 */
char *build_instance_snapshot(ReporterInstance_t *rpt, int ntop,
                             char *rpbuf, int rpsize, 
                             struct tm *last_tm, int trace_ip);

/**
 * Sort winner bucket lists with merge-sorting.
 * Called by extract_hotkeys.
 */
void extract_hotkeys_by_sort(ReporterInstance_t *rpt, 
                             ReportCopyWinner_t *copy_winner, int maxtop);

/**
 * Reset winner lists with current entry which removed during sorting
 * and clean the contents.
 */
void reset_winner_lists(ReporterInstance_t *rpt);

/**
 * dump hot client from client reference count after sorting items
 */
void dump_hot_client(ReportSortEntry_t *clients, uint64_t nbuckets,
                     int ntop, int rpsize, char *rpbuf);

#endif
