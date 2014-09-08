/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSSUTIL.C
// Remote Workplace Server - Server init & exception handling

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

#include "RWSSRV.H"

/****************************************************************************/

// This file contains server thread initialization, the window proc
// that receives msgs from the client, and the exception handler.
// It also contains the functions and window proc required to support
// operations on the WPS's primary (UI) thread.

/****************************************************************************/

// global data (declared extern in RWSSRV.H)
// 
// RWSFULLVERSION for v0.80 GA is 0x08000100

char            szCopyright[] = "RWS v" RWSVERSION " - (C)Copyright 2004-2007  R.L.Walsh";

ATOM            aRwsSrv = 0;        // the atom used for msg & window IDs
ATOM            aRwsUI = 0;         // used to identify the UI host window
BOOL            fFalse = FALSE;     // my compiler doesn't like while (FALSE)
BOOL            fUseHandler = TRUE; // turn on exception handling
HMUX            hmuxServer = 0;     // coordinates server & thread1 shutdown
HMODULE         hmodSOM = 0;        // som.dll's module handle
HMODULE         hmodRWS = 0;
HMQ             hmqWPS = 0;
HMQ             hmqServer = 0;      // the server thread's HMQ
HWND            hThread1 = 0;       // the server's object window on thread1
PID             pidWPS = 0;

SOMClass *      clsSOMClass = 0;    // class objects we reference frequently
SOMClass *      clsWPAbstract = 0;
SOMClass *      clsWPDesktop = 0;
SOMClass *      clsWPFileSystem = 0;
SOMClass *      clsWPShadow = 0;
SOMClass *      clsWPTransient = 0;

char            szRwsCls[] = RWSCLASSNAME;
char            szRwsSrv[] = RWSSRVNAME;
char            szRwsUI[]  = RWSUINAME;
char            szRwsFQPath[260] = "";

#define RWSINITMP1  ((MPARAM)READABLEULONG( 'R','W','S','\0'))
#define RWSINITMP2  ((MPARAM)READABLEULONG( 'I','N','I','T'))

/****************************************************************************/

void    _System ServerThreadProc( ULONG ul);
void    _System RealServerThreadProc( ULONG ulRtnAddr, ULONG hmodUnload);

void            ServerCloseHmux( void);
BOOL            ServerInitHandlerFlag( void);
BOOL            ServerThreadInit( void);
MRESULT _System ServerWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

ULONG           Thread1Init( void);
int     _System Thread1InputHook( HAB hab, PQMSG pqmsg, ULONG fs);
MRESULT _System Thread1WndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void            Thread1AddSem( HWND hwnd);
void            Thread1CloseSem( HWND hwnd);

ULONG   _System RwsExceptionHandler( PEXCEPTIONREPORTRECORD pReport,
                                     PEXCEPTIONREGISTRATIONRECORD pRegRec,
                                     PCONTEXTRECORD pContext, PVOID pVoid);

// suppress an "unreferenced static variable" warning
void *          UtilDummy( void) { return (void*)__somtmp_SOMObject; }

/****************************************************************************/
/* Thread Support functions                                                 */
/****************************************************************************/

// This function is part of a kludge to ensure that RwsSrv dll won't crash
// the WPS if SOM tries to unload it before this thread has terminated.
// On entry, RealServerThreadProc() reloads the dll to lock it in place.
// Just before  exiting, it changes the stack so its return becomes a jump
// to DosFreeModule().  The return address that function sees is the first
// argument we passed to RealServerThreadProc(), i.e. ServerThreadProc()'s
// own return address.  This eliminates the possibility that DosFreeModule()
// might return to an address that's no longer valid.

void    _System ServerThreadProc( ULONG ul)
{
    // pass our return address & the a place to put the module handle
    RealServerThreadProc( (&ul)[-1], 0);

    // this return will never be reached unless
    // RealServerThreadProc() is unable to reload this dll
    return;
}

/****************************************************************************/

// this is the server thread's "main()";  it initializes global variables
// and PM, creates the window that clients will post msgs to, then enters
// a msg loop

void    _System RealServerThreadProc( ULONG ulRtnAddr, ULONG hmodUnload)

{
    BOOL            fRtn = FALSE;
    HAB         	hab;
    HMQ         	hmq;
    HWND            hwnd;
    QMSG        	qmsg;
    char            szBuf[16];
    RWSXCPTREGREC   rwsregrec;

do
{
    // reloading the dll should ensure that it doesn't get unloaded
    // while this thread is still active
    if (szRwsFQPath[0] == 0 ||
        DosLoadModule( szBuf, sizeof(szBuf), szRwsFQPath, &hmodUnload))
    {
        hmodUnload = 0;
        somPrintf( RWSSRVNAME ": ServerThreadProc - unable to reload dll\n");
    }

    // if exception handling is on, setup our handler - this one should
    // only be triggered by an access violation during init and will
    // terminate the thread;  exceptions while processing client requests
    // should be caught by Resolve()'s handler and continue execution

    if (ServerInitHandlerFlag())
    {
        rwsregrec.regrec.prev_structure = 0;
        rwsregrec.regrec.ExceptionHandler = &RwsExceptionHandler;

        // if an exception has occurred, abort
        if (setjmp( rwsregrec.jmpbuf))
        {
            somPrintf( RWSSRVNAME ": ServerThreadProc - Access violation handled\n");
            break;
        }

        // if we can't set the handler, abort
        if (DosSetExceptionHandler( &rwsregrec.regrec))
        {
            somPrintf( RWSSRVNAME ": ServerThreadProc - DosSetExceptionHandler failed\n");
            break;
        }
    }

    // init PM
    hab = WinInitialize( 0);
    if (!hab)
        break;

    hmq = WinCreateMsgQueue( hab, 0);
    if (!hmq)
        break;

    // init the globals
    if (!ServerThreadInit())
        break;

    // use a WC_STATIC because it's quick & easy (that's what IBM does)
    hwnd = WinCreateWindow( HWND_OBJECT, WC_STATIC, "",
                            0, 0, 0, 0, 0, 0, HWND_TOP,
                            aRwsSrv, 0, 0);
    if (!hwnd)
        break;

    somPrintf( RWSSRVNAME ": ServerThreadProc - hab= %x  hwnd= %x\n",
               hab, hwnd);

    // this is intended as a fail-safe way for clients to confirm they've
    // located the correct window:  while it's unlikely another window
    // would use our ID, it's utterly improbable that another's QWL_USER
    // would have this value
    if (!WinSetWindowULong( hwnd, QWL_USER, aRwsSrv))
        break;

    // throw away the original wndproc & replace it with our own
    if (!WinSubclassWindow( hwnd, &ServerWndProc))
        break;

    // create a window on the WPS' primary thread that will
    // host objects' popup menus
    Thread1Init();

    // wpclsUnInitData() will use this when it's time to unload
    hmqServer = hmq;

    // when the Task List and the default dialog proc close a top-level
    // window, they post a WM_QUIT with the hwnd in mp1 or mp2;  if we
    // don't handle these msgs, RwsServer will terminate prematurely
    do {
        if (WinGetMsg( hab, &qmsg, 0, 0, 0))
            WinDispatchMsg( hab, &qmsg);
        else
        if (qmsg.mp1 == 0)
        {
            // standard WM_QUIT - mp1==mp2==0
            if (qmsg.mp2 == 0)
                fRtn = TRUE;
            else
            // tasklist - mp1==0, mp2==hwnd
            if (qmsg.mp2 == (MP)qmsg.hwnd)
                WinDestroyWindow( qmsg.hwnd);
        }
        else
        // WM_CLOSE & WM_SYSCOMMAND/SC_CLOSE - mp1==(window), hwnd==mp2==0
        if (qmsg.mp2 == 0 && qmsg.hwnd == 0)
            WinDestroyWindow( (HWND)qmsg.mp1);

    } while (fRtn == FALSE);

} while (fFalse);

    if (fRtn == FALSE)
        DosBeep( 330, 90);

    if (hwnd)
        WinDestroyWindow( hwnd);
    if (hmq)
        WinDestroyMsgQueue( hmq);
    if (hab)
        WinTerminate( hab);

    ServerCloseHmux();

    // unset the handler
    if (fUseHandler)
        DosUnsetExceptionHandler( &rwsregrec.regrec);

    somPrintf( RWSSRVNAME ": server thread is terminating - fRtn= %d  hmod= %x\n",
               fRtn, hmodUnload);

    // if the dll was reloaded above, this will change our return address
    // into a jump to DosFreeModule();  it's return & argument will be the
    // args passed to this function
    if (hmodUnload)
        (&ulRtnAddr)[-1] = (ULONG)&DosFreeModule;

    // signal that this thread has terminated
    hmqServer = 0;

    return;
}

/****************************************************************************/

#ifndef ERROR_TIMEOUT
  #define ERROR_TIMEOUT     640
#endif

void            ServerCloseHmux( void)

{
    ULONG   	rc;
    ULONG       cnt = 64;
    ULONG       ctr;
    SEMRECORD   asemRec[64];

do {
    if (!hmuxServer) {
        somPrintf( "CloseServerHmux: hmuxServer is null\n");
        break;
    }

    // see if a wait is necessary
    rc = DosWaitMuxWaitSem( hmuxServer, 0, &ctr);
    if (rc == ERROR_TIMEOUT) {
        somPrintf( RWSSRVNAME ": server thread is about to wait for its hmux sem\n");
        rc = DosWaitMuxWaitSem( hmuxServer, (ULONG)-1, &ctr);
    }

    if (rc && rc != 286)  // 286 = ERROR_EMPTY_MUXWAIT
        somPrintf( "CloseServerHmux: DosWaitMuxWaitSem - rc= %d\n", rc);

    rc = DosQueryMuxWaitSem( hmuxServer, &cnt, asemRec, &ctr);
    if (rc) {
        somPrintf( "CloseServerHmux: DosQueryMuxWaitSem - rc= %d\n", rc);
        break;
    }

    for (ctr = 0; ctr < cnt; ctr++) {
        rc = DosDeleteMuxWaitSem( hmuxServer, asemRec[ctr].hsemCur);
        if (rc)
            somPrintf( "CloseServerHmux: sem #%d  DosDeleteMuxWaitSem - rc= %d\n", ctr, rc);

        rc = DosCloseEventSem( (ULONG)asemRec[ctr].hsemCur);
        if (rc)
            somPrintf( "CloseServerHmux: sem #%d  DosCloseEventSem - rc= %d\n", ctr, rc);
    }

    rc = DosCloseMuxWaitSem( hmuxServer);
    if (rc)
        somPrintf( "CloseServerHmux: DosCloseMuxWaitSem - rc= %d\n", rc);

    hmuxServer = 0;

    DosSleep (100);

} while (fFalse);

    return;
}

/****************************************************************************/

// exception handling is on by default - scan the environment to see
// if should be turned off;  set fUseHandler accordingly

BOOL            ServerInitHandlerFlag( void)

{
    ULONG       cnt;
    char *      ptr;
    COUNTRYCODE cc = {0, 0};
    char        szText[8];

do
{
    fUseHandler = TRUE;
    if (DosScanEnv( "RWSEXCEPTIONHANDLER", &ptr))
        break;

    cnt = strlen( ptr);
    if (cnt > 3)
        break;

    strcpy( szText, ptr);
    DosMapCase( cnt, &cc, szText);

    if (strcmp( szText, "OFF") == 0 ||
        strcmp( szText, "NO") == 0 ||
        strcmp( szText, "0") == 0)
    {
        fUseHandler = FALSE;
        somPrintf( RWSSRVNAME ": Exception handling is OFF\n");
    }

} while (fFalse);

    return (fUseHandler);
}

/****************************************************************************/

// get the atom we use as a message value & window ID, the module handle
// for som.dll, then get the class objects we'll reference frequently;
// all of these classes should already be loaded, so we use _somClassFromId
// rather than _somFindClass

BOOL            ServerThreadInit( void)

{
    BOOL        fRtn = FALSE;
    ULONG       rc;
    PPIB        ppib;
    PTIB        ptib;
    somId       classid;
    SEMRECORD   semRec;
    char        szText[16];

do
{
    DosGetInfoBlocks( &ptib, &ppib);
    pidWPS = ppib->pib_ulpid;

    // create a muxwait without any sems - as windows are created and
    // destroyed on Thread1, they'll add & delete sems to/from this muxwait
    semRec.hsemCur = 0;
    semRec.ulUser = 0;

    rc = DosCreateMuxWaitSem( 0, &hmuxServer, 0, &semRec, DCMW_WAIT_ALL);
    if (rc)
    {
        somPrintf( "ServerThreadInit: DosCreateMuxWaitSem - rc= %d  hmux= %x\n",
                   rc, hmuxServer);
        break;
    }

    // get our atom
    aRwsSrv = WinAddAtom( WinQuerySystemAtomTable(), szRwsSrv);
    if (aRwsSrv == 0)
        break;

    // get our UI host atom
    aRwsUI = WinAddAtom( WinQuerySystemAtomTable(), szRwsUI);
    if (aRwsUI == 0)
        break;

    // get som.dll's module handle
    if (DosLoadModule( szText, sizeof( szText), "SOM", &hmodSOM))
        break;

    // the SOMClass class object
    classid = somIdFromString( "SOMClass");
    clsSOMClass = _somClassFromId( SOMClassMgrObject, classid);
    SOMFree( classid);
    if (clsSOMClass == 0)
        break;

    // the WPTransient class object
    classid = somIdFromString( "WPTransient");
    clsWPTransient = _somClassFromId( SOMClassMgrObject, classid);
    SOMFree( classid);
    if (clsWPTransient == 0)
        break;

    // the WPAbstract class object
    classid = somIdFromString( "WPAbstract");
    clsWPAbstract = _somClassFromId( SOMClassMgrObject, classid);
    SOMFree( classid);
    if (clsWPAbstract == 0)
        break;

    // the WPFileSystem class object
    classid = somIdFromString( "WPFileSystem");
    clsWPFileSystem = _somClassFromId( SOMClassMgrObject, classid);
    SOMFree( classid);
    if (clsWPFileSystem == 0)
        break;

    // the WPDesktop class object
    classid = somIdFromString( "WPDesktop");
    clsWPDesktop = _somClassFromId( SOMClassMgrObject, classid);
    SOMFree( classid);
    if (clsWPDesktop == 0)
        break;

    // the WPShadow class object
    classid = somIdFromString( "WPShadow");
    clsWPShadow = _somClassFromId( SOMClassMgrObject, classid);
    SOMFree( classid);
    if (clsWPShadow == 0)
        break;

    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

// this handles only one msg:  the one identified by our atom;  if it can
// access the gettable shared memory pointer in mp1, it calls the server's
// primary function, frees the memory, then posts a reply to the client;
// if it can't get the memory, it lets the client know immediately

MRESULT _System ServerWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)

{
    ULONG       rc;

    if (msg != aRwsSrv)
        return (WinDefWindowProc( hwnd, msg, mp1, mp2));

    if (DosGetSharedMem( (PVOID)((ULONG)mp1 & 0xffff0000), PAG_READ | PAG_WRITE))
        rc = RWSSRV_DOSGETMEM;
    else
    {
        rc = Resolve( (PRWSHDR)mp1, (HWND)mp2);
        if (!(rc & RWSF_NOFREE))
            DosFreeMem( (PVOID)((ULONG)mp1 & 0xffff0000));
    }

    if (!(rc & RWSF_NOREPLY))
        WinPostMsg( (HWND)mp2, aRwsSrv, mp1, (MP)LOUSHORT( rc));

    return (0);
}

/****************************************************************************/
#if 0

// this handles only one msg:  the one identified by our atom;  if it can
// access the gettable shared memory pointer in mp1, it calls the server's
// primary function, frees the memory, then posts a reply to the client;
// if it can't get the memory, it lets the client know immediately

MRESULT _System ServerWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)

{
    ULONG       rc;
    USHORT      id;

    if (msg != aRwsSrv)
        return (WinDefWindowProc( hwnd, msg, mp1, mp2));

    if (DosGetSharedMem( (PVOID)((ULONG)mp1 & 0xffff0000), PAG_READ | PAG_WRITE))
        rc = RWSSRV_DOSGETMEM;
    else
    {
        id = ((PRWSHDR)mp1)->MsgId;
        rc = Resolve( (PRWSHDR)mp1, (HWND)mp2);
        if (!(rc & RWSF_NOFREE))
            DosFreeMem( (PVOID)((ULONG)mp1 & 0xffff0000));
    }

    if (!(rc & RWSF_NOREPLY))
        WinPostMsg( (HWND)mp2, aRwsSrv, mp1, (MP)LOUSHORT( rc));

    if (!(rc & RWSF_NOREPLY)) {
        static HWND     hReply = 0;
        static MPARAM   hdr = 0;
        static MPARAM   rtn = 0;
        if (id & 1) {
            hReply = (HWND)mp2;
            hdr = mp1;
            rtn = (MP)LOUSHORT( rc);
        }
        else {
            WinPostMsg( (HWND)mp2, aRwsSrv, mp1, (MP)LOUSHORT( rc));
            WinPostMsg( hReply, aRwsSrv, hdr, rtn);
        }
    }

    return (0);
}

#endif
/****************************************************************************/
/* UI Support functions - RWS thread                                        */
/****************************************************************************/

// To avoid compromising the integrity of RwsServer's thread (and to get
// some things to work properly, such as certain menus), RWS processes
// RwsCommands that open windows on the WPS's primary (UI) thread rather
// than its own.  RWS accomplishes this by hooking the primary thread's
// msg queue, then creating a host window in the hook procedure.  Later,
// when a command is handled, code running on the RWS thread posts a msg
// to the host window, causing it take the appropriate actions.

ULONG           Thread1Init( void)

{
    ULONG       rc = 0;

do
{
    // get the primary thread's HMQ using this undocumented API
    // (we could also get the desktop folder's hwnd & query its hmq)
    hmqWPS = Win32QueueFromID( 0, pidWPS, 1);
    if (hmqWPS == 0)
        ERRNBR( 1)

    // set a hook in that queue
    if (WinSetHook( 0, hmqWPS, HK_INPUT,
                    (PFN)Thread1InputHook, hmodRWS) == FALSE)
        ERRNBR( 2)

    // trigger the hook by posting our unique msg
    if (WinPostQueueMsg( hmqWPS, aRwsUI, RWSINITMP1, RWSINITMP2) == FALSE)
        ERRNBR( 3)

} while (fFalse);

    if (rc)
        somPrintf( RWSSRVNAME ": Thread1Init:  rc= %d\n", rc);

    return (rc);
}

/****************************************************************************/
/* UI Support functions - WPS primary thread                                */
/****************************************************************************/

// This hook only responds to one message and it must be constructed
// correctly;  all other msgs are passed on by returning FALSE.  When
// our unique msg is received, we try to create the UI host window
// and subclass it.  Regardless of outcome, we then uninstall the hook.

int     _System Thread1InputHook( HAB hab, PQMSG pqmsg, ULONG fs)

{

    if (pqmsg->msg == aRwsUI &&         // our unique msg ID
        pqmsg->hwnd == 0 &&             // this should be a queue msg
        fs == PM_REMOVE &&              // no false starts
        pqmsg->mp1 == RWSINITMP1 &&     // sanity check
        pqmsg->mp2 == RWSINITMP2)       // sanity check
    {
        // use a WC_STATIC to avoid registering a special class
        hThread1 = WinCreateWindow( HWND_OBJECT, WC_STATIC, "",
                                     0, 0, 0, 0, 0, 0, HWND_TOP,
                                     aRwsUI, 0, 0);

        if (hThread1 == 0)
            somPrintf( RWSSRVNAME ": Thread1InputHook:  WinCreateWindow failed\n");
        else
        // throw away the original wndproc because the subproc will
        // handle all msgs;  if subclassing fails, destroy the window
        if (WinSubclassWindow( hThread1, &Thread1WndProc))
            Thread1AddSem( hThread1);
        else
        {
            somPrintf( RWSSRVNAME ": Thread1InputHook:  WinSubclassWindow failed"
                       " - destroying hThread1= %x\n", hThread1);
            WinDestroyWindow( hThread1);
            hThread1 = 0;
        }

        // unhook the msg queue
        if (!WinReleaseHook( 0, hmqWPS, HK_INPUT,
                             (PFN)Thread1InputHook, hmodRWS))
            somPrintf( RWSSRVNAME ": Thread1InputHook:  unable to unhook queue\n");

        // prevent further handling of this msg
        return (TRUE);
    }

    // enable normal handling for all other msgs
    return (FALSE);
}

/****************************************************************************/

MRESULT _System Thread1WndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)

{
    BOOL                fJump = FALSE;
    MRESULT             mRtn = 0;
    RWSXCPTREGREC       rwsregrec;

do
{
    // if exception handling is on, setup our handler - if this fails, abort
    if (fUseHandler)
    {
        rwsregrec.regrec.prev_structure = 0;
        rwsregrec.regrec.ExceptionHandler = &RwsExceptionHandler;

        // if an exception has occurred, abort
        if (setjmp( rwsregrec.jmpbuf))
        {
            fJump = TRUE;
            somPrintf( RWSSRVNAME ": Thread1WndProc - Access violation handled\n");
            break;
        }

        if (DosSetExceptionHandler( &rwsregrec.regrec))
        {
            somPrintf( RWSSRVNAME ": Thread1WndProc - DosSetExceptionHandler failed\n");
            break;
        }
    }

    // a msg from the sever thread
    if (msg == aRwsUI)
    {
        Thread1ResolveCommand( hwnd, (PRWSHDR)mp1, (HWND)mp2);
        break;
    }

    // this is a "customized" version of the msg to avoid mishaps
    if (msg == WM_CLOSE)
    {
        if (mp1 == (MP)hwnd && hwnd == hThread1)
        {
            WinSetWindowPtr( hwnd, QWP_PFNWP, (PVOID)WinDefWindowProc);
            WinDestroyWindow( hwnd);
            hThread1 = 0;
            Thread1CloseSem( hwnd);
        }
        break;
    }

    // all msgs other than those above are handled here;
    // consequently, this window no longer behaves like a WC_STATIC
    mRtn = WinDefWindowProc( hwnd, msg, mp1, mp2);

} while (fFalse);

    // suppress the spurious error when a longjmp occurs
    if (fUseHandler && DosUnsetExceptionHandler( &rwsregrec.regrec) && !fJump)
        somPrintf( RWSSRVNAME ": Thread1WndProc - DosUnsetExceptionHandler failed\n");

    return (mRtn);
}

/****************************************************************************/

// Every RWS window created on Thread 1 gets an event sem
// that it either closes or posts when the window closes.
// The server thread uses these via an hmux to keep RWSxx.dll
// from being unloaded prematurely.

void            Thread1AddSem( HWND hwnd)

{
    ULONG       rc;
    SEMRECORD   semRec;

do {
    if (!hmuxServer) {
        somPrintf( "Thread1AddSem: hmuxServer is null\n");
        break;
    }

    semRec.ulUser = hwnd;
    rc = DosCreateEventSem( 0, (PULONG)&semRec.hsemCur, 0, FALSE);
    if (rc) {
        somPrintf( "Thread1AddSem: DosCreateEventSem - rc= %d\n", rc);
        break;
    }

    rc = DosAddMuxWaitSem( hmuxServer, &semRec);
    if (rc) {
        somPrintf( "Thread1AddSem: DosAddMuxWaitSem - rc= %d\n", rc);
        rc = DosCloseEventSem( (ULONG)semRec.hsemCur);
        if (rc)
            somPrintf( "Thread1AddSem: DosCloseEventSem - rc= %d\n", rc);
    }

} while (fFalse);

    return;
}

/****************************************************************************/

// When an RWS window on Thread 1 closes, this either deletes its
// event sem or posts it, depending on whether the server thread
// is waiting for it.

void            Thread1CloseSem( HWND hwnd)

{
    ULONG       rc;
    ULONG       cnt = 64;
    ULONG       ctr;
    SEMRECORD   asemRec[64];

do {
    if (!hmuxServer) {
        somPrintf( "Thread1CloseSem: hmuxServer is null\n");
        break;
    }

    rc = DosQueryMuxWaitSem( hmuxServer, &cnt, asemRec, &ctr);
    if (rc) {
        somPrintf( "Thread1CloseSem: DosQueryMuxWaitSem - rc= %d\n", rc);
        break;
    }

    for (ctr = 0; ctr < cnt; ctr++)
        if (asemRec[ctr].ulUser == hwnd)
            break;

    if (ctr >= cnt) {
        somPrintf( "Thread1CloseSem: unable to locate sem for hwnd= %x\n", hwnd);
        break;
    }

    // if the server isn't waiting for its hmux, we'll be able
    // to close the sem & delete it from the list
    rc = DosCloseEventSem( (ULONG)asemRec[ctr].hsemCur);
    if (!rc) {
        rc = DosDeleteMuxWaitSem( hmuxServer, asemRec[ctr].hsemCur);
        if (rc)
            somPrintf( "Thread1CloseSem: DosDeleteMuxWaitSem - rc= %d\n", rc);
        break;
    }

    // if the server is waiting, then post it & let the server clean up
    rc = DosPostEventSem( (ULONG)asemRec[ctr].hsemCur);
    if (rc)
        somPrintf( "Thread1CloseSem: DosPostEventSem - rc= %d\n", rc);

} while (fFalse);

    return;
}

/****************************************************************************/
/* Exception Handler                                                        */
/****************************************************************************/

ULONG   _System RwsExceptionHandler( PEXCEPTIONREPORTRECORD pReport,
                                     PEXCEPTIONREGISTRATIONRECORD pRegRec,
                                     PCONTEXTRECORD pContext, PVOID pVoid)

{
    if (pReport->fHandlerFlags == 0 &&
        pReport->ExceptionNum == XCPT_ACCESS_VIOLATION)
    {
        // show some basic info
        somPrintf( "\n" RWSSRVNAME ": access violation at %x - code= %x  addr= %x\n",
                   pReport->ExceptionAddress, pReport->ExceptionInfo[0],
                   pReport->ExceptionInfo[1]);

        // return to the scene of the crime
        longjmp( ((PRWSXCPTREGREC)pRegRec)->jmpbuf, RWSSRV_EXCEPTION);
    }

 	return (XCPT_CONTINUE_SEARCH);
}

/****************************************************************************/
/****************************************************************************/

