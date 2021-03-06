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

/************************************************************************
 * 
 *  btree_raw_range.c  May 9, 2013   Brian O'Krafka
 * 
 *  NOTES: xxxzzz
 *    - persisting secondary index handles
 *    - get by seqno
 *    - return primary key in secondary key access
 * 
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <inttypes.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "btree_hash.h"
#include "btree_raw.h"
#include "btree_map.h"
#include "btree_raw_internal.h"

//  Define this to include detailed debugging code
//#define DEBUG_STUFF


/*********************************************************
 *
 *    RANGE QUERIES
 *
 *********************************************************/


typedef enum ZS_range_enums_t {
    BUFFER_PROVIDED      = 1<<0,  // buffers for keys and data provided by application
    ALLOC_IF_TOO_SMALL   = 1<<1,  // if supplied buffers are too small, ZS will allocate
    SEQNO_LE             = 1<<5,  // only return objects with seqno <= end_seq
    SEQNO_GT_LE          = 1<<6,  // only return objects with start_seq < seqno <= end_seq

    START_GT             = 1<<7,  // keys must be >  key_start
    START_GE             = 1<<8,  // keys must be >= key_start
    START_LT             = 1<<9,  // keys must be <  key_start
    START_LE             = 1<<10, // keys must be <= key_start

    END_GT               = 1<<11, // keys must be >  key_end
    END_GE               = 1<<12, // keys must be >= key_end
    END_LT               = 1<<13, // keys must be <  key_end
    END_LE               = 1<<14, // keys must be <= key_end

    KEYS_ONLY            = 1<<15, // only return keys (data is not required)

    PRIMARY_KEY          = 1<<16, // return primary keys in secondary index query
};

typedef struct ZS_range_meta {
    uint32_t       flags;        // flags controlling type of range query (see above)
    uint32_t       keybuf_size;  // size of application provided key buffers (if applicable)
    uint64_t       databuf_size; // size of application provided data buffers (if applicable)
    char          *key_start;    // start key
    uint32_t       keylen_start; // length of start key
    char          *key_end;      // end key
    uint32_t       keylen_end;   // length of end key
    uint64_t       start_seq;    // starting sequence number (if applicable)
    uint64_t       end_seq;      // ending sequence number (if applicable)
} ZS_range_meta_t;

struct ZS_cursor;       // opaque cursor handle
uint64_t ZS_indexid_t;  // persistent opaque index handle

/* Start an index query.
 * 
 * Returns: ZS_SUCCESS if successful
 *          ZS_FAILURE if unsuccessful
 */
ZS_status_t ZSGetRange(struct ZS_thread_state *thrd_state, //  client thread ZS context
                         ZS_cguid_t              cguid,      //  container
                         ZS_indexid_t            indexid,    //  handle for index to use (use PRIMARY_INDEX for primary)
                         struct ZS_cursor      **cursor,     //  returns opaque cursor for this query
                         ZS_range_meta_t        *meta);      //  query attributes (see above)

typedef struct ZS_range_data {
    ZS_status_t  status;           // status
    char         *key;              // index key value
    uint32_t      keylen;           // index key length
    char         *data;             // data
    uint64_t      datalen;          // data length
    uint64_t      seqno;            // sequence number for last update
    uint64_t      syndrome;         // syndrome for key
    char         *primary_key;      // primary key value (if required)
    uint32_t      primary_keylen;   // primary key length (if required)
} ZS_range_data_t;

/* Gets next n_in keys in the indexed query.
 *
 * The 'values' array must be allocated by the application, and must hold up to
 * 'n_in' entries.
 * If the BUFFER_PROVIDED flag is set, the key and data fields in 'values' must be
 * pre-populated with buffers provided by the application (with sizes that were
 * specified in 'meta' when the index query was started).  If the application provided
 * buffer is too small for returned item 'i', the status for that item will 
 * be ZS_BUFFER_TOO_SMALL; if the ALLOC_IF_TOO_SMALL flag is set, ZS will allocate
 * a new buffer whenever the provided buffer is too small.
 * 
 * If the SEQNO_LE flag is set, only items whose sequence number is less than or
 * equal to 'end_seq' from ZS_range_meta_t above are returned.
 * If there are multiple versions of an item that satisfy the inequality,
 * always return the most recent version.
 *
 * If the SEQNO_GT_LE flag is set, only items for which start_seq < item_seqno <= end_seq
 * are returned.  If there are multiple versions of an item that satisfy the inequality,
 * always return the most recent version.
 * 
 * SEQNO_LE and SEQNO_GT_LE are mutually exclusive and must not be set together.
 * If neither SEQNO_LE or SEQNO_GT_LE are set the most recent version of each key
 * is returned.
 * 
 * 
 * Returns: ZS_SUCCESS    if all statuses are successful
 *          ZS_QUERY_DONE if query is done (n_out will be set to 0)
 *          ZS_FAILURE    if one or more of the key fetches fails (see statuses for the
 *                         status of each fetched object)
 * 
 * statuses[i] returns: ZS_SUCCESS if the i'th data item was retrieved successfully
 *                      ZS_BUFFER_TOO_SMALL  if the i'th buffer was too small to retrieve the object
 */
ZS_status_t
ZSGetNextRange(struct ZS_thread_state *thrd_state,  //  client thread ZS context
                struct ZS_cursor       *cursor,      //  cursor for this indexed search
                int                      n_in,        //  size of 'values' array
                int                     *n_out,       //  number of items returned
                ZS_range_data_t        *values);     //  array of returned key/data values


/* End an index query.
 * 
 * Returns: ZS_SUCCESS if successful
 *          ZS_UNKNOWN_CURSOR if the cursor is invalid
 */
ZS_status_t ZSGetRangeFinish(struct ZS_thread_state *thrd_state, 
                               struct ZS_cursor *cursor);


/*********************************************************
 *
 *    SECONDARY INDEXES
 *
 *********************************************************/

/*
 *   Function used to extract secondary key
 *   from primary key and/or data.
 *   This function must use ZSGetSecondaryKeyBuffer() below
 *   to allocate a buffer for the extracted key.
 *
 *   Returns:  0 if successful
 *             1 if fails (eg: ZSGetSecondaryKeyBuffer() fails)
 */
typedef int (ZS_index_fn_t)(void     *index_data,        //  opaque user data 
                             char     *key,               //  primary key of object
                             uint32_t  keylen,            //  length of primary key
                             char     *data,              //  object data (if required, see flags)
                             uint64_t  datalen,           //  length of object data (if required, see flags)
                             char    **secondary_key,     //  returned secondary key
                             uint32_t *keylen_secondary); //  returned length of secondary key

/*
 *  Allocate a buffer in which to place an extracted
 *  secondary key.
 * 
 *  Returns NULL if a buffer cannot be allocated.
 */
char * ZSGetSecondaryKeyBuffer(uint32_t len);



/*
 *  Function used to compare secondary index values
 *  to determine ordering in the index.
 *
 *  Returns: -1 if key1 comes before key2
 *            0 if key1 is the same as key2
 *            1 if key1 comes after key2
 */
typedef int (ZS_cmp_fn_t)(void     *index_data, //  opaque user data
                           char     *key1,       //  first secondary key
                           uint32_t  keylen1,    //  length of first secondary key
                           char     *key2,       //  second secondary key
                           uint32_t  keylen1);   //  length of second secondary key

typedef enum ZS_range_enums_t {
    INDEX_USES_DATA = 1<<0,  //  Indicates that secondary index key 
                             //  is derived from object data
};

typedef struct ZS_index_meta {
    uint32_t        flags;       //  flags (see ZS_range_enums_t)
    ZS_index_fn   *index_fn;    //  function used to extract secondary key
    ZS_cmp_fn     *cmp_fn;      //  function used to compare index values
    void           *index_data;  //  opaque user data for index/cmp functions
} ZS_index_meta_t;

/*
 * Create a secondary index
 * Index creation is done synchronously: the function does
 * not return until the index is fully created.
 * Secondary index creation in crash-safe: if ZS crashes while
 * index creation is in progress, index creation will be completed
 * when ZS restarts.
 * 
 * Returns: ZS_SUCCESS if successful
 *          ZS_xxxzzz if ZS runs out of memory
 *          ZS_xxxzzz if ZS runs out of storage
 */
ZS_status_t
ZSAddSecondaryIndex(struct ZS_thread_state *thrd_state,   //  client thread ZS context
                     ZS_cguid_t              cguid,        //  container in which to add index
                     ZS_index_id_t          *indexid,      //  persistent opaque handle for new index
                     ZS_index_meta_t        *meta);        //  attributes of new index

/*
 * Delete a secondary index
 * 
 * Index deletion is done synchronously: the function does
 * not return until the index is fully deleted.
 * Secondary index deletion is crash-safe: if ZS crashes while
 * index deletion is in progress, index deletion will be completed
 * when ZS restarts.
 * 
 * Returns: ZS_SUCCESS if successful
 *          ZS_INVALID_INDEX if index is invalid
 */
ZS_status_t
ZSDeleteSecondaryIndex(struct ZS_thread_state *thrd_state, //  client thread ZS context
                        ZS_cguid_t              cguid,      //  container in which to add index
                        ZS_indexid_t            indexid);   //  persistent opaque handle of index to delete

/*
 * Get a list of all current secondary indices.
 * Array returned in index_ids is allocated by ZS and
 * must be freed by application.
 * 
 * Returns: ZS_SUCCESS if successful
 *          ZS_xxxzzz if index_ids cannot be allocated
 */
ZS_status_t
ZSGetContainerIndices(struct ZS_thread_state *ts,         //  client thread ZS context
                         ZS_cguid_t            cguid,      //  container
                         uint32_t              *n_indices,  //  returns number of indices
                         ZS_indexid_t         *index_ids); //  returns array of index ids

/*
 * Get attributes for a particular indexid.
 * 
 * Returns: ZS_SUCCESS if successful
 *          ZS_xxxzzz if indexid is invalid
 */
ZS_status_t
ZSGetIndexMeta(struct ZS_thread_state *ts,       //  client thread ZS context
                ZS_cguid_t              cguid,    //  container
                ZS_indexid_t            indexid,  //  index id
                ZS_index_meta_t        *meta);    //  attributes of index

