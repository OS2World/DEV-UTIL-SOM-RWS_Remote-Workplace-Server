/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSCLI.H
// Remote Workplace Server - Client header

/****************************************************************************/

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Remote Workplace Server - Client component.
 *
 * The Initial Developer of the Original Code is Richard L. Walsh.
 * 
 * Portions created by the Initial Developer are Copyright (C) 2004-2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

/****************************************************************************/

#define INCL_DOS
#define INCL_PM
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "..\RWS.H"
#include "..\RWSERR.H"

/****************************************************************************/

extern BOOL     fFalse;

#define RWSTIMEOUT_STR      "RWSTIMEOUT"
#define RWSTIMEOUT_DFLT     20

#define RWSMAXTHREADS       32

// thread-specific info used by to dispatch & receive msgs properly
typedef struct _TIDINFO
{
    HWND        hwnd;
    PRWSHDR     pHdr;
    USHORT      msgId;
    USHORT      flags;
    ULONG       ulTime;
    HWND        hNotify;
    ULONG       idNotify;
} TIDINFO;

typedef TIDINFO *PTIDINFO;

// used in TIDINFO.flags
#define RWSF_DISPATCH   1   // RwsDispatch dispatched this msg
#define RWSF_RESEND     2   // RwsDispatch should generate a WM_SEM1 msg

// used in RWSHDR.Flags
#define RWSF_INUSE      1   // pHdr is in use & can't be deleted
#define RWSF_RCVD       2   // pHdr was received outside RwsDispatch's msg loop
#define RWSF_TIMEOUT    4   // pHdr timed-out outside RwsDispatch's msg loop


/****************************************************************************/

// in RWSCUTIL.C
ULONG   _System RwsGetMem( PRWSHDR* ppHdr, ULONG size);

/****************************************************************************/

