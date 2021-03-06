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

/*
 * File:   fthSched.h
 * Author: Jim
 *
 * Created on February 29, 2008
 *
 * (c) Copyright 2008, Schooner Information Technology, Inc.
 * http: //www.schoonerinfotech.com/
 *
 * $Id: fthSched.h 396 2008-02-29 22:55:43Z jim $
 */

// Resolve include order issue. fthThreadQ.h needs to have the fthThread 
// defined so that its inline definitions can be resolved.
#include "fthThreadQ.h"

#ifndef _FTH_SCHED_H
#define _FTH_SCHED_H

#include "platform/types.h"

#include "fthSpinLock.h"
#include "fthSchedType.h"

#include "fthDispatch.h"

#define FTH_SCHED_SWITCH_COUNT

//
// Per-thread structure
typedef struct fthSched {

    fthDispatchArea_t dispatch;              // ***ORDER***

#ifdef FTH_TIME_STATS
    uint64_t totalThreadRunTime;
    uint64_t schedulerIdleTime;
    uint64_t schedulerDispatchTime;
    uint64_t schedulerLowPrioDispatchTime;
    uint64_t schedulerNumDispatches;
    uint64_t schedulerNumLowPrioDispatches;
#endif    

    // tsc value to detect TSC going backwards
    uint64_t prevTsc;
    // Timer drift adjustment fields for scheduler
    uint64_t prevTimeTsc;                    // tsc ticks of previous time check
    struct timespec prevTimeSpec;            // Time spec of previous time check

    unsigned backwardsGettimeWarned : 1;     // Warned of backwards gettime
    unsigned backwardsTscWarned : 1;         // Warned of backwards tsc
#ifdef ENABLE_FTH_TRACE
    struct fthTraceBuffer *traceBuffer;
#endif
    uint32_t schedMask;                      // Mask for this scheduler
    int schedNum;                            // Ordinal of scheduler thread
    int preferedSched;                       // Prefered scheduler
    int prevDispatchPrio;                    // Low/high prio of previous dispatch
    int globalDispatchSkip;

#define SCHED_MAX_FREE_WAIT 200
#define SCHED_MAX_FREE_SPARSE 50
    int freeWaitCount;                       // Number on the free list
    int freeSparseCount;                     // Number on the free list
    
    fthWaitQ_lll_t freeWait;                 // Free wait list elements
    fthSparseQ_lll_t freeSparse;             // Free sparse locks

    #ifdef FTH_INSTR_LOCK
        fthLockTraceData_t   *locktrace_data;
    #endif // FTH_INSTR_LOCK

    #ifdef AIO_TRACE
        aio_trace_sched_state_t *aiotrace_data;
    #endif // AIO_TRACE

} fthSched_t;

#endif
