/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// ICNZMAIN.C
// Remote Workplace Server - demo program "Iconomize"

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
 * The Original Code is Remote Workplace Server - "Iconomize" demo program.
 *
 * The Initial Developer of the Original Code is Richard L. Walsh.
 * 
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

/****************************************************************************/

// Iconomize demonstrates most of the features of Remote Workplace Server.
// The program lists all Abstract objects which have a custom icon assigned
// and displays that icon along with the object's default icon and other
// info about the object.  In many cases, the custom icon is identical to
// the default.  Using Iconomize to delete these duplicates can reduce the
// size of os2.ini by 50-100k.

/****************************************************************************/

#include "ICNZ.H"

/****************************************************************************/

void            Morph( void);
MRESULT _System MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void            InitDlg( HWND hwnd);
void            InitGraphics( HWND hwnd);
void            InitMenus( HWND hwnd);
ULONG           InitCnr( void);
MRESULT         FormatFrame( HWND hwnd, MPARAM mp1, MPARAM mp2);
void            UpdateStatus( char * pszMsg);
ULONG           GetData( void);
MRESULT _System WaitWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
ULONG           GetList( void);
ULONG           GetListItems( PRWSBLD pBld, ULONG cnt,
                              PMINIRECORDCORE * ppRec, char ** ppBuf);
ULONG           GetDefaultIcon( ULONG ulObj, PULONG puRec);
void            EmptyCnr( void);

/****************************************************************************/

extern COLINFO      ci[];
extern ULONG        ulSortCol;

/****************************************************************************/

char        szCopyright[] = "Iconomize v" RWSVERSION " - (C)Copyright 2004  R.L.Walsh";
BOOL        fFalse = FALSE;
BOOL        fRestoreDefaults = FALSE;
HWND        hDlg = 0;
HWND        hCnr = 0;
HWND        hStatus = 0;
HWND        hmnuMain = 0;
HWND        hmnuSort = 0;
HWND        hmnuSingle = 0;
HWND        hmnuMultiple = 0;
HPOINTER    hErrIcon = 0;
HPOINTER    hSizePtr = 0;
HPOINTER    hSortPtr = 0;
HBITMAP     hUpBmp = 0;
HBITMAP     hDownBmp = 0;
PMINIRECORDCORE precMenu = 0;
ULONG       ulTotCnt = 0;
ULONG       ulTotSize = 0;
ULONG       ulDelCnt = 0;
ULONG       ulDelSize = 0;
char *      pStrings = 0;

// used with wpSetIconInfo() to reset an object's icon to its default
ICONINFO    iiClear = { sizeof(ICONINFO), ICON_CLEAR, 0, 0, 0, 0, 0 };

char        szStatus[] =
    "   Total: %-4d  size: %-6d     Deleted: %-4d  size: %-6d        ";

char        szError[64];

/****************************************************************************/
/****************************************************************************/

// I hope this doesn't need any explanation

int         main( void)

{
    HAB     hab = 0;
    HMQ     hmq = 0;
    HWND    hwnd = 0;
    int     nRtn = 8;
    QMSG    qmsg;

do
{
    Morph();

    hab = WinInitialize( 0);
    if (!hab)
        break;

    hmq = WinCreateMsgQueue( hab, 0);
    if (!hmq)
        break;

    // avoid compatibility problems
    // RWSFULLVERSION for Iconomize v0.80 is 0x08000100
    if (RwsQueryVersion( 0) < RWSFULLVERSION) {
        WinMessageBox( HWND_DESKTOP, 0,
                       "Please use a newer version of the RWS08 dlls",
                       "FPos", 1, MB_OK | MB_ERROR | MB_MOVEABLE);
        break;
    }

    hwnd = WinLoadDlg(
                HWND_DESKTOP,               //  parent-window
                NULLHANDLE,                 //  owner-window
                MainWndProc,                //  dialog proc
                NULLHANDLE,                 //  EXE module handle
                IDD_MAIN,                   //  dialog id
                NULL);                      //  pointer to create params

    if (!hwnd)
        break;

    while (WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0))
        WinDispatchMsg( hab, &qmsg);

    nRtn = 0;

} while (fFalse);

    if (nRtn)
        DosBeep( 440, 60);

    if (hwnd)
        WinDestroyWindow( hwnd);
    if (hmq)
        WinDestroyMsgQueue( hmq);
    if (hab)
        WinTerminate( hab);

    return (nRtn);
}

/****************************************************************************/

// enable this program to use PM functions
// even if it was linked or started as a VIO app

void        Morph( void)

{
    PPIB    ppib;
    PTIB    ptib;

    DosGetInfoBlocks( &ptib, &ppib);
    ppib->pib_ultype = 3;

    return;
}

/****************************************************************************/
/****************************************************************************/

// just your basic dialog procedure

MRESULT _System MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    switch (msg)
    {
        case WM_COMMAND:
            Command( hwnd, SHORT1FROMMP( mp1));
            return (0);

        case WM_CONTROL:
            if (mp1 == MPFROM2SHORT( FID_CLIENT, CN_CONTEXTMENU))
                PopupMenu( hwnd, (PMINIRECORDCORE)mp2);
            else
            if (mp1 == MPFROM2SHORT( FID_CLIENT, CN_ENTER))
                if (((PNOTIFYRECORDENTER)mp2)->pRecord)
                    Command( hwnd, IDM_DELETE);
            return (0);

        case WM_CHAR:
            if ((CHARMSG(&msg)->fs & (USHORT)KC_KEYUP) == 0 &&
                (CHARMSG(&msg)->fs & (USHORT)KC_VIRTUALKEY) &&
                 CHARMSG(&msg)->vkey == VK_DELETE)
            {
                Command( hwnd, IDM_DELETE);
                return (0);
            }
            break;

        case WM_SYSCOMMAND:
            if (SHORT1FROMMP( mp1) != SC_CLOSE)
                break;

            if (SaveChanges( "Exit", TRUE))
                WinPostMsg( hwnd, WM_QUIT, NULL, NULL);
            return (0);

        case WM_CLOSE:
            if (SaveChanges( "Exit", TRUE))
                WinPostMsg( hwnd, WM_QUIT, NULL, NULL);
            return (0);

        case WM_INITDLG:
            InitDlg( hwnd);
            return (0);

        case WM_INITMENU:
            if (mp1 == (MP)IDM_SINGLE || mp1 == (MP)IDM_MULTIPLE)
            {
                WinSendMsg( hmnuMain, MM_ENDMENUMODE, (MP)TRUE, 0);
                WinSetOwner( hmnuSort, (HWND)mp2);
            }
            else
            if (mp1 == (MP)IDM_FILE)
                WinSendMsg( (HWND)mp2, MM_SETITEMATTR,
                    MPFROM2SHORT( IDM_SAVE, FALSE),
                    MPFROM2SHORT( MIA_DISABLED, (ulDelCnt ? 0 : MIA_DISABLED)));

            return (0);

        case WM_MENUSELECT:
            if ((HWND)mp2 != hmnuMain)
                break;

            WinSendMsg( hmnuSingle, MM_ENDMENUMODE, (MP)TRUE, 0);
            WinSendMsg( hmnuMultiple, MM_ENDMENUMODE, (MP)TRUE, 0);
            WinSetOwner( hmnuSort, (HWND)mp2);
            return (0);

        case WM_MENUEND:
            if (mp1 == (MP)IDM_SINGLE && precMenu)
                WinSendMsg( hCnr, CM_SETRECORDEMPHASIS, (MP)precMenu,
                            MPFROM2SHORT( FALSE, CRA_SOURCE));
            return (0);

        case WM_QUERYFRAMECTLCOUNT:
            return ((MRESULT)(SHORT1FROMMR(
                              WinDefDlgProc( hwnd, msg, mp1, mp2)) + 1));

        case WM_FORMATFRAME:
            return (FormatFrame( hwnd, mp1, mp2));

        case WM_SAVEAPPLICATION:
            if (fRestoreDefaults)
                PrfWriteProfileData( HINI_USERPROFILE, "ICONOMIZE", 0, 0, 0);
            else
            {
                StoreSort();
                WinStoreWindowPos( "ICONOMIZE", "WNDPOS", hwnd);
            }
            break;

        case WM_DESTROY:
            EmptyCnr();
            RwsClientTerminate();
            break;

    } //end switch (msg)

    return (WinDefDlgProc( hwnd, msg, mp1, mp2));
}

/****************************************************************************/
/****************************************************************************/

void            InitDlg( HWND hwnd)

{
    ULONG       rc;
    char *      pErr;
    char        szText[128];

do
{
    // since there's only one of each of these windows, it's easier to
    // put these in globals than to keep calling WinWindowFromID(), etc
    rc = 1;
    hDlg = hwnd;
    hCnr = WinWindowFromID( hwnd, FID_CLIENT);
    hStatus = WinWindowFromID( hwnd, IDC_STATUS);
    if (hCnr == 0 || hStatus == 0)
        ERRMSG( "Unable to locate window")

    // get icons, pointers, and menu checkmarks
    InitGraphics( hwnd);

    // load the menubar & popup menus, then reformat
    // the frame to handle the menubar & status bar
    InitMenus( hwnd);

    // init sort settings
    RestoreSort();

    // init RWS for the current process;
    // this will register the RWS class if needed;
    rc = RwsClientInit( TRUE);
    if (rc)
        ERRMSG( "Unable to initialize RwsClient")

    // the WPS timesout after 30 seconds when trying to
    // access a network drive that isn't currently mapped
    RwsSetTimeout( 35);

    // setup the container & its columns
    rc = InitCnr();
    if (rc)
        ERRMSG( "Unable to initialize window")

    // wait until main() has entered its msg loop before getting any data
    WinPostMsg( hwnd, WM_COMMAND, (MP)IDM_GETDATA, 0);

    WinRestoreWindowPos( "ICONOMIZE", "WNDPOS", hwnd);

} while (fFalse);

    // if there was an error, abort
    if (rc)
    {
        sprintf( szText, "Error:  %s - rc= %d\nPress 'OK' to end this program",
                 pErr, rc);
        WinMessageBox( HWND_DESKTOP, 0, szText, "Iconomize", 1,
                       MB_OK | MB_ERROR | MB_MOVEABLE);
        WinPostMsg( hwnd, WM_QUIT, NULL, NULL);
    }

    return;
}

/****************************************************************************/

void        InitGraphics( HWND hwnd)

{
    HPS         hps;
    HMODULE     hmod;
    HPOINTER    hico;

    // get & set the app icon
    hico = WinLoadPointer( HWND_DESKTOP, 0, IDI_APPICO);
    WinSendMsg( hwnd, WM_SETICON, (MP)hico, 0);

    // get the error icon
    hErrIcon = WinLoadPointer( HWND_DESKTOP, 0, IDI_ERRICO);

    // get the sort pointer
    hSortPtr = WinLoadPointer( HWND_DESKTOP, 0, IDI_SORTPTR);
    if (hSortPtr == 0)
        hSortPtr = WinQuerySysPointer( HWND_DESKTOP, SPTR_ARROW, FALSE);

    // get the sizing pointer that the container control uses
    if (DosQueryModuleHandle( "PMCTLS", &hmod) ||
        (hSizePtr = WinLoadPointer( HWND_DESKTOP, hmod, 100)) == 0)
        hSizePtr = WinQuerySysPointer( HWND_DESKTOP, SPTR_SIZEWE, FALSE);

    // load the sort menu's custom checkmarks
    hps = WinGetPS( hwnd);
    if (hps)
    {
        hUpBmp = GpiLoadBitmap( hps, 0, IDB_UPBMP, 0, 0);
        hDownBmp = GpiLoadBitmap( hps, 0, IDB_DOWNBMP, 0, 0);
        WinReleasePS( hps);
    }
    // if that failed, use the standard checkmark
    if (hUpBmp == 0)
        hUpBmp = WinGetSysBitmap( HWND_DESKTOP, SBMP_MENUCHECK);
    if (hDownBmp == 0)
        hDownBmp = WinGetSysBitmap( HWND_DESKTOP, SBMP_MENUCHECK);

    return;
}

/****************************************************************************/

void            InitMenus( HWND hwnd)

{
    MENUITEM    mi;

    // load the menubar - its ownerdraw Sort submenu
    // will be shared with the popup menus
    hmnuMain = WinLoadMenu( hwnd, 0, IDM_MAIN);
    WinSendMsg( hmnuMain, MM_QUERYITEM, MPFROM2SHORT( IDM_SORT, FALSE), &mi);
    hmnuSort = mi.hwndSubMenu;

    // for each of the popup menus, restore its original ID,
    // then change each one's Sort menuitem into a submenu

    hmnuSingle = WinLoadMenu( HWND_OBJECT, 0, IDM_SINGLE);
    WinSetWindowUShort( hmnuSingle, QWS_ID, IDM_SINGLE);
    WinSendMsg( hmnuSingle, MM_QUERYITEM, MPFROM2SHORT( IDM_SORT, FALSE), &mi);
    mi.afStyle = MIS_SUBMENU;
    mi.hwndSubMenu = hmnuSort;
    WinSendMsg( hmnuSingle, MM_SETITEM, 0, &mi);

    hmnuMultiple = WinLoadMenu( HWND_OBJECT, 0, IDM_MULTIPLE);
    WinSetWindowUShort( hmnuMultiple, QWS_ID, IDM_MULTIPLE);
    WinSendMsg( hmnuMultiple, MM_QUERYITEM, MPFROM2SHORT( IDM_SORT, FALSE), &mi);
    mi.afStyle = MIS_SUBMENU;
    mi.hwndSubMenu = hmnuSort;
    WinSendMsg( hmnuMultiple, MM_SETITEM, 0, &mi);

    // since the above changed the ownership of the sort menu,
    // restore ownership to the menubar
    WinSetOwner( hmnuSort, hmnuMain);

    // finally, make the frame aware of the menubar;
    // the WM_FORMATFRAME msg this generates will let us
    // tell the frame about the status window as well
    WinSendMsg( hwnd, WM_UPDATEFRAME, (MP)FCF_MENU, 0);

    return;
}

/****************************************************************************/

ULONG       InitCnr( void)

{
    ULONG           rc = 0;
    ULONG           ctr;
    PFIELDINFO      pfi;
    PFIELDINFO      ptr;
    FIELDINFOINSERT fii;
    CNRINFO         cnri;

do
{
    // allocate FIELDINFO structs for each column
    pfi = WinSendMsg( hCnr, CM_ALLOCDETAILFIELDINFO, (MP)eCNTCOLS, 0);
    if (pfi == 0)
        ERRNBR( 1)

    // initialize those structs from our column-info table;
    // each column will get one DWORD at the end of the
    // MINIRECORDCORE struct to store its data
    for (ptr=pfi, ctr=0; ctr < eCNTCOLS; ctr++, ptr=ptr->pNextFieldInfo)
    {
        ci[ctr].pfi = ptr;
        ptr->flData = ci[ctr].flData;
        ptr->flTitle = (ci[ctr].flData & CFA_ALIGNMASK);
        ptr->pTitleData = ci[ctr].pszTitle;
        ptr->offStruct = sizeof(MINIRECORDCORE) + (ctr * sizeof(ULONG));
    }

    // insert the column info into the container
    fii.cb = sizeof(FIELDINFOINSERT);
    fii.pFieldInfoOrder = (PFIELDINFO)CMA_FIRST;
    fii.fInvalidateFieldInfo = FALSE;
    fii.cFieldInfoInsert = eCNTCOLS;
    if (WinSendMsg( hCnr, CM_INSERTDETAILFIELDINFO, (MP)pfi, (MP)&fii) == 0)
        ERRNBR( 2)

    // configure the container for Details view
    cnri.cb = sizeof(CNRINFO);
    cnri.pFieldInfoLast = ci[LASTLEFTCOL].pfi;
    cnri.flWindowAttr = CV_DETAIL | CA_DRAWICON | CA_DETAILSVIEWTITLES;
    cnri.xVertSplitbar = 300;
    if (WinSendMsg( hCnr, CM_SETCNRINFO, (MP)&cnri, 
        (MP)(CMA_PFIELDINFOLAST | CMA_FLWINDOWATTR | CMA_XVERTSPLITBAR)) == 0)
        ERRNBR( 3)

    // update the container's layout
    WinSendMsg( hCnr, CM_INVALIDATEDETAILFIELDINFO, 0, 0);

    // subclass the two column heading windows
    SubclassColumnHeadings();

} while (fFalse);

    return (rc);
}

/****************************************************************************/
/****************************************************************************/

// handles the positioning of the menu & status bars

MRESULT     FormatFrame( HWND hwnd, MPARAM mp1, MPARAM mp2)

{
    USHORT  usClient = 0;
    USHORT  usCnt;
    PSWP    pSWP = (PSWP)mp1;
    RECTL   rcl;

    // have the dlg format the controls it knows about
    // (which by now includes the menubar we added)
    usCnt = SHORT1FROMMR(WinDefDlgProc( hwnd, WM_FORMATFRAME, mp1, mp2));

    // fill in the next SWP with some basic info
    pSWP[usCnt].hwnd = hStatus;
    pSWP[usCnt].fl = SWP_SIZE | SWP_MOVE | SWP_SHOW;

    // get the height of the status window
    WinQueryWindowRect( pSWP[usCnt].hwnd, &rcl);
    pSWP[usCnt].cy = rcl.yTop;

    // find the client window's SWP
    while (pSWP[usClient].hwnd != hCnr)
        usClient++;

    // position the status bar at the client's current origin
    // and make it as wide as the client
    pSWP[usCnt].x  = pSWP[usClient].x;
    pSWP[usCnt].y  = pSWP[usClient].y;
    pSWP[usCnt].cx = pSWP[usClient].cx;

    // reduce the client window's height & move its origin up
    // by the height of the status bar
    pSWP[usClient].cy -= pSWP[usCnt].cy;
    pSWP[usClient].y += pSWP[usCnt].cy;

    // return total count of frame controls
    return( MRFROMSHORT( usCnt + 1));
}

/****************************************************************************/

// show the counters and the optional extra message

void        UpdateStatus( char * pszMsg)

{
    int     cnt;
    char    szText[128];

    cnt = sprintf( szText, szStatus, ulTotCnt, ulTotSize, ulDelCnt, ulDelSize);
    if (pszMsg)
        strcpy( &szText[cnt], pszMsg);
    WinSetWindowText( hStatus, szText);
 
   return;
}

/****************************************************************************/
/****************************************************************************/

// called at startup and when Reload is selected from the menu,
// it performs high-level functions to fill the window with data

ULONG       GetData( void)

{
    ULONG           rc = 0;
    HWND            hWait;
    CNRINFO         cnri;
    char *          pErr = szCopyright;

do
{
    // show the "please wait" dialog
    hWait = WinLoadDlg( HWND_DESKTOP,       // parent-window
                        hDlg,               // owner-window
                        WaitWndProc,        // dialog proc
                        NULLHANDLE,         // EXE module handle
                        IDD_WAIT,           // dialog id
                        NULL);              // pointer to create params

    // init the counters
    ulTotCnt = 0;
    ulTotSize = 0;
    ulDelCnt = 0;
    ulDelSize = 0;

    // if the container already has data, delete the icons and
    // free the records, then free the buffer containing string data
    EmptyCnr();
    free( pStrings);

    // set the sort function to be used when inserting records
    cnri.cb = sizeof(CNRINFO);
    cnri.pSortRecord = ci[ulSortCol].pvSort;
    WinSendMsg( hCnr, CM_SETCNRINFO, (MP)&cnri, (MP)CMA_PSORTRECORD);

    // get the data
    rc = GetList();
    if (rc)
        ERRMSG( "Unable to list objects")

    // find out how wide each column is, then reposition the splitbar
    GetColumnWidths();
    ResetSplitBar();

} while (fFalse);

    // get rid of the wait dialog, then update the status bar
    WinDestroyWindow( hWait);
    UpdateStatus( pErr);

    return (rc);
}

/****************************************************************************/

// displayed while loading the icon list

MRESULT _System WaitWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    switch (msg)
    {
        case WM_COMMAND:
        case WM_CONTROL:
            return (0);

        case WM_SYSCOMMAND:
            if (SHORT1FROMMP( mp1) != SC_CLOSE)
                break;

            WinPostMsg( 0, WM_QUIT, 0, 0);
            return (0);

        case WM_CLOSE:
            WinPostMsg( 0, WM_QUIT, 0, 0);
            return (0);

    } //end switch (msg)

    return (WinDefDlgProc( hwnd, msg, mp1, mp2));
}

/****************************************************************************/

// The primary data acquisition function:  gets a list of the objects,
// allocates a corresponding number of container records, then indirectly
// dispatches requests for data to RwsServer in batches of 16 objects;
// when done, it inserts all records at once.  It demonstrates how to
// convert multiple objects with a minimum of code & overhead.

ULONG       GetList( void)

{
    ULONG           rc = 0;
    ULONG           ctr;
    ULONG           cnt;
    ULONG           size;
    PRWSARG         pArg;
    PRWSBLD         pBld = 0;
    char *          pMem = 0;
    char *          ptr;
    char *          pBuf;
    PMINIRECORDCORE pRec;
    PMINIRECORDCORE precFirst;
    RECORDINSERT    ri;

#define CNTOBJS     16
#define CNTARGS     (4*CNTOBJS)

do
{
    // see how large the list of objects is
    if (PrfQueryProfileSize( HINI_USERPROFILE, "PM_Abstract:Icons",
                             0, &size) == FALSE)
        ERRNBR( 1)

    // get some mem to hold the list
    pMem = (char*)malloc( size);
    if (pMem == 0)
        ERRNBR( 2)

    // get the list of objects
    if (PrfQueryProfileData( HINI_USERPROFILE, "PM_Abstract:Icons",
                             0, pMem, &size) == FALSE)
        ERRNBR( 3)

    // count the nbr of items
    for (ptr=pMem, cnt=0; *ptr; ptr=strchr( ptr, 0)+1)
        cnt++;

    // calc the space needed to hold each object's string data
    for (ctr=0, size=0; ctr < eCNTCOLS; ctr++)
        size += ci[ctr].ulExtra;

    // alloc the string buffer
    pStrings = (char*)malloc( cnt * size);
    if (pStrings == 0)
        ERRNBR( 4)

    // allocate a record for each item
    precFirst = (PMINIRECORDCORE)WinSendMsg( hCnr, CM_ALLOCRECORD,
                                    (MP)(eCNTCOLS*sizeof(ULONG)), (MP)cnt);
    if (precFirst == 0)
        ERRNBR( 5)

    // allocate space to hold the RWSBLD/RWSARG array;  we'll be converting
    // CNTOBJS objects at a time, each requiring 4 RWSARG structs
    size = sizeof( RWSBLD) + CNTARGS*sizeof( RWSARG);
    pBld = (PRWSBLD)malloc( size);
    if (pBld == 0)
        ERRNBR( 6)

    // clear the array and init the header
    memset( pBld, 0, size);
    pBld->callType = RWSP_CONV;                 // data conversion call
    pBld->rtnType = RWSR_ASIS;                  // returns the nbr of conversions

    // fill in the fields that will be constant;  note that the
    // first arg will not be converted - this is a quick & dirty
    // way to associate the key string with the converted data
    // and avoids having to coordinate two separate lists

    for (ctr=0, pArg=CALCARGPTR( pBld); ctr < CNTARGS; )
    {
        pArg[ctr++].type = RWSI_ASIS;           // no conversion
        pArg[ctr++].type = RWSC_OHNDL_OICON;    // obj handle to obj icon
        pArg[ctr].type = RWSC_PREV_OTITLE;      // previous arg's obj to title
        pArg[ctr++].size = ci[eTITLE].ulExtra;  // max title length
        pArg[ctr].type = RWSC_PREV_OPATH;       // previous arg's obj to path
        pArg[ctr++].size = ci[ePATH].ulExtra;   // max path length
    }

    // step thru the list of object handles;  a null string signals the end
    for (ptr=pMem, pBuf=pStrings, ctr=0, pArg=CALCARGPTR( pBld), pRec=precFirst;
                                                *ptr; ptr=strchr( ptr, 0)+1)
    {
        // 1st arg:  a ptr to the key string
        pArg[ctr++].value = (ULONG)ptr;

        // 2nd arg:  the key converted into an object handle
        pArg[ctr++].value = strtoul( ptr, 0, 16) + 0x20000;

        // 3rd & 4th args:  nothing needed - they'll reuse
        // the object pointer obtained for the 2nd arg
        ctr += 2;

        // perform the conversions & populate the records in batches
        if (ctr >= CNTARGS)
        {
            ERRRTN( GetListItems( pBld, ctr, &pRec, &pBuf))
            ctr = 0;
        }
    }

    if (rc)
        break;

    // handle the final batch of entries
    if (ctr)
        ERRRTN( GetListItems( pBld, ctr, &pRec, &pBuf))

    // init the recordinsert struct
    ri.cb = sizeof(RECORDINSERT);
    ri.pRecordOrder = (PRECORDCORE)CMA_FIRST;
    ri.pRecordParent = 0;
    ri.fInvalidateRecord = TRUE;
    ri.zOrder = CMA_TOP;
    ri.cRecordsInsert = cnt;

    // insert all the records at once
    if (WinSendMsg( hCnr, CM_INSERTRECORD, (MP)precFirst, (MP)&ri) == 0)
        ERRNBR( 7)

} while (fFalse);

    // free our memory
    free( pMem);
    free( pBld);

    printf( "GetList:  count= %d  size= %d\n", ulTotCnt, ulTotSize);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "GetList:  rc= %d %s\n", rc, szError);
    }

    return (rc);
}

/****************************************************************************/

// this takes a list of requests from the caller, dispatches it to RwsServer,
// then uses the returned data to populate a series of records

ULONG       GetListItems( PRWSBLD pBld, ULONG cnt,
                          PMINIRECORDCORE * ppRec, char ** ppBuf)

{
    ULONG       rc = 0;
    ULONG       ctr;
    PRWSDSC     pDsc;
    PRWSHDR     pHdr = 0;
    PMINIRECORDCORE pRec;
    char *      pBuf;

do
{
    // identify the number of args (extra RWSARG structs will be ignored)
    pBld->argCnt = cnt;

    // provide a place to store a ptr to the final results
    pBld->ppHdr = &pHdr;

    // have RwsClient construct a server request block then dispatch it
    ERRRTN( RwsCallIndirect( pBld))

    // point pDsc at arg0 (the return value), i.e. the first entry minus 1 
    ERRRTN( RwsGetArgPtr( pHdr, 0, &pDsc))

    // each record contains a MINIRECORDCORE struct followed by an
    // array of DWORDS (one for each column);  string data is stored
    // separately in pBuf

    for (ctr=0, pRec=*ppRec, pBuf=*ppBuf; ctr < cnt; ctr+=4, pRec=pRec->preccNextRecord)
    {
        PULONG      puRec = (PULONG)&pRec[1];
        char *      pText;

        ++ulTotCnt;

        // point at the first of the 4 entries for this key;  the value
        // contains a pointer to the key string (this wasn't touched by RWS)
        pDsc = pDsc->pnext;

        // key
        puRec[eKEY] = (ULONG)pBuf;
        strcpy( pBuf, (char*)pDsc->value);
        pBuf = strchr( pBuf, 0) + 1;

        // next entry:  custom icon
        pDsc = pDsc->pnext;
        if (pDsc->rc == 0)
            puRec[eCUST] = *((PULONG)pDsc->pget);
        else
            puRec[eCUST] = hErrIcon;

        // default icon & size
        GetDefaultIcon( pDsc->value, puRec);
        ulTotSize += puRec[eSIZE];

        // next entry:  title
        pDsc = pDsc->pnext;
        if (pDsc->rc == 0)
        {
            pText = pDsc->pget;
            while ((pText = strpbrk( pText, "\r\n")) != 0)
                *pText++ = ' ';
            pText = pDsc->pget;
        }
        else
            pText = "Error";

        puRec[eTITLE] = (ULONG)pBuf;
        strcpy( pBuf, pText);
        pBuf = strchr( pBuf, 0) + 1;

        // path
        pDsc = pDsc->pnext;
        if (pDsc->rc == 0)
        {
            pText = pDsc->pget;
            while ((pText = strpbrk( pText, "\r\n")) != 0)
                *pText++ = ' ';
            pText = pDsc->pget;
        }
        else
            pText = "Error";

        puRec[ePATH] = (ULONG)pBuf;
        strcpy( pBuf, pText);
        pBuf = strchr( pBuf, 0) + 1;
    }

    // return the ptr to the next record &
    // the next position in the string buffer
    *ppRec = pRec;
    *ppBuf = pBuf;

} while (fFalse);

    // free the request block RwsClient allocated on our behalf
    RwsFreeMem( pHdr);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "GetListItems:  rc= %d %s\n", rc, szError);
    }

    return (rc);
}

/****************************************************************************/

// This gets the object's default icon by:  1) retrieving the custom icon's
// data;  2) resetting the object to its default icon;  3) retrieving that
// icon;  and 4) restoring the custom icon from the saved icon data.  It
// attempts to reduce the overhead incurred by object conversions and method
// lookups by reusing:  a) the WPS object ptr returned when the custom icon
// was retrieved;  b) the server request returned for wpQueryIconData when
// calling wpSetIconData;  and c) the ptr to the function which implements
// the wpSetIconData method for this object

ULONG       GetDefaultIcon( ULONG ulObj, PULONG puRec)

{
    ULONG   	rc = 0;
    ULONG       ulPFN;
    PRWSHDR     pHdrData = 0;
    PRWSHDR     pHdrMisc = 0;
    ICONINFO *  pII;

do
{
    // preset the default icon to our error icon
    puRec[eDEF] = hErrIcon;

    // get the data for the custom icon
    ERRRTN( RwsCall( &pHdrData, RWSP_MNAM, (ULONG)"wpQueryIconData",
                                RWSR_ASIS, 0, 2,
                                RWSI_ASIS, 0, ulObj,
                                RWSI_PBUF, 0x6000, 0))

    // get the ptr to the ICONINFO that RwsServer put in arg2
    pII = (ICONINFO*)RwsGetResult( pHdrData, 2, 0);
    if ((LONG)pII == -1)
        ERRNBR( 1)

    // store the size of the custom icon
    puRec[eSIZE] = pII->cbIconData;

    // reset the object to its default icon
    ERRRTN( RwsCall( &pHdrMisc, RWSP_MNAM, (ULONG)"wpSetIconData",
                                RWSR_ASIS, 0, 2,
                            	RWSI_ASIS, 0, ulObj,
                                RWSI_PBUF, sizeof( iiClear), (ULONG)&iiClear))

    // save the address of this object's "wpSetIconData" method
    ulPFN = CALCPROCPTR( pHdrMisc)->value;

    // free the request block allocated on the last call
    RwsFreeMem( pHdrMisc);
    pHdrMisc = 0;

    // get the default icon;  using a conversion function rather than the
    // corresponding method call avoids having to do a method name lookup
    rc = RwsCall( &pHdrMisc, RWSP_CONV, 0,
                             RWSR_ASIS, 0, 1,
                             RWSC_OBJ_OICON, 0, ulObj);

    // if successful, store the default icon
    if (rc == 0)
    {
        puRec[eDEF] = RwsGetResult( pHdrMisc, 1, 0);
        if (puRec[eDEF] == (ULONG)-1)
        {
            puRec[eDEF] = hErrIcon;
            rc = 2;
        }
    }

    // reuse the first server request block which already has the icon
    // data in place;  change the procedure info so the server can call
    // "wpSetIconData" without having to do a name lookup
    CALCPROCPTR( pHdrData)->value = ulPFN;
    CALCPROCPTR( pHdrData)->type  = RWSP_MPFN;
    ERRRTN( RwsDispatch( pHdrData))

} while (fFalse);

    // free the server request blocks
    RwsFreeMem( pHdrMisc);
    RwsFreeMem( pHdrData);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "GetDefaultIcon:  rc= %d %s\n", rc, szError);
    }

    return (rc);
}

/****************************************************************************/

// deletes any records in the container and destroys their icons
// prior to reloading the container with fresh data

void        EmptyCnr( void)

{
    PMINIRECORDCORE pRec;

do
{
    // get the first record
    pRec = WinSendMsg( hCnr, CM_QUERYRECORD, 0,
                       MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER));

    // if the cnr is empty, exit
    if (pRec == 0)
        break;

    // for each record, destroy the custom & default icons, but be
    // careful not to destroy the icon displayed when there's an error
    do {
        if ((LONG)pRec == -1)
        {
            printf( "invalid record - aborting\n");
            break;
        }

        if (((PULONG)&pRec[1])[eCUST] != hErrIcon)
            WinDestroyPointer( ((PULONG)&pRec[1])[eCUST]);

        if (((PULONG)&pRec[1])[eDEF] != hErrIcon)
            WinDestroyPointer( ((PULONG)&pRec[1])[eDEF]);
        
        pRec = WinSendMsg( hCnr, CM_QUERYRECORD, pRec,
                           MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER));
    } while (pRec);

    // remove all records and free them
    if (WinSendMsg( hCnr, CM_REMOVERECORD, 0,
                    MPFROM2SHORT( 0, CMA_FREE | CMA_INVALIDATE)) != 0)
        printf( "GetData - CM_REMOVERECORD failed\n");

} while (fFalse);

    return;
}

/****************************************************************************/
/****************************************************************************/

