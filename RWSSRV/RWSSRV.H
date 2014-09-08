/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSSRV.H
// Remote Workplace Server - Server header

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
 * The Original Code is Remote Workplace Server - Server component.
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
#include <string.h>
#include <setjmp.h>
#include <wpabs.h>
#include <wpclsmgr.h>
#include <wpdesk.h>
#include <wpfsys.h>
#include <wpshadow.h>
#include <wptrans.h>
#include "..\RWS.H"
#include "..\RWSERR.H"

/****************************************************************************/

// Macros & typedefs

// a generic typedef for a function that takes one or more arguments
typedef ULONG _System _RWSPROC( ULONG, ...);
typedef _RWSPROC *PFNRWSPROC;

// a generic typedef for a function that takes no arguments
typedef ULONG _System _RWSPROCV( void);
typedef _RWSPROCV *PFNRWSPROCV;

// flags that are currently used only by RwsCommands
#define RWSF_NOREPLY        0x40000000UL
#define RWSF_NOFREE         0x80000000UL

// used to generate an atom that's used as a msg to the UI host window
#define RWSUINAME           "RWSUI" RWSVERSIONNBR

// the default view for drive objects
#define OPEN_AUTO           120

// my favorite macro
#define MP      MPARAM

/****************************************************************************/

// the following eliminate various VAC++ 3.65 informational and warning
// messages without having to turn off diagnostics that might prove useful

// Every .h file for every SOM/WPS class I could find contains this macro
// which redefines the original found in somcdev.h:
//    #define somresolve_(obj,mToken) ((somMethodProc*)((void)obj, mToken))
// Because "obj" is never used, VAC generates an informational message
// for every method call.  The following eliminates these pointless msgs:

#undef somresolve_
#define somresolve_(obj,mToken) ((somMethodProc*)(mToken))

// these nonexistent functions are referenced by "#pragma linkage"
// statements in somltypes.h;  declaring them here keeps VAC happy
void somGenericApply(void);
void somResolveDLocator(void);
void somResolveMLocator(void);

/****************************************************************************/

// alternate implementations of these macros
#ifdef LOUSHORT
    #undef LOUSHORT
#endif
#define LOUSHORT(l)     (((PUSHORT)(PVOID)&(l))[0])

#ifdef HIUSHORT
    #undef HIUSHORT
#endif
#define HIUSHORT(l)     (((PUSHORT)(PVOID)&(l))[1])

/****************************************************************************/

// stuff for RWSCMD_LOCATE

typedef struct _LOCATEOBJ
{
    WPObject *      pObj;
    HWND            hCnr;
    ULONG           ctr;
} LOCATEOBJ;

typedef LOCATEOBJ *PLOCATEOBJ;

#define LOCATEOBJ_CNT       16
#define LOCATEOBJ_TRIES     20
#define LOCATEOBJ_TIMER     (TID_USERMAX - 27)

/****************************************************************************/

// Exception handling

typedef struct _RWSXCPTREGREC
{
    EXCEPTIONREGISTRATIONRECORD regrec;
    jmp_buf                     jmpbuf;
} RWSXCPTREGREC;

typedef RWSXCPTREGREC *PRWSXCPTREGREC;

/****************************************************************************/

// shared global data (defined in RWSSUTIL.C)

extern ATOM             aRwsUI;
extern ATOM             aRwsSrv;
extern BOOL             fFalse;
extern BOOL             fUseHandler;
extern HMODULE          hmodSOM;
extern HWND             hThread1;
extern PID              pidWPS;

extern SOMClass *       clsSOMClass;
extern SOMClass *       clsWPAbstract;
extern SOMClass *       clsWPDesktop;
extern SOMClass *       clsWPFileSystem;
extern SOMClass *       clsWPShadow;
extern SOMClass *       clsWPTransient;

/****************************************************************************/

// undocumented PM API function

HMQ     _System Win32QueueFromID( HAB hab, ULONG pid, ULONG tid);

// in RWSSRV.C
ULONG           Resolve( PRWSHDR pHdr, HWND hReply);
ULONG           ResolveArgIn( PRWSDSC pArg,  PRWSDSC pArg1);
ULONG           ResolveArgOut( PRWSDSC pArg, PRWSDSC pRtn, HWND hReply);
ULONG           ResolveRtn( PRWSDSC pArg,  PRWSDSC pRtn, HWND hReply);
SOMObject *     ObjFromShadow( SOMObject * pShadow);
SOMClass *      ClassFromName( PSZ pszClass);

// in RWSSUTIL.C
void            Thread1AddSem( HWND hwnd);
void            Thread1CloseSem( HWND hwnd);
ULONG   _System RwsExceptionHandler( PEXCEPTIONREPORTRECORD pReport,
                                     PEXCEPTIONREGISTRATIONRECORD pRegRec,
                                     PCONTEXTRECORD pContext, PVOID pVoid);

// in RWSSCMD.C
ULONG           ResolveCommand( PRWSHDR pHdr, HWND hReply);
void            Thread1ResolveCommand( HWND hwnd, PRWSHDR pHdr, HWND hReply);

/****************************************************************************/

