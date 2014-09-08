/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSCUTIL.C
// Remote Workplace Server - Client utility functions

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

#include "RWSCLI.H"

/****************************************************************************/

char            szCopyright[] = "RWS v" RWSVERSION " - (C)Copyright 2004-2007  R.L.Walsh";

BOOL            fFalse  = FALSE;
BOOL            fRegRws = FALSE;
BOOL            fExitList = FALSE;
ATOM            aRwsSrv = 0;
ATOM            aRwsCancel = 0;
HMODULE         hmodCli = 0;
HOBJECT         hobjRws = 0;
HWND            hRwsSrv = 0;
PVOID           pRwsMem = 0;
ULONG           ulTimeout = RWSTIMEOUT_DFLT*1000;
ULONG           ulUserTimeout = 0;
TIDINFO         aTidInfo[RWSMAXTHREADS+1];
char            szObjTitle[32] = "";
char            szRwsSrv[] = RWSSRVNAME;
char            szRwsCls[] = RWSCLASSNAME;
char            szRwsCancel[] = RWSCLINAME "_CANCEL";

/****************************************************************************/

ULONG   _System RwsQueryVersion( PULONG pulReserved);
ULONG   _System RwsClientInit( BOOL fRegister);
PTIDINFO        ValidateClientWindow( void);
PTIDINFO        GetThreadInfo( void);
void            InitTimeout( void);
void            InitObjectTitle( void);
BOOL            RegisterRWSClass( void);
BOOL            ValidateServerWindow( void);
HWND            FindServerWindow( void);
ULONG   _System RwsDispatch( PRWSHDR pHdr);
MRESULT _System ClientWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void            PostNotification( PTIDINFO pTI, ULONG idEvent, ULONG ulParm);
ULONG   _System RwsDispatchStatus( PBOOL pfReady);
ULONG   _System RwsCancelDispatch( HWND hNotify, ULONG ulCmd);
ULONG   _System RwsNotify( HWND hNotify, ULONG idNotify);
ULONG   _System RwsGetServerMsgID( PULONG pulMsgID);
ULONG   _System RwsDispatchAsync( HWND hwnd, PRWSHDR pHdr);
ULONG   _System RwsGetMem( PRWSHDR* ppHdr, ULONG size);
ULONG   _System RwsFreeMem( PRWSHDR ptr);
ULONG   _System RwsGetResult( PRWSHDR pHdr, ULONG ndxArg, PULONG pSize);
ULONG           GetResultCnt( PVOID pBuf, PRWSDSC pRtn, ULONG ndx);
ULONG   _System RwsGetArgPtr( PRWSHDR pHdr, ULONG ndxArg, PRWSDSC* ppArg);
ULONG   _System RwsGetRcString( ULONG rc, ULONG cbBuf, char * pszBuf);
ULONG   _System RwsGetTimeout( PULONG pulSecs, PULONG pulUser);
ULONG   _System RwsSetTimeout( ULONG ulSecs);
void    _System RwsExitListProc( ULONG ulTermCode);
void            ErrMsg( char * msg, ULONG ul);

/****************************************************************************/

// not defined in some versions of the OS/2 Toolkit (ordinal DOSCALL1.360)
// DosQueryModFromEIP( &ulModule, &ulObject, ulBufLen, szBuffer, &ulOffset, ulEIP);

/****************************************************************************/
/* Init functions                                                           */
/****************************************************************************/

// RWSFULLVERSION for v0.80 is 0x08000100

ULONG   _System RwsQueryVersion( PULONG pulReserved)

{
    if (pulReserved)
        *pulReserved = 0;

    return (RWSFULLVERSION);
}

/****************************************************************************/

// called by an application to init the RwsClient and
// cause RwsServer to be loaded if it isn't already

ULONG   _System RwsClientInit( BOOL fRegister)

{
    ULONG       rc = 0;
    ULONG       ulObj;
    ULONG       ulOffs;
    char    	szBuf[16];

do
{
    // get RwsCliXX's module handle;  DosQueryModuleHandle
    // will fail if we were loaded by f/q name, this won't
    DosQueryModFromEIP( &hmodCli, &ulObj, sizeof(szBuf), szBuf,
                        &ulOffs, (ULONG)RwsClientInit);

    // get/create the atom we use as a msg & window ID
    if (aRwsSrv == 0)
    {
        aRwsSrv = WinAddAtom( WinQuerySystemAtomTable(), szRwsSrv);
        if (aRwsSrv == 0)
            ERRNBR( RWSCLI_NOATOM)
    }

    // get or cause the creation of the server's window
    if (hRwsSrv == 0)
    {
        fRegRws = fRegister;
        if (ValidateServerWindow() == FALSE)
            ERRNBR( RWSCLI_SERVERNOTFOUND)
    }

    // create & subclass the window this thread uses to receive
    // replies from the server;  if other threads call RwsDispatch(),
    // their windows will be created on the fly
    if (ValidateClientWindow() == 0)
        ERRNBR( RWSCLI_NOCLIENTWND)

    // alloc the gettable shared mem used to pass our data to the server
    // then setup the DosSub*() API to manage it
    if (pRwsMem == 0)
    {
        if (DosAllocSharedMem( &pRwsMem, 0, RWSMEMSIZE,
                               PAG_COMMIT | OBJ_GETTABLE |
                               PAG_READ | PAG_WRITE))
            ERRNBR( RWSCLI_DOSALLOCMEM)

        if (DosSubSetMem( pRwsMem, DOSSUB_INIT | DOSSUB_SERIALIZE, RWSMEMSIZE))
        {
            DosFreeMem( pRwsMem);
            pRwsMem = 0;
            ERRNBR( RWSCLI_DOSSUBSETMEM)
        }
    }

    InitTimeout();

} while (fFalse);

    if (rc)
        ErrMsg( "RwsClientInit failed - rc= %d", rc);

    return (rc);
}

/****************************************************************************/

// get the window this thread uses to receive replies posted by
// the server;  if it doesn't exist yet, create one and subclass it;

PTIDINFO        ValidateClientWindow( void)

{
    PTIDINFO    pRtn = 0;

do
{
    pRtn = GetThreadInfo();
    if (!pRtn)
        break;

    if (pRtn->hwnd)
    {
        if (WinIsWindow( 0, pRtn->hwnd))
            break;
        pRtn->hwnd = 0;
    }

    // create a static-class window 'cause it's easy
    pRtn->hwnd = WinCreateWindow( HWND_OBJECT, WC_STATIC, "",
                            SS_TEXT, 0, 0, 0, 0, 0, HWND_BOTTOM,
                            READABLEULONG( 0, 0, 'W', 'C'), 0, 0);
    if (pRtn->hwnd == 0)
    {
        pRtn = 0;
        break;
    }

    // subclass it - the subclass proc will completely
    // replace the original wndproc
    if (WinSubclassWindow( pRtn->hwnd, &ClientWndProc) == 0)
    {
        WinDestroyWindow( pRtn->hwnd);
        pRtn->hwnd = 0;
        pRtn = 0;
        break;
    }

} while (fFalse);

    return (pRtn);
}

/****************************************************************************/

PTIDINFO        GetThreadInfo( void)

{
    HAB         hab;
    ULONG       ndx;

    hab = WinQueryAnchorBlock( HWND_DESKTOP);
    ndx = hab & 0xffff;
    if (ndx && ndx <= RWSMAXTHREADS)
        return (&aTidInfo[ndx]);

    return (0);
}

/****************************************************************************/

// if there's a user-set timeout, use that - otherwise, use the default;
// the user-set timeout is saved for use by RwsGetTimeout()

void        InitTimeout( void)

{
    ULONG       lSecs = 0;
    char *      ptr;

    if (DosScanEnv( RWSTIMEOUT_STR, &ptr) == 0)
        lSecs = atol( ptr);

    if (lSecs > 0 && lSecs <= 300)
    {
        ulTimeout = (ULONG)(lSecs * 1000);
        ulUserTimeout = ulTimeout;
    }
    else
        ulTimeout = (ULONG)(RWSTIMEOUT_DFLT * 1000);

    return;
}

/****************************************************************************/

// construct a title for our RWS object based on the exe's name & PID

void        InitObjectTitle( void)

{
    PTIB    ptib;
    PPIB    ppib;
    ULONG   ctr;
    char *  pSrc;
    char *  pDst;

    DosGetInfoBlocks( &ptib, &ppib);

    pSrc = strrchr( ppib->pib_pchcmd, '\\');
    if (pSrc)
        pSrc++;
    else
        pSrc = ppib->pib_pchcmd;

    for (pDst=szObjTitle, ctr=0; ctr < 24; ctr++)
    {
        if (*pSrc == '\0' || *pSrc == '.')
            break;
        *pDst++ = *pSrc++;
    }

    strcpy( pDst, "-0x");
    pDst = strchr( pDst, '\0');
    _ultoa( ppib->pib_ulpid, pDst, 16);

    return;
}

/****************************************************************************/

// this function locates and validates the server window;  it is called
// during init and again each time the client prepares to post a request
// to the server;  if the WPS has crashed, it will reinit the server

BOOL        ValidateServerWindow( void)

{
    BOOL        fRtn = FALSE;
    ULONG       ctr;

do
{
    // if we have a window & it validates, exit
    if (hRwsSrv && WinQueryWindowULong( hRwsSrv, QWL_USER) == aRwsSrv)
    {
        fRtn = TRUE;
        break;
    }
    hRwsSrv = 0;

    // if the title for our object hasn't been constructed, do so now
    if (*szObjTitle == 0)
        InitObjectTitle();

    // create an object to keep the RWS class & its dll in memory
    hobjRws = WinCreateObject( szRwsCls, szObjTitle, "", "<WP_NOWHERE>",
                               CO_UPDATEIFEXISTS);

    // if that failed, see if the class needs to be registered,
    // then try again;  exit on a second failure
    if (hobjRws == 0)
    {
        if (fRegRws == FALSE || RegisterRWSClass() == FALSE)
            break;

        hobjRws = WinCreateObject( szRwsCls, szObjTitle, "", "<WP_NOWHERE>",
                                   CO_UPDATEIFEXISTS);
        if (hobjRws == 0)
            break;
    }

    // it may take a while for the server to create its object window if
    // the class was just loaded; wait for up to 2 seconds, then give up
    for (ctr = 22; ctr; ctr--)
    {
        hRwsSrv = FindServerWindow();
        if (hRwsSrv)
           break;
        DosSleep( 90);
    }

    if (hRwsSrv == 0)
    {
        RwsClientTerminate();
        break;
    }

    // set an entry in the Exit List;  it has the highest priority (0)
    // so it will be among the first procs that are run;  if the app
    // calls RwsClientTerminate(), it will be removed from the list
    if (fExitList == FALSE)
        if (DosExitList( EXLST_ADD | 0x0000, RwsExitListProc) == 0)
            fExitList = TRUE;
        else
            ErrMsg( "unable to set exit list proc", 0);

    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

// this is called when WinCreateObject() fails during init;  it
// registers the RWS class without bothering to check whether it's
// already registered because if there were a valid registration
// WinCreateObject() probably wouldn't have failed

BOOL        RegisterRWSClass( void)

{
    BOOL        fRtn = TRUE;
    char *  	ptr;
    char        szPath[260];

do
{
    // first see if RwsSrv is in the same directory as RwsCli;
    // in many instances, this may be the only way to find it since
    // the WPS may have a different PATH or LIBPATH than the client app

    if (DosQueryModuleName( hmodCli, sizeof(szPath), szPath) == 0 &&
        (ptr = strrchr( szPath, '\\')) != 0)
    {
        strcpy( ptr+1, RWSSRVDLL);
        if (WinRegisterObjectClass( szRwsCls, szPath))
            break;
        WinDeregisterObjectClass( szRwsCls);
    }

    // next, see if the server dll is in the current directory (likely)
    // or somewhere along the PATH (unlikely);  if so, we'll register the
    // class using the dll's f/q name;  if it can't be found in those
    // locations, we'll try to register using just the module name in
    // the hope it can be found along the LIBPATH

    if (DosSearchPath( SEARCH_ENVIRONMENT | SEARCH_CUR_DIRECTORY |
                       SEARCH_IGNORENETERRS, "PATH", RWSSRVDLL,
                       szPath, sizeof( szPath)))
        strcpy( szPath, RWSSRVMOD);

    // if WinRegisterObjectClass() returns FALSE, it means the dll couldn't
    // be found;  however the class still gets added to the class list, so
    // we have to explicitly remove it to avoid future registration failures

    fRtn = WinRegisterObjectClass( szRwsCls, szPath);
    if (fRtn == FALSE)
    {
        ErrMsg( "unable to register WPS class %s", (ULONG)szRwsCls);
        WinDeregisterObjectClass( szRwsCls);
    }

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

HWND        FindServerWindow( void)

{
    HWND        hRtn;
    HENUM       hEnum;

do
{
    // look for our window the quick & dirty way
    hRtn = WinWindowFromID( HWND_OBJECT, aRwsSrv);
    if (hRtn == 0)
        break;

    // if the window we found validates, exit
    if (WinQueryWindowULong( hRtn, QWL_USER) == aRwsSrv)
        break;

    // deal with the unlikely possibility that another window
    // has the same ID as the one we're looking for
    hEnum = WinBeginEnumWindows( HWND_OBJECT);
    while ((hRtn = WinGetNextWindow( hEnum)) != 0)
    {
        if (WinQueryWindowULong( hRtn, QWL_USER) == aRwsSrv)
            break;
    }
    WinEndEnumWindows( hEnum);

} while (fFalse);

    return (hRtn);
}

/****************************************************************************/
/*  Message Dispatch functions                                              */
/****************************************************************************/

#define RWSDISPATCHTIMER    (aRwsSrv & (ULONG)TID_USERMAX)

ULONG   _System RwsDispatch( PRWSHDR pHdr)

{
    ULONG       rc = 0;
    MRESULT     mres = 0;
    HWND        hRwsCli;
    PTIDINFO    pTI;
    QMSG        qmsg;

    pTI = GetThreadInfo();
    if (!pTI)
        return (RWSCLI_BADTID);

    // new in v0.80 - prevent recursive calls
    if (pTI->pHdr)
        return (RWSCLI_RECURSIVECALL);

    if (pHdr == 0)
        return (RWSCLI_BADDISPATCHARG);

    // confirm the server window is present & valid
    if (ValidateServerWindow() == FALSE)
        return (RWSCLI_SERVERNOTFOUND);

    // get or create the client window for this thread
    if (!ValidateClientWindow())
        return (RWSCLI_NOCLIENTWND);

    // start the timeout timer
    if (!WinStartTimer( 0, pTI->hwnd, RWSDISPATCHTIMER, ulTimeout))
        return (RWSCLI_NOTIMER);

do {

    hRwsCli = pTI->hwnd;
    pTI->ulTime = WinGetCurrentTime( 0);
    pTI->pHdr = pHdr;
    pTI->msgId++;
    pHdr->MsgId = pTI->msgId;
    pHdr->Flags = RWSF_INUSE;

    // post the msg, identifying the data block & the window to reply to
    if (!WinPostMsg( hRwsSrv, aRwsSrv, (MPARAM)pHdr, (MPARAM)hRwsCli))
    {
        pTI->msgId--;
        pHdr->Flags = 0;
        ERRNBR( RWSCLI_WINPOSTMSG)
    }

    // if there's a notify window, advise we're entering the loop
    PostNotification( pTI, RWSN_ENTER, 0);

    // this is a do-forever loop
    while (rc == 0) {

        // look for:  a msg posted to our window, our "deferred event" flag,
        // or any msg - if the last is a WM_QUIT, repost it, then exit

        if (!WinPeekMsg( 0, &qmsg, hRwsCli, 0, 0, PM_REMOVE))
            if (pTI->flags & RWSF_RESEND) {
                qmsg.hwnd = hRwsCli;
                qmsg.msg  = WM_SEM1;
                qmsg.mp1 = qmsg.mp2 = 0;
            }
            else
            if (!WinGetMsg( 0, &qmsg, 0, 0, 0)) {
                WinPostMsg( qmsg.hwnd, qmsg.msg, qmsg.mp1, qmsg.mp2);
                ERRNBR( RWSCLI_WM_QUIT)
            }

        // if the msg is for our window, signal that Rws dispatched it;
        // on return, exit the loop if the return is encoded appropriately
        if (qmsg.hwnd == hRwsCli) {
            pTI->flags = RWSF_DISPATCH;
            mres = WinDispatchMsg( 0, &qmsg);
            pTI->flags = 0;
            if (HIUSHORT( mres) == aRwsSrv) {
                rc = LOUSHORT( mres);
                break;
            }
        }
        // dispatch the msg to another window
        else {
            pTI->flags = 0;
            WinDispatchMsg( 0, &qmsg);
        }
    }

} while (fFalse);

    // stop the timer, then signal that we're not in a dispatch
    WinStopTimer( 0, hRwsCli, RWSDISPATCHTIMER);
    pTI->pHdr = 0;

    // if there's a notify window, advise we've exited dispatch
    PostNotification( pTI, RWSN_EXIT, rc);

    return (rc);
}

/****************************************************************************/

MRESULT _System ClientWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)

{
    MRESULT     mRtn = 0;
    BOOL        fRtn = FALSE;
    BOOL        fEvent = FALSE;
    ULONG       rc = 0;
    BOOL        fDispatch;
    PTIDINFO    pTI;
    PRWSHDR     pHdr;

    pTI = GetThreadInfo();
    if (!pTI)
        return (0);

do {
    fDispatch = (pTI->flags & RWSF_DISPATCH) ? TRUE : FALSE;
    pTI->flags = 0;

    switch (msg) {

      case WM_TIMER:
        // not our timer
        if (SHORT1FROMMP( mp1) != RWSDISPATCHTIMER) {
            mRtn = WinDefWindowProc( hwnd, msg, mp1, mp2);
            break;
        }

        // if we're not waiting for a msg, timer should be off
        pHdr = pTI->pHdr;
        if (!pHdr) {
            WinStopTimer( 0, hwnd, RWSDISPATCHTIMER);
            break;
        }        

        // if the hdr isn't marked already, see if it should time-out
        if (!(pHdr->Flags & (RWSF_RCVD | RWSF_TIMEOUT)) &&
            WinGetCurrentTime( 0) - pTI->ulTime >= ulTimeout) {
                pHdr->Flags |= RWSF_TIMEOUT;
                fEvent = TRUE;
        }

        fRtn = TRUE;
        break;

      case WM_SEM1:
        // our "check for mail" msg
        pHdr = pTI->pHdr;
        if (pHdr)
            fRtn = TRUE;
        break;

      case WM_DESTROY:
        // cleanup
        pTI->hwnd = 0;
        if (pTI->pHdr) {
            pTI->pHdr->Flags = 0;
            RwsFreeMem( pTI->pHdr);
            pTI->pHdr = 0;
        }
        break;

      default:
        // msg from server
        if (msg == aRwsSrv) {
            pHdr = (PRWSHDR)mp1;
            if (!pHdr)
                break;

            // not the hdr we're waiting for - free it
            if (pHdr != pTI->pHdr) {
                pHdr->Flags = 0;
                RwsFreeMem( pHdr);
                break;
            }

            // mark as received
            pHdr->Flags |= RWSF_RCVD;
            fEvent = TRUE;
            fRtn = TRUE;
            break;
        }

        // msg from RwsCancelDispatch
        if (msg == aRwsCancel) {
            pHdr = pTI->pHdr;
            if (!pHdr)
            break;

            // force a timeout
            if (!(pHdr->Flags & (RWSF_RCVD | RWSF_TIMEOUT)))
                pHdr->Flags |= RWSF_TIMEOUT;

            // post a notification to user's window, if requested
            if (mp1 && mp2)
                WinPostMsg( (HWND)mp1, WM_CONTROL,
                            MPFROM2SHORT( mp2, RWSN_CANCEL), (MPARAM)fDispatch);
            else
                fEvent = TRUE;

            fRtn = TRUE;
            break;
        }

        // other msgs (if any) should speed up the timer when fDispatch is off
        if (pTI->pHdr && !fDispatch)
            pTI->flags = RWSF_RESEND;
        mRtn = WinDefWindowProc( hwnd, msg, mp1, mp2);
        break;

    } // end switch

    // nothing more to do - exit
    if (!fRtn)
        break;

    // this msg wasn't dispatched by RwsDispatch()'s loop, so we can't
    // break out of it - defer action until that loop is active again
    if (!fDispatch) {
        pTI->flags = RWSF_RESEND;
        // if there's a notify window, advise that an event was blocked
        if (fEvent)
            PostNotification( pTI, RWSN_BLOCKED, 0);
        break;
    }

    // if the received or timeout flags are on, create a return
    // value that will cause RwsDispatch() to exit its loop

    if (pHdr->Flags & RWSF_RCVD) {
        // if the msgIds don't match, we've got serious problems!
        if (pHdr->MsgId != pTI->msgId)
            rc = RWSCLI_UNEXPECTEDMSG;
        else
            if (!rc)
                rc = pHdr->Rc;

        // enable this hdr to be deallocated
        pHdr->Flags = 0;
    }
    else
    // leave the flags on so the hdr can't be deallocated
    if (pHdr->Flags & RWSF_TIMEOUT)
        rc = RWSCLI_TIMEOUT;
    else
        break;

    // this will cause RwsDispatch()'s loop to exit
    mRtn = MRFROM2SHORT( rc, aRwsSrv);

} while (fFalse);

    // if RwsDispatch()'s loop is not active, speed up the timer
    // so it will have something to handle when it gets control
    if (pTI->flags == RWSF_RESEND)
        WinStartTimer( 0, hwnd, RWSDISPATCHTIMER, 500);

    return (mRtn);
}

/****************************************************************************/

void        PostNotification( PTIDINFO pTI, ULONG idEvent, ULONG ulParm)

{
    if (pTI->hNotify)
        if (!pTI->idNotify)
            pTI->hNotify = 0;
        else
           if (!WinPostMsg( pTI->hNotify, WM_CONTROL,
                            MPFROM2SHORT( pTI->idNotify, idEvent),
                            (MPARAM)ulParm) &&
                !WinIsWindow( 0, pTI->hNotify))
                pTI->hNotify = pTI->idNotify = 0;

    return;
}

/****************************************************************************/

ULONG   _System RwsDispatchStatus( PBOOL pfReady)

{
    ULONG       rc = 0;
    PTIDINFO    pTI;

do {
    pTI = GetThreadInfo();
    if (!pTI)
        ERRNBR( RWSCLI_BADTID)

    if (!pTI->pHdr)
        ERRNBR( RWSCLI_NOTINDISPATCH)

    if (pfReady)
        if (pTI->pHdr->Flags & (RWSF_RCVD | RWSF_TIMEOUT))
            *pfReady = TRUE;
        else
            *pfReady = 0;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

ULONG   _System RwsCancelDispatch( HWND hNotify, ULONG idNotify)

{
    ULONG       rc = 0;
    PTIDINFO    pTI;

do {
    pTI = GetThreadInfo();
    if (!pTI)
        ERRNBR( RWSCLI_BADTID)

    if (!pTI->pHdr)
        ERRNBR( RWSCLI_NOTINDISPATCH)

    // get/create the atom we use to cancel a dispatch
    // (intentionally deferred until needed)
    if (aRwsCancel == 0)
    {
        aRwsCancel = WinAddAtom( WinQuerySystemAtomTable(), szRwsCancel);
        if (aRwsCancel == 0)
            ERRNBR( RWSCLI_NOATOM)
    }

    if (!WinPostMsg( pTI->hwnd, aRwsCancel, (MPARAM)hNotify, (MPARAM)idNotify))
        ERRNBR( RWSCLI_WINPOSTMSG)

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// WM_CONTROL msgs will be posted to hNotify in this format:
//  mp1 = MPFROM2SHORT( idNotify, RWSN_*)
//  mp2 - depends on RWSN_ code:
//          RWSN_ENTER      mp2 = 0
//          RWSN_EXIT       mp2 = rc returned by RwsDispatch()
//          RWSN_BLOCKED    mp2 = 0
//          RWSN_CANCEL     mp2 = (RwsDispatch will now exit) ? TRUE : FALSE

ULONG   _System RwsNotify( HWND hNotify, ULONG idNotify)

{
    ULONG       rc = 0;
    PTIDINFO    pTI;

do {
    pTI = GetThreadInfo();
    if (!pTI)
        ERRNBR( RWSCLI_BADTID)

    if (hNotify && idNotify) {
        pTI->hNotify  = hNotify;
        pTI->idNotify = idNotify;
    }
    else
    if (!hNotify && !idNotify)
        pTI->hNotify = pTI->idNotify = 0;
    else
        ERRNBR( RWSCLI_MISSINGARGS)

} while (fFalse);

    return (rc);
}

/****************************************************************************/
/****************************************************************************/

// called by applications that use any of the ...Async() functions
// to obtain the message ID used by RwsServer for its reply messages

ULONG   _System RwsGetServerMsgID( PULONG pulMsgID)

{
    ULONG       rc = RWSCLI_NOATOM;

    if (pulMsgID && aRwsSrv)
    {
        *pulMsgID = aRwsSrv;
        rc = 0;
    }

    return (rc);
}

/****************************************************************************/

// this is for clients that want to handle replies from the server themselves

ULONG   _System RwsDispatchAsync( HWND hwnd, PRWSHDR pHdr)

{
    ULONG       rc = 0;

    if (pHdr == 0 || hwnd == 0)
        rc = RWSCLI_BADDISPATCHARG;
    else
    if (ValidateServerWindow() == FALSE)
        rc = RWSCLI_SERVERNOTFOUND;
    else
    if (WinPostMsg( hRwsSrv, aRwsSrv, (MPARAM)pHdr, (MPARAM)hwnd) == FALSE)
        rc = RWSCLI_WINPOSTMSG;

    return (rc);
}

/****************************************************************************/
/* Memory Management functions                                              */
/****************************************************************************/

// alloc mem for server request block using DosSubAllocMem();
// this function is not exported currently

ULONG   _System RwsGetMem( PRWSHDR* ppHdr, ULONG size)

{
    ULONG       rc = 0;
    PVOID       pMem;

do
{
    // guarantee an invalid ptr if there's a failure
    *ppHdr = 0;

    // eliminate requests that are clearly out of range
    if (size < (sizeof(RWSHDR) + 2*sizeof(RWSDSC)) ||
        size > RWSMEMSIZE-64)
        ERRNBR( RWSCLI_BADMEMSIZE)

    // DosSubAllocMem allocates in blocks of 8 bytes, so round up
    size = (size+7) & (ULONG)~7;
    if (DosSubAllocMem( pRwsMem, &pMem, size))
        ERRNBR( RWSCLI_OUTOFMEM)

    // clear the mem, save its size, then set return to a valid ptr
    memset( pMem, 0, size);
    ((PRWSHDR)pMem)->Size = size;
    *ppHdr = (PRWSHDR)pMem;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// free the server request block; passing a null ptr is not an error;
// RwsDispatch() & FreeOverdueMsgs() clear MsgId when they receive a
// reply from the server;  if MsgId is not zero, then the server may
// still be using the block, so don't free it

ULONG   _System RwsFreeMem( PRWSHDR pHdr)

{
    ULONG       rc = 0;

    if (pHdr)
    {
        if (pHdr->Flags & RWSF_INUSE)
            rc = RWSCLI_MEMINUSE;
        else
        if (DosSubFreeMem( pRwsMem, (PVOID)pHdr, pHdr->Size))
            rc = RWSCLI_BADMEMPTR;
    }

    return (rc);
}

/****************************************************************************/
/* Client App Utility functions                                             */
/****************************************************************************/

// Returns the final value for the specified arg after any copying or
// conversion has been done (e.g. for RWSI_OHNDL you get an object ptr,
// for RWSO_PPSTR you get a ptr to the string).  The optional pSize is
// slightly misnamed:  if the return appears to be a dword, size is 0;
// if the return is a ptr to string, it is -1;  otherwise, it is the
// calculated size of the buffer the return value points to.  Also,
// unlike other Rws functions this returns a value, not a return code.

ULONG   _System RwsGetResult( PRWSHDR pHdr, ULONG ndxArg, PULONG pSize)

{
    ULONG       ulRtn = (ULONG)-1;
    ULONG       ctr;
    PRWSDSC     pArg;

do
{
    // ensure pHdr is valid
    if (pHdr == 0)
        break;

    // ensure the ndx is valid
    if (ndxArg > pHdr->Cnt)
        break;

    // find the requested arg
    pArg = (CALCPROCPTR( pHdr))->pnext;
    for (ctr=0; ctr < ndxArg && pArg; ctr++)
        pArg = pArg->pnext;

    if (pArg == 0)
        break;

    // if RwsServer generated an error while handling this arg, exit
    if (pArg->rc)
        break;

    // if pSize was supplied, init to zero
    if (pSize)
        *pSize = 0;

    switch (GETP( pArg->type))
    {
      case GET_VOID:
        break;

      case GET_ASIS:
        if (GIVEP( pArg->type) == GIVE_PPVOID)
            ulRtn = *(PULONG)pArg->value;
        else
            ulRtn = pArg->value;

        if (pSize)
        {
            if (GIVEP( pArg->type) == GIVE_PSTR)
                *pSize = (ULONG)-1;
            else
            if (GIVEP( pArg->type) == GIVE_PBUF)
                *pSize = pArg->cbgive;
        }
        break;

      case GET_PSTR:
      case GET_PPSTR:
        ulRtn = (ULONG)pArg->pget;
        if (pSize)
            *pSize = (ULONG)-1;
        break;

      case GET_PBUF:
      case GET_PPBUF:
        ulRtn = (ULONG)pArg->pget;
        if (pSize == 0)
            break;

        ctr = GETQ( pArg->type) & (UCHAR)COPY_MASK;
        if (ctr >= COPY_CNTFIRST && ctr <= COPY_CNTLAST)
            *pSize = GetResultCnt( pArg->pget,
                                   (CALCPROCPTR( pHdr))->pnext, ctr);
        else
            *pSize = pArg->cbget;

        break;

      case GET_COPYICON:
      case GET_COPYMINI:
        ulRtn = *(PULONG)pArg->pget;
        break;

      case GET_CONV:
        switch (GETQ( pArg->type) & (UCHAR)COPY_MASK)
        {
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_OBJICON:
          case CONV_OBJMINI:
          case CONV_CLSOBJ:
          case CONV_CLSICON:
          case CONV_CLSMINI:
            ulRtn = *(PULONG)pArg->pget;
            break;

          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_OBJNAME:
          case CONV_OBJTITLE:
          case CONV_OBJID:
          case CONV_OBJFLDR:
          case CONV_CLSNAME:
          case CONV_SOMID:
            ulRtn = (ULONG)pArg->pget;
            if (pSize)
                *pSize = (ULONG)-1;
            break;
        }
    }

} while (fFalse);

    return (ulRtn);
}

/****************************************************************************/

// used when a function copies data into a buffer & identifies the count
// at the head of the buffer, in the return value, or in another arg

ULONG       GetResultCnt( PVOID pBuf, PRWSDSC pRtn, ULONG ndx)

{
    ULONG       ulRtn = 0;
    ULONG       ctr;
    PRWSDSC     pArg;

    // the first 4 bytes of the buffer identify its size
    if (ndx == COPY_CNTULONG)
        ulRtn = *(PULONG)pBuf;
    else
    // the first 2 bytes of the buffer identify its size
    if (ndx == COPY_CNTUSHORT)
        ulRtn = *(PUSHORT)pBuf;
    else
    // the buffer is an array of ULONGS, the last element is zero
    if (ndx == COPY_CNTULZERO)
    {
        for (ctr=0; ((PULONG)pBuf)[ctr]; ctr++)
            {;}
        ulRtn = (ctr+1)*sizeof( ULONG);
    }
    else
    // the copy count is the function's return value
    if (ndx == COPY_CNTRTN)
    {
        if (pRtn->type == RWSR_ASIS)
            ulRtn = pRtn->value;
    }
    else
    // the copy count was returned in arg1-arg9
    {
        ndx -= COPY_CNTRTN;

        for (pArg=pRtn, ctr=0; ctr < ndx && pArg; ctr++)
            pArg = pArg->pnext;

        if (pArg && pArg->type == RWSI_PPVOID)
            ulRtn = *(PULONG)pArg->value;
    }

    return (ulRtn);
}

/****************************************************************************/

// returns a ptr to the specified arg's RWSDSC struct;
// note that args are numbered starting at 1 - arg0 is the return value

ULONG   _System RwsGetArgPtr( PRWSHDR pHdr, ULONG ndxArg, PRWSDSC* ppArg)

{
    ULONG       rc = 0;
    ULONG       ctr;
    PRWSDSC     pArg;

do
{
    // ensure pHdr is valid
    if (pHdr == 0)
        ERRNBR( RWSCLI_BADMEMPTR)

    // ensure the ndx is valid
    if (ndxArg > pHdr->Cnt)
        ERRNBR( RWSCLI_BADARGNDX)

    // find the requested arg
    pArg = (CALCPROCPTR( pHdr))->pnext;
    for (ctr=0; ctr < ndxArg && pArg; ctr++)
        pArg = pArg->pnext;

    if (pArg == 0)
        ERRNBR( RWSCLI_MISSINGARGS)

    // return the pointer to the requested arg
    *ppArg = pArg;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

ULONG   _System RwsGetRcString( ULONG rcIn, ULONG cbBuf, char * pszBuf)

{
    ULONG   rc = RWSCLI_ERRMSGNOTFOUND;

    if (pszBuf && cbBuf)
    {
        *pszBuf = 0;

        if (rcIn)
        {
            if (WinLoadString( 0, hmodCli, rcIn, cbBuf, pszBuf))
                rc = 0;
        }
        else
            if (cbBuf >= 12)
            {
                strcpy( pszBuf, "RWS_NOERROR");
                rc = 0;
            }
    }

    return (rc);
}

/****************************************************************************/

// return the current timeout, and optionally, the timeout that was
// explicitly set by the user;  apps can use the latter to determine
// whether the user changed the default to handle machine-specific needs

ULONG   _System RwsGetTimeout( PULONG pulSecs, PULONG pulUser)

{
    ULONG   rc = 0;

    if (!pulSecs)
        rc = RWSCLI_MISSINGARGS;
    else 
    {
        *pulSecs = ulTimeout / 1000;
        if (pulUser)
            *pulUser = ulUserTimeout / 1000;
    }

    return (rc);
}

/****************************************************************************/

// set the timeout to the specified value or reset it to its original value

ULONG   _System RwsSetTimeout( ULONG ulSecs)

{
    ULONG   rc = 0;

    if (ulSecs > 300)
        rc = RWSCLI_BADTIMEOUT;
    else
    {
        if (ulSecs > 0)
            ulTimeout = ulSecs * 1000;
        else
            if (ulUserTimeout)
                ulTimeout = ulUserTimeout;
            else
                ulTimeout = RWSTIMEOUT_DFLT * 1000;
    }

    return (rc);
}

/****************************************************************************/

// destroys the RWS object created earlier & removes our entry
// from the exit list since it isn't needed once the object is gone

ULONG   _System RwsClientTerminate( void)

{
    ULONG   rc = RWSCLI_TERMINATEFAILED;

    // if there's an object, try to destroy it;  regardless
    // of outcome, clear the object handle (if it failed once,
    // it's unlikely to succeed on the 2nd try)
    if (hobjRws)
    {
        if (WinDestroyObject( hobjRws))
            rc = 0;
        else
            ErrMsg( "unable to destroy RWS object - hObj= %x", hobjRws);

        hobjRws = 0;
    }

    // if there's an entry in the exit list, it's now pointless
    // so remove it;  regardless of outcome, clear the flag
    if (fExitList)
    {
        if (DosExitList( EXLST_REMOVE, RwsExitListProc))
            ErrMsg( "unable to remove exit list proc", 0);

        fExitList = FALSE;
    }

    return (rc);
}

/****************************************************************************/

void    _System RwsExitListProc( ULONG ulTermCode)

{
    ErrMsg( "RwsExitListProc - termination code= %x", ulTermCode);

    if (hobjRws)
        WinDestroyObject( hobjRws);

    DosExitList( EXLST_EXIT, 0);

    return;
}

/****************************************************************************/
/****************************************************************************/

void    ErrMsg( char * msg, ULONG ul)

{
    ULONG   cnt;
    char    szText[300];

    cnt = sprintf( szText, "%s: ", RWSCLIMOD);
    cnt += sprintf( &szText[cnt], msg, ul);
    strcpy( &szText[cnt], "\r\n");
    cnt += 2;

    DosWrite( 2, szText, cnt, &cnt);

    return;
}

/****************************************************************************/

