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
 * Copyright (c) 2013 SanDisk Corporation.  All rights reserved.
 *
 * File:   interface.h
 * Author: Niranjan Neelakanta
 *
 * Created on March 25, 2013.
 */

/*
 * Format of version 1.0
 *
 * @@CONTACT DETAILS@@
 * Contact: 
 * Company: 
 * Email: 
 * @@PARTICULARS@@
 * Product: ZS
 * Version: 
 * Machine ID:
 * Type: Permanent|Periodic
 * Date valid from:  mm/dd/yy 00:00:00
 * Date valid to:  mm/dd/yy 00:00:00
 * @@CIPHERTEXT@@
 * ... 
 * The license details are stored in a regular file, /opt/sandisk/zs<xx>/license.
 * It contains following details:
 *
 * 1.	Customer contact details: This just provides details of the customer to
 * 	whom license was provided. This has name, company and mail id. This 
 *	doesn't form any key for the generation of license.
 *
 * 2.	Product Name: This gives the details of the product to which this license
 *	is entitled for. This forms one of the key for generation of license.
 *	Having this helps us in using the infrastructure of generating license
 *	to different products. Thus if licenses are requested for different
 *	products for same machine for same time frame, different keys will be 
 *	generated.
 *
 * 3.	Product Version: This will have the version of the product to which this
 *	license is applicable. This too is one of the key for license generation.
 *	This way, if we deliver new version and the license is of previous release
 *	is installed, we can deny the service. 
 *
 * 4.	Machine ID: This is the unique machine ID on which the license was
 *	requested for and ZS is installed. This is one of the keys, and all ZS
 *	APIs would check for this ID to match with that of the machine. This
 *	restricts users from using the license on more than one machine.
 *
 * 5.	Date range: This gives the start and end time (in GMT) of the validity
 *	of the license. The license will not be valid before the start date and
 *	beyond the end date.
 *
 * 6.	License key: This is the unique key. The data will be read from this
 *	license and compared with product, date and the machine. 
 *
 */

#ifndef _ZSVER_1_0
#define _ZSVER_1_0
#include <stdio.h>
#include "license/versions.h"
#include "license/interface.h"

#if 0
extern const char contact_header[];
extern const char particulars_header[];
extern const char key_header[];
extern const char *contact_section[];
#endif
extern const char *particulars_section[];


#ifdef __cplusplus
extern "C" {
#endif

extern int		flf_gen_blank_file_v1_0(FILE *);
extern enum lic_state	flf_gen_lic_file_v1_0(char *, char **);
extern enum lic_state	flf_val_lic_file_v1_0(char *);
extern int		flf_check_lic_comp_v1_0(char *, char *, char *, int);
extern void		flf_get_license_details_v1_0(char *, lic_data_t *);
#ifdef __cplusplus
}
#endif
#endif

