/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSSCMD.C
// Remote Workplace Server - Command functions

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

// This file handles RwsCommands.  After validation, commands that
// display windows are handled on the WPS's primary (UI) thread.

/****************************************************************************/

ULONG           ResolveCommand( PRWSHDR pHdr, HWND hReply);
ULONG           Cmd_PopupMenu( PRWSHDR pHdr, HWND hReply);
ULONG           Cmd_Open( PRWSHDR pHdr, HWND hReply);
ULONG           Cmd_Locate( PRWSHDR pHdr, HWND hReply);
ULONG           Cmd_ListObjects( PRWSHDR pHdr, HWND hReply);
ULONG           Cmd_OpenUsing( PRWSHDR pHdr, HWND hReply);
ULONG           Cmd_Delete( PRWSHDR pHdr, HWND hReply);

void            Thread1ResolveCommand( HWND hwnd, PRWSHDR pHdr, HWND hReply);
ULONG           Thread1DisplayPopup( PRWSHDR pHdr);
MRESULT _System MenuOwnerWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
ULONG           Thread1Open( PRWSHDR pHdr);
HWND            OpenObject( WPObject * pObj, ULONG ulView, BOOL fNew);
ULONG           Thread1Locate( PRWSHDR pHdr);
void            TimedSelect( WPObject * pObj, HWND hFrame);
BOOL            SelectObject( WPObject * pObj, HWND hCnr);
MRESULT _System LocateObjWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
ULONG           ListObjects( PRWSHDR pHdr);
ULONG           Thread1OpenUsing( PRWSHDR pHdr);

// suppress an "unreferenced static variable" warning
void *          CmdDummy( void) { return (void*)__somtmp_SOMObject; }

/****************************************************************************/

// used by TimedSelect()
LOCATEOBJ   aLocateObj[LOCATEOBJ_CNT];

/****************************************************************************/
// functions that run on the RwsSrv thread
/****************************************************************************/

// this performs basic validation & object resolution;
// for commands that display windows, it passes pHdr & hReply
// to Thread1ResolveCommand() via a msg posted to the UI host
// window;  on exit, it sets flags which prevent the current
// thread from freeing pHdr and posting a reply to the client

ULONG           ResolveCommand( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    BOOL        fThread1 = FALSE;
    PRWSDSC     pProc;

do
{
    pProc = CALCPROCPTR( pHdr);

    switch (pProc->value)
    {
        case RWSCMD_POPUPMENU:
            rc = Cmd_PopupMenu( pHdr, hReply);
            fThread1 = TRUE;
            break;

        case RWSCMD_OPEN:
            rc = Cmd_Open( pHdr, hReply);
            fThread1 = TRUE;
            break;

        case RWSCMD_LOCATE:
            rc = Cmd_Locate( pHdr, hReply);
            fThread1 = TRUE;
            break;

        case RWSCMD_LISTOBJECTS:
            rc = Cmd_ListObjects( pHdr, hReply);
            break;

        case RWSCMD_OPENUSING:
            rc = Cmd_OpenUsing( pHdr, hReply);
            fThread1 = TRUE;
            break;

        case RWSCMD_DELETE:
            rc = Cmd_Delete( pHdr, hReply);
            break;

        default:
            rc = RWSSRV_CMDBADCMD;
            break;
    }

    if (rc || fThread1 == FALSE)
        break;

    // does the thread 1 window exist?
    if (hThread1 == 0)
        ERRNBR( RWSSRV_CMDNOMENUWND)

    // signal the client not to free this pHdr;  without this,
    // it might get freed while thread 1 is still using it
    pHdr->Rc = (USHORT)-1;

    // post a msg to the UI host window on thread 1
    if (WinPostMsg( hThread1, aRwsUI, (MP)pHdr, (MP)hReply) == FALSE)
    {
        pHdr->Rc = 0;
        ERRNBR( RWSSRV_CMDPOSTMSGFAILED)
    }

    // tell ServerWndProc() not to free pHdr or reply
    // to the client - Thread1ResolveCommand will do that
    rc = (RWSF_NOFREE | RWSF_NOREPLY);

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// HWND RWSCMD_POPUPMENU( WPObject * somSelf, HWND hClient, PPOINTL pPtl)

ULONG           Cmd_PopupMenu( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pArg;
    PRWSDSC     pRtn;

do
{
    // is the arg count correct?
    if (pHdr->Cnt != 3)
        ERRNBR( RWSSRV_CMDBADARGCNT)

    // get rtn - confirm its type is correct
    pRtn = CALCRTNPTR( pHdr);
    if (pRtn->type != RWSR_ASIS)
        ERRNBR( RWSSRV_CMDBADRTNTYPE)

    // get arg1 (somSelf), then resolve it
    pArg = pRtn->pnext;
    ERRRTN( ResolveArgIn( pArg, pArg))
    ERRRTN( ResolveArgOut( pArg, pRtn, hReply))

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// HWND RWSCMD_OPEN( WPObject * somSelf, ULONG ulView, BOOL fNew)

ULONG           Cmd_Open( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pArg;
    PRWSDSC     pRtn;

do
{
    // is the arg count correct?
    if (pHdr->Cnt != 3)
        ERRNBR( RWSSRV_CMDBADARGCNT)

    // get rtn - confirm its type is correct
    pRtn = CALCRTNPTR( pHdr);
    if (pRtn->type != RWSR_ASIS)
        ERRNBR( RWSSRV_CMDBADRTNTYPE)

    // get arg1 (somSelf), then resolve it
    pArg = pRtn->pnext;
    ERRRTN( ResolveArgIn( pArg, pArg))
    pArg->value = (ULONG)ObjFromShadow( (SOMObject *)pArg->value);
    ERRRTN( ResolveArgOut( pArg, pRtn, hReply))

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// HWND RWSCMD_LOCATE( WPObject * somSelf)

ULONG           Cmd_Locate( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pArg;
    PRWSDSC     pRtn;

do
{
    // is the arg count correct?
    if (pHdr->Cnt != 1)
        ERRNBR( RWSSRV_CMDBADARGCNT)

    // get rtn - confirm its type is correct
    pRtn = CALCRTNPTR( pHdr);
    if (pRtn->type != RWSR_ASIS)
        ERRNBR( RWSSRV_CMDBADRTNTYPE)

    // get arg1 (somSelf), then resolve it
    pArg = pRtn->pnext;
    ERRRTN( ResolveArgIn( pArg, pArg))
    ERRRTN( ResolveArgOut( pArg, pRtn, hReply))


} while (fFalse);

    return (rc);
}

/****************************************************************************/

// ULONG RWSCMD_LISTOBJECTS( WPFolder * somSelf, PBYTE pBuf, PULONG pulFlags)

ULONG           Cmd_ListObjects( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pArg;
    PRWSDSC     pRtn;
    SOMClass *  pClass;

do
{
    // is the arg count correct?
    if (pHdr->Cnt != 3)
        ERRNBR( RWSSRV_CMDBADARGCNT)

    // get rtn - confirm its type is correct
    pRtn = CALCRTNPTR( pHdr);
    if (pRtn->type != RWSR_ASIS)
        ERRNBR( RWSSRV_CMDBADRTNTYPE)

    // get arg1 (somSelf), then resolve it
    pArg = pRtn->pnext;
    ERRRTN( ResolveArgIn( pArg, pArg))

    // confirm this object is a folder
    pClass = ClassFromName( "WPFolder");
    if (pClass == 0 || _somIsA( (SOMObject*)pArg->value, pClass) == FALSE)
        ERRNBR( RWSSRV_CMDBADARG)

    // convert object if requested
    ERRRTN( ResolveArgOut( pArg, pRtn, hReply))

    // get arg2 (pBuf), validate it
    pArg = pArg->pnext;
    if (pArg->type != RWSI_PBUF)
        ERRNBR( RWSSRV_CMDBADARGTYPE)
    if (pArg->cbgive/sizeof(ULONG) == 0)
        ERRNBR( RWSSRV_CMDBADARG)

    // get arg3 (pulFlags), validate it & eliminate invalid flags
    pArg = pArg->pnext;
    if (pArg->type != RWSI_PULONG)
        ERRNBR( RWSSRV_CMDBADARGTYPE)
    *(PULONG)pArg->value &= LISTOBJ_FILESHADOWS;

    rc = ListObjects( pHdr);

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// BOOL RWSCMD_OPENUSING( WPObject * somSelf, WPObject * pTarget)

ULONG           Cmd_OpenUsing( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pArg;
    PRWSDSC     pRtn;

do
{
    // is the arg count correct?
    if (pHdr->Cnt != 2)
        ERRNBR( RWSSRV_CMDBADARGCNT)

    // get rtn - confirm its type is correct
    pRtn = CALCRTNPTR( pHdr);
    if (pRtn->type != RWSR_ASIS)
        ERRNBR( RWSSRV_CMDBADRTNTYPE)

    // get arg1 (somSelf), then resolve it
    pArg = pRtn->pnext;
    ERRRTN( ResolveArgIn( pArg, pArg))
    ERRRTN( ResolveArgOut( pArg, pRtn, hReply))

    // resolve arg2 (pTarget)
    ERRRTN( ResolveArgIn( pArg->pnext, pArg))
    ERRRTN( ResolveArgOut( pArg->pnext, pRtn, hReply))

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// BOOL RWSCMD_DELETE( WPObject * somSelf, BOOL fForce)

ULONG           Cmd_Delete( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pArg;
    PRWSDSC     pRtn;
    WPObject *  pObj;

do
{
    // is the arg count correct?
    if (pHdr->Cnt != 2)
        ERRNBR( RWSSRV_CMDBADARGCNT)

    // get rtn - confirm its type is correct
    pRtn = CALCRTNPTR( pHdr);
    if (pRtn->type != RWSR_ASIS)
        ERRNBR( RWSSRV_CMDBADRTNTYPE)

    // get arg1 (somSelf), then resolve it
    pArg = pRtn->pnext;
    ERRRTN( ResolveArgIn( pArg, pArg))
    ERRRTN( ResolveArgOut( pArg, pRtn, hReply))
    pObj = (SOMObject*)pArg->value;

    // if the force flag is on, make object deletable
    if (pArg->pnext->value) {
        if (_somIsA( pObj, clsWPFileSystem) && !_wpSetAttr( pObj, 0))
            ERRNBR( RWSSRV_CMDFAILED)

        if (!_wpSetup( pObj, "NODELETE=NO"))
            ERRNBR( RWSSRV_CMDFAILED)
    }

    if (!_wpDelete( pObj, 0))
        ERRNBR( RWSSRV_CMDFAILED)

} while (fFalse);

    return (rc);
}

/****************************************************************************/
// functions that run on the WPS's UI thread
/****************************************************************************/

// This function is called by the menu host window in response to a msg
// posted by ResolveCommand.  It runs on the WPS's primary thread, not
// RwsServer's.  After processing the requested command,  it frees pHdr
// and posts a reply msg to the client.

void            Thread1ResolveCommand( HWND hwnd, PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pProc;

do
{
    pProc = CALCPROCPTR( pHdr);

    if (pProc->type != RWSP_CMD)
        ERRNBR( RWSSRV_CMDNOTACMD)

    switch (pProc->value)
    {
        case RWSCMD_POPUPMENU:
            rc = Thread1DisplayPopup( pHdr);
            break;

        case RWSCMD_OPEN:
            rc = Thread1Open( pHdr);
            break;

        case RWSCMD_LOCATE:
            rc = Thread1Locate( pHdr);
            break;

        case RWSCMD_OPENUSING:
            rc = Thread1OpenUsing( pHdr);
            break;

        default:
            rc = RWSSRV_CMDBADCMD;
            break;
    }

} while (fFalse);

    // processing is now complete - save the rc, then perform
    // the tasks we told ServerWndProc() not to handle
    pHdr->Rc = LOUSHORT(rc);
    DosFreeMem( (PVOID)((ULONG)pHdr & 0xffff0000));
    WinPostMsg( hReply, aRwsSrv, pHdr, (MP)rc);

    return;
}

/****************************************************************************/
/****************************************************************************/

// Handles RWSCMD_POPUPMENU on the WPS's UI thread and uses default for
// values not supplied by the client.  It creates a temporary window that
// will own the menu, then calls _wpDisplayMenu.

ULONG           Thread1DisplayPopup( PRWSHDR pHdr)

{
    ULONG       rc = RWSSRV_CMDFAILED;
    HWND        hClient;
    HWND        hHost;
    POINTL      ptl;
    PRWSDSC     pArg;
    WPObject *  pObj;

do
{
    // get arg1 - the object
    pArg = CALCRTNPTR( pHdr)->pnext;
    pObj = (WPObject*)pArg->value;

    // get arg2 - if no client window was specified,
    // use the Desktop folder window
    pArg = pArg->pnext;
    hClient = pArg->value;
    if (hClient == 0)
        hClient = _wpclsQueryActiveDesktopHWND( clsWPDesktop);

    // get arg3 - if it appears to contain a POINTL, use it;
    // otherwise, use the current mouse position
    pArg = pArg->pnext;
    if (pArg->value && GIVEP( pArg->type) == GIVE_PBUF &&
        pArg->cbgive == sizeof( POINTL))
        ptl = *(PPOINTL)pArg->value;
    else
    {
        WinQueryPointerPos( HWND_DESKTOP, &ptl);
        WinMapWindowPoints( HWND_DESKTOP, hClient, &ptl, 1);
    }

    // adjust the position so the menu doesn't appear
    // an icon-width away from the pointer
    ptl.x -= WinQuerySysValue( HWND_DESKTOP, SV_CXICON);

    // create a window that will own the menu;  it will
    // self-destruct after the menu is dismissed
    hHost = WinCreateWindow( HWND_OBJECT, WC_STATIC, "",
                             0, 0, 0, 0, 0, 0, HWND_BOTTOM,
                             0x9876, 0, 0);
    if (hHost == 0)
        break;

    // put pObj in the window's QWL_USER so it can determine
    // which object should get the menu's commands
    WinSetWindowULong( hHost, QWL_USER, (ULONG)pObj);

    // throw away the original wndproc because the subproc will
    // handle all msgs;  if subclassing fails, destroy the window
    if (WinSubclassWindow( hHost, &MenuOwnerWndProc) == FALSE)
    {
        WinDestroyWindow( hHost);
        break;
    }

    Thread1AddSem( hHost);

    // set pArg to arg0 (i.e. the return value), then display the
    // object's popup menu (the only type that really works)
    pArg = CALCRTNPTR( pHdr);
    pArg->value = _wpDisplayMenu( pObj, hHost, hClient,
                                  &ptl, MENU_OBJECTPOPUP, 0);
    if (pArg->value)
        rc = 0;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// used the by window that Thread1DisplayPopup() creates;  this window
// owns the menu that was created and receives msgs from it;  for most
// menu commands, it invokes a method on the object stored in QWL_USER;
// for others, it forwards the msg to the Desktop folder window;  after
// the menu is dismissed, it posts a msg to itself to destroy this window

MRESULT _System MenuOwnerWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)

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
            somPrintf( RWSSRVNAME ": MenuOwnerWndProc - Access violation handled\n");
            break;
        }

        if (DosSetExceptionHandler( &rwsregrec.regrec))
        {
            somPrintf( RWSSRVNAME ": MenuOwnerWndProc - DosSetExceptionHandler failed\n");
            break;
        }
    }

    switch (msg)
    {
      case WM_COMMAND:
      case WM_SYSCOMMAND:
      {
        WPObject *      pObj = (WPObject*)WinQueryWindowPtr( hwnd, QWL_USER);
        HWND            hDesk = _wpclsQueryActiveDesktopHWND( clsWPDesktop);

        // if the target object is null or invalid, exit
        if (pObj == NULL || somIsObj( pObj) == FALSE)
            break;

        // some commands are actually handled by the Desktop folder
        // window and not the target object;  for these, forward the msg
        if (SHORT1FROMMP( mp1) == WPMENUID_CREATEANOTHER ||
            SHORT1FROMMP( mp1) == WPMENUID_MOVE ||
            SHORT1FROMMP( mp1) == WPMENUID_COPY ||
            SHORT1FROMMP( mp1) == WPMENUID_CREATESHADOW ||
            (SHORT1FROMMP( mp1) >= 0x7d0 && SHORT1FROMMP( mp1) <= 0x7df))
        {
            if (hDesk)
                WinPostMsg( hDesk, msg, mp1, mp2);
            break;
        }

        // move the focus to the Desktop folder window to ensure
        // that a newly-opened folder window will inherit the focus,
        // then invoke the current object's method
        if (hDesk)
            WinFocusChange( HWND_DESKTOP, hDesk, 0);
        _wpMenuItemSelected( pObj, hwnd, SHORT1FROMMP( mp1));

        break;
      }

      // handling is similar to WM_COMMAND, just simpler
      case WM_HELP:
      {
        WPObject *      pObj = (WPObject*)WinQueryWindowPtr( hwnd, QWL_USER);

        // ensure the target object is valid
        if (pObj && somIsObj( pObj))
            _wpMenuItemHelpSelected( pObj, SHORT1FROMMP( mp1));

        break;
      }

      case WM_MENUEND:
        if ((HWND)WinQueryWindow( (HWND)mp2, QW_OWNER) == hwnd)
            WinPostMsg( hwnd, WM_USER+1, 0, 0);
        break;

      case WM_USER+1:
        WinDestroyWindow( hwnd);
        Thread1CloseSem( hwnd);
        break;

      default:
        // all msgs other than those above are handled here;
        // consequently, this window no longer behaves like a WC_STATIC
        mRtn = WinDefWindowProc( hwnd, msg, mp1, mp2);
        break;
    }

} while (fFalse);

    // suppress the spurious error if a longjmp occured
    if (fUseHandler && DosUnsetExceptionHandler( &rwsregrec.regrec) && !fJump)
        somPrintf( RWSSRVNAME ": MenuOwnerWndProc - DosUnsetExceptionHandler failed\n");

    return (mRtn);
}

/****************************************************************************/
/****************************************************************************/

// Handles RWSCMD_OPEN on the WPS's UI thread

ULONG           Thread1Open( PRWSHDR pHdr)

{
    ULONG       rc = 0;
    PRWSDSC     pRtn;
    PRWSDSC     pArg;
    WPObject *  pObj;

    // arg0 - the return value
    pRtn = CALCRTNPTR( pHdr);

    // arg1 - the object
    pArg = pRtn->pnext;
    pObj = (WPObject*)pArg->value;

    // arg2 - the view;  arg3 - the open new window flag
    pArg = pArg->pnext;

    // open the object
    pRtn->value = OpenObject( pObj, pArg->value, pArg->pnext->value);
    if (pRtn->value == 0)
        rc = RWSSRV_CMDFAILED;

    return (rc);
}

/****************************************************************************/

// opens objects

HWND            OpenObject( WPObject * pObj, ULONG ulView, BOOL fNew)

{
    HWND        hwnd = 0;
    ULONG       ulAltView = 0;
    PVIEWITEM   pvi = 0;

    // determine the correct view if none was specified
    if (ulView == OPEN_DEFAULT)
    {
        ulView = _wpQueryDefaultView( pObj);

        // for drive objects, change OPEN_AUTO to OPEN_TREE;
        // if a datafile's default is "open associated program"
        // or "open program added to file's menu", look for
        // OPEN_RUNNING also

        if (ulView == OPEN_AUTO)
            ulView = OPEN_TREE;
        else
        if ((ulView >= 4096 && ulView < 4104) ||
            (ulView >= 5000 && ulView < 5008))
            ulAltView = OPEN_RUNNING;
    }

    // unless a new window was specified, look for an existing one
    if (fNew == FALSE)
    {
        while ((pvi = _wpFindViewItem( pObj, VIEW_ANY, pvi)) != NULL) {
            if (pvi->view == ulView || pvi->view == ulAltView)
            {
                hwnd = pvi->handle;
                break;
            }
        }
    }

    // open or switch to a window
    if (hwnd)
        _wpSwitchTo( pObj, ulView);
    else
        hwnd = _wpOpen( pObj, NULLHANDLE, ulView, 0);

    return (hwnd);
}

/****************************************************************************/
/****************************************************************************/

// Handles RWSCMD_LOCATE on the WPS's UI thread;  it determines the correct
// folder & view to open for the target object, then opens the folder and
// gets the object selected

ULONG           Thread1Locate( PRWSHDR pHdr)

{
    ULONG       rc = RWSSRV_CMDFAILED;
    ULONG       ulView;
    PRWSDSC     pRtn;
    WPObject *  pObj;
    WPObject *  pOpen;
    SOMClass *  anyClass;

do
{
    // arg0 - the return value
    pRtn = CALCRTNPTR( pHdr);

    // arg1 - the object
    pObj = (WPObject*)pRtn->pnext->value;

    // get the folder the object is in
    pOpen = _wpQueryFolder( pObj);
    if (pOpen == NULL)
        break;

    // if this is a root folder, convert to a drive object
    anyClass = ClassFromName( "WPRootFolder");
    if (anyClass && _somIsA( pOpen, anyClass))
    {
        pOpen = _wpQueryDisk( pOpen);
        if (pOpen == NULL)
            break;
    }

    // get the default view;  if it's not icon, change to details
    ulView = _wpQueryDefaultView( pOpen);
    if (ulView != OPEN_CONTENTS)
        ulView = OPEN_DETAILS;

    // open the folder - exit if this fails
    pRtn->value = OpenObject( pOpen, ulView, FALSE);
    if (pRtn->value == 0)
        break;

    // try to select the object;  any failures will be ignored
    TimedSelect( pObj, pRtn->value);

    // report success
    rc = 0;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// called by Thread1Locate() to select an object;  if the folder was
// just opened, it may not be populated yet, so this will fail;  if so,
// it creates a window and a timer to keep retrying

void            TimedSelect( WPObject * pObj, HWND hFrame)

{
    BOOL        fRtn = FALSE;
    HWND        hLocate = 0;
    HWND        hCnr;
    ULONG       ctr;

do
{
    // get the container window
    hCnr = WinWindowFromID( hFrame, FID_CLIENT);
    if (hCnr == 0)
        break;

    // if we can select the object now, we're done
    fRtn = SelectObject( pObj, hCnr);
    if (fRtn)
        break;

    // if not, see if there's a slot in the locate table to store the
    // info for later use (it's unlikely we'd have 16 locates active)
    for (ctr=0; ctr < LOCATEOBJ_CNT; ctr++)
        if (aLocateObj[ctr].pObj == 0)
        {
            aLocateObj[ctr].pObj = pObj;
            aLocateObj[ctr].hCnr = hCnr;
            aLocateObj[ctr].ctr  = LOCATEOBJ_TRIES;
            break;
        }

    // no room at the inn?  oh well, forget it...
    if (ctr >= LOCATEOBJ_CNT)
        break;

    // create a window to receive timer msgs
    hLocate = WinCreateWindow( HWND_OBJECT, WC_STATIC, "",
                               0, 0, 0, 0, 0, 0, HWND_BOTTOM, 0, 0, 0);
    if (hLocate == 0)
        break;

    // put a ptr to the locate table entry in the window's QWL_USER
    WinSetWindowULong( hLocate, QWL_USER, (ULONG)&aLocateObj[ctr]);

    // throw away the original wndproc because the subproc
    // will handle all msgs
    if (WinSubclassWindow( hLocate, &LocateObjWndProc) == FALSE)
        break;

    Thread1AddSem( hLocate);

    // start a 500ms timer
    if (WinStartTimer( WinQueryAnchorBlock( hLocate), hLocate,
                       LOCATEOBJ_TIMER, 500) != LOCATEOBJ_TIMER)
    {
        Thread1CloseSem( hLocate);
        break;
    }

    fRtn = TRUE;

} while (fFalse);

    // if we failed after creating the time window, destroy it
    if (fRtn == FALSE && hLocate)
        WinDestroyWindow( hLocate);

    return;
}

/****************************************************************************/

// if the object can be found in target window, this function selects it
// and scrolls it into view if needed

BOOL            SelectObject( WPObject * pObj, HWND hCnr)

{
    BOOL            fRtn = FALSE;
    PUSEITEM        pu = NULL;
    PRECORDITEM     pr;
    PMINIRECORDCORE pm;
    RECTL           rclView;
    RECTL           rclRec;
    LONG            x;
    LONG            y;
    QUERYRECORDRECT qrr;

do
{
    // see if the object has been inserted into the target container yet
    while ((pu = _wpFindUseItem( pObj, USAGE_RECORD, pu)) != NULL)
    {
        pr = (PRECORDITEM)&pu[1];
        if (pr->hwndCnr == hCnr)
        {
            fRtn = TRUE;
            break;
        }
    }

    // if not, exit - we'll try again later;  if there are any
    // failures beyond this point, we'll abort & won't try again
    if (fRtn == FALSE)
        break;

    // deselect all of the records
    pm = (PMINIRECORDCORE)CMA_FIRST;
    while ((pm = (PMINIRECORDCORE)WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS,
                                              pm, (MP)CRA_SELECTED)) != NULL)
    {
        if (pm == (PMINIRECORDCORE)-1)
            break;

        WinSendMsg( hCnr, CM_SETRECORDEMPHASIS,
                    pm, MPFROM2SHORT( FALSE, CRA_SELECTED));
    }

    // set up to get the record's rectangle
    qrr.cb = sizeof(QUERYRECORDRECT);
    qrr.pRecord = (PRECORDCORE)pr->pRecord;
    qrr.fRightSplitWindow = FALSE;
    qrr.fsExtent = (CMA_ICON | CMA_TEXT);

    // select our record, see if the window has been scrolled,
    // and get the record's rectangle - if any of this fails, abort
    if (!WinSendMsg( hCnr, CM_SETRECORDEMPHASIS, pr->pRecord,
                     MPFROM2SHORT( TRUE, (CRA_SELECTED | CRA_CURSORED))) ||
        !WinSendMsg( hCnr, CM_QUERYVIEWPORTRECT, &rclView,
                     MPFROM2SHORT( CMA_WINDOW, FALSE)) ||
        !WinSendMsg( hCnr, CM_QUERYRECORDRECT, &rclRec, &qrr))
        break;

    x = 0;
    y = 0;

    // determine if our record is in view
    if (rclRec.xLeft < rclView.xLeft)
        x += rclRec.xLeft - rclView.xLeft;
    if (rclRec.yBottom < rclView.yBottom)
        y += rclView.yBottom - rclRec.yBottom;

    if (rclRec.xRight >= rclView.xRight)
        x += rclRec.xRight - rclView.xRight;
    if (rclRec.yTop >= rclView.yTop)
        y += rclView.yTop - rclRec.yTop;

    // if not, scroll it into view
    if (x)
        WinSendMsg( hCnr, CM_SCROLLWINDOW, (MP)CMA_HORIZONTAL, (MP)x);
    if (y)
        WinSendMsg( hCnr, CM_SCROLLWINDOW, (MP)CMA_VERTICAL, (MP)y);

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

// used by the window that TimedSelect() creates;  it receives timer msgs
// and attempts to select an object;  after LOCATEOBJ_TRIES (i.e. 20),
// it stops the timer & destroys itself

MRESULT _System LocateObjWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)

{
    BOOL                fJump = FALSE;
    PLOCATEOBJ          pLoc;
    RWSXCPTREGREC       rwsregrec;

    // we only handle our timer msg
    if (msg != WM_TIMER || (USHORT)mp1 != LOCATEOBJ_TIMER)
        return (WinDefWindowProc( hwnd, msg, mp1, mp2));

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
            somPrintf( RWSSRVNAME ": LocateObjWndProc - Access violation handled\n");
            break;
        }

        if (DosSetExceptionHandler( &rwsregrec.regrec))
        {
            somPrintf( RWSSRVNAME ": LocateObjWndProc - DosSetExceptionHandler failed\n");
            break;
        }
    }

    // get the pointer to our entry in aLocateObj
    pLoc = (PLOCATEOBJ)WinQueryWindowPtr( hwnd, QWL_USER);

    // if we couldn't select the object but still have some tries
    // left, dec the ctr;  otherwise, cleanup & destroy this window
    if (SelectObject( pLoc->pObj, pLoc->hCnr) == FALSE && pLoc->ctr)
        pLoc->ctr--;
    else
    {
        WinStopTimer( WinQueryAnchorBlock( hwnd), hwnd, LOCATEOBJ_TIMER);
        WinDestroyWindow( hwnd);
        pLoc->pObj = 0;
        pLoc->hCnr = 0;
        Thread1CloseSem( hwnd);
    }

} while (fFalse);

    // suppress the spurious error if a longjmp occured
    if (fUseHandler && DosUnsetExceptionHandler( &rwsregrec.regrec) && !fJump)
        somPrintf( RWSSRVNAME ": LocateObjWndProc - DosUnsetExceptionHandler failed\n");

    return (0);
}

/****************************************************************************/
/****************************************************************************/

// this function implements RWSCMD_LISTOBJECTS and runs on RwsServer's
// thread;  it obtains a list of objects in a folder, then replaces
// each object pointer with the object's handle

ULONG           ListObjects( PRWSHDR pHdr)

{
    ULONG       rc = 0;
    ULONG       cntObj;
    ULONG       flags;
    ULONG       ul;
    ULONG       ctr;
    HFIND       hFind = 0;
    PULONG      pulBuf;
    PRWSDSC     pArg;
    WPFolder*   pFolder;
    WPObject*   pObj;
    M_WPObject* clsList[3];

do
{
    // get arg1 - the object
    pArg = CALCRTNPTR( pHdr)->pnext;
    pFolder = (WPFolder*)pArg->value;

    // get arg2 - pointer to buffer
    pArg = pArg->pnext;
    pulBuf = (PULONG)pArg->value;
    cntObj = (ULONG)(pArg->cbgive/sizeof(ULONG));

    // get arg3 - flags
    pArg = pArg->pnext;
    flags = *(PULONG)pArg->value & LISTOBJ_MASK;

    // identify the classes we want to find:  Abstract & Transient objects,
    // shadows of filesystem objects, or all objects (NOT recommended
    // because it creates large numbers of otherwise useless file handles -
    // call DosFindFirst(), etc, then use the f/q paths of the files found)
    if (flags == LISTOBJ_FILESHADOWS)
    {
        clsList[0] = clsWPShadow;
        clsList[1] = 0;
    }
    else
    if (flags == LISTOBJ_ALL)
    {
        clsList[0] = 0;
    }
    else
    {
        clsList[0] = clsWPAbstract;
        clsList[1] = clsWPTransient;
        clsList[2] = 0;
    }

    // find some objects
    ul = _wpclsFindObjectFirst( clsWPAbstract, clsList, &hFind, 0, pFolder,
                                FALSE, 0, (WPObject**)pulBuf, &cntObj);

    // if the return is FALSE, see why;  if there were too many objects,
    // signal that but continue; if there were no objects, exit without
    // an errorcode;  otherwise, exit with an errorcode
    if (ul == FALSE)
    {
        ul = _wpclsQueryError( clsWPAbstract);
        if (ul == WPERR_BUFFER_OVERFLOW)
            *(PULONG)pArg->value |= LISTOBJ_OVERFLOW;
        else
        {
            cntObj = 0;
            if (ul != WPERR_OBJECT_NOT_FOUND)
                rc = RWSSRV_CMDFAILED;
            break;
        }
    }

    // cycle thru the returned object pointers;  if they meet the criteria,
    // (if any) replace the pointer with the object's handle;  since
    // wpFindObjectFirst locks each object, we have to unlock it so it
    // eventually goes dormant

    // if we're looking for shadows of files & folders, get the original
    // object & see if it's derived from the FileSystem class;  if so,
    // replace the shadow's pointer with the original's handle
    if (flags)
        for (ctr=0, ul=0; ctr < cntObj; ctr++)
        {
            pObj = _wpQueryShadowedObject( (WPObject*)pulBuf[ctr], FALSE);
            _wpUnlockObject( (WPObject*)pulBuf[ctr]);

            if (pObj && _somIsA( pObj, clsWPFileSystem))
            {
                pulBuf[ul] = _wpQueryHandle( pObj);
                if (pulBuf[ul])
                    ul++;
            }
        }

    // if we want all non-file objects, just get each one's handle
    else
        for (ctr=0, ul=0; ctr < cntObj; ctr++)
        {
            pObj = (WPObject*)pulBuf[ctr];
            pulBuf[ul] = _wpQueryHandle( pObj);
            if (pulBuf[ul])
                ul++;
            _wpUnlockObject( pObj);
        }

    if (ul < cntObj)
    {
        pulBuf[ul] = 0;
        cntObj = ul;
    }

} while (fFalse);

    // free the find handle, if any
    if (hFind)
        _wpclsFindObjectEnd( clsWPAbstract, hFind);

    // get arg0 (the return value);  provide nbr of objects in buffer
    pArg = CALCRTNPTR( pHdr);
    pArg->value = cntObj;

    return (rc);
}

/****************************************************************************/
/****************************************************************************/

// Opens an object (usually a file) using another object (usually a program).
// The action is identical to dropping the source object on a target object.

ULONG           Thread1OpenUsing( PRWSHDR pHdr)

{
    ULONG       rc = 0;
    PRWSDSC     pRtn;
    PRWSDSC     pArg;

    // arg0 - the return value
    pRtn = CALCRTNPTR( pHdr);

    // arg1 - the object
    pArg = pRtn->pnext;

    pRtn->value = _wpDroppedOnObject( (WPObject*)pArg->value,
                                      (WPObject*)pArg->pnext->value);
    if (pRtn->value == 0)
        rc = RWSSRV_CMDFAILED;

    return (rc);
}

/****************************************************************************/
/****************************************************************************/

