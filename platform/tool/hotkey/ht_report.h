/*
 * Copyright (c) 2009-2016, SanDisk Corporation. All rights reserved.
 * No use, or distribution, of this source code is permitted in any form or
 * means without a valid, written license agreement with SanDisk Corp.  Please
 * refer to the included End User License Agreement (EULA), "License" or "License.txt" file
 * for terms and conditions regarding the use and redistribution of this software.
 */

/**
 * File: ht_report.h
 * Description: hotkey report
 *      report the top N hot keys
 * Author: Norman Xu Hickey Liu Mingqiang Zhuang
 */
#ifndef HT_REPORT_H
#define HT_REPORT_H

#include "inttypes.h"

#define HOTKEY_REPORT_ENTRY_SIZE        300

/**
 * Report instance for different type of reporters
 */
typedef struct ReporterInstance {
    /* candiate */
    struct ReportEntry              *candidates;
    int                             ncandidates;
    int                             ncandidates_per_list;

    /* winner */
    struct ReportWinnerList         *winner_head;
    int                             nwinner_head;
    int                             nwinners_per_list;
    char                            *key_table;
    struct ReportEntry              *winners;
    int                             nwinners;

    int                             cmd;
    int                             nbuckets_per_winner_list;
    int                             nkeys_per_winner_list;
    int                             nsort_winners;
} ReporterInstance_t;

/**
 * hotkey reporter flag for reporter instance
 */
enum hotkey_reporter_flags {
    HOTKEY_CMD_SEPARATE         = 0x01,
    HOTKEY_CLI_SEPARATE         = 0x02,
    HOTKEY_CLI_FLAGS_MASK       = 0x07
};


/**
 * Reporter controls candidates, winners and snapshots
 */
typedef struct Reporter {
    struct ReporterInstance         *instances;
    int                             cur_instance;
    int                             maxtop;
    void                            *mm_ptr;
    void                            *mm_pstart;
    int                             mm_size;
    int                             mm_used;
    struct ReportClientEntry        *client_ref;
    struct tm                       *last_tm;
    int                             client_mode;
    int                             hotkey_mode;
    int                             dump_num;
} Reporter_t;

/**
 * Calculate the exactly memory exhausted in hotkey
 * The calcualation method must follow the init methods
 * for each structure that in Reporter_t.
 */
int calc_hotkey_memory(int maxtop, int nbuckets, int reporter_flag);

/**
 * Init hot key report system. nbuckets is the bucket total number.
 * maxtop is the maxinum top value the hot key reporter should support.
 * cmd refer to cmd_type_t.
 */
Reporter_t *hot_key_init(void *mm_ptr, int mm_size, int nbuckets,
                         int maxtop, int reporter_flag);


/**
 * Update hot key according to keys, commands and clients;
 * It is supposed to be called after client commands have been parsed.
 * The command parameter could be CMD_GET, CMD_UPD.
 */
int hot_key_update(Reporter_t *preporter, char *key, int key_len,
                   uint64_t hash, uint32_t bucket, int command,
                   uint32_t client_ip);

/**
 * Caller want to know the ntop hot keys. It should provide string buffer.
 */
int hot_key_report(Reporter_t *preporter, int ntop, char *rpbuf,
                   int rpsize, int command, uint32_t client_ip);
/**
 * Caller want to know the ntop hot clients. 
 * It should provide string buffer.
 */
int hot_client_report(Reporter_t *preporter, int ntop,
                      char *rpbuf, int rpsize);

/**
 * Reset reporter hotkeys including hotkey in winner lists and clients
 */
int hot_key_reset(Reporter_t *preporter);

/**
 * Clean up reporter instance
 */
int hot_key_cleanup(Reporter_t *preporter);

/**
 * Report hotkey support version
 */
int hot_key_report_version( char ** bufp, int * lenp );

#endif
