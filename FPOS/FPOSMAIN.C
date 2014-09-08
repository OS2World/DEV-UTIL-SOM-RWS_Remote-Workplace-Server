/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// FPOSMAIN.C
// Remote Workplace Server - demo program "FPos"

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
 * The Original Code is Remote Workplace Server - "FPos" demo program.
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
/*

This program enables the user to significantly reduce the size of os2.ini
by deleting folder position data that is stored there unnecessarily.
It demonstrates how to use RWS and employs most of its features.  It also
implements a container in details view that offers user-resizable column
widths and click-to-sort column headings.

*/
/****************************************************************************/

#include "FPOS.H"

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
char *          GetView( char * pszKey, char * pszWork);

/****************************************************************************/

extern COLINFO      ci[];
extern ULONG        ulSortCol;

/****************************************************************************/

char        szCopyright[] = "FPos v" RWSVERSION " - (C)Copyright 2004  R.L.Walsh";
BOOL        fFalse = FALSE;
BOOL        fRestoreDefaults = FALSE;
HWND        hDlg = 0;
HWND        hCnr = 0;
HWND        hStatus = 0;
HWND        hmnuMain = 0;
HWND        hmnuSort = 0;
HWND        hmnuSingle = 0;
HWND        hmnuMultiple = 0;
HPOINTER    hSizePtr = 0;
HPOINTER    hSortPtr = 0;
HPOINTER    hDelIco = 0;
HPOINTER    hChkIco = 0;
HBITMAP     hUpBmp = 0;
HBITMAP     hDownBmp = 0;
PMINIRECORDCORE precMenu = 0;
ULONG       ulTotCnt = 0;
ULONG       ulTotSize = 0;
ULONG       ulDelCnt = 0;
ULONG       ulDelSize = 0;
char *      pStrings = 0;

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
    // RWSFULLVERSION for FPos v0.80 is 0x08000100
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

        case WM_DESTROY:
            RwsClientTerminate();
            break;

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
                PrfWriteProfileData( HINI_USERPROFILE, "FPOS", 0, 0, 0);
            else
            {
                StoreSort();
                WinStoreWindowPos( "FPOS", "WNDPOS", hwnd);
            }
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
    // this will register the RWS class if needed
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

    WinRestoreWindowPos( "FPOS", "WNDPOS", hwnd);

} while (fFalse);

    // if there was an error, abort
    if (rc)
    {
        sprintf( szText, "Error:  %s - rc= %d\nPress 'OK' to end this program",
                 pErr, rc);
        WinMessageBox( HWND_DESKTOP, 0, szText, "FPos", 1,
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

    // get the icons (the check icon is actually blank)
    hDelIco = WinLoadPointer( HWND_DESKTOP, 0, IDI_APPICO);
    hChkIco = WinLoadPointer( HWND_DESKTOP, 0, IDI_CHKICO);

    // set the app icon
    WinSendMsg( hwnd, WM_SETICON, (MP)hDelIco, 0);

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
    cnri.flWindowAttr = CV_DETAIL | CA_DRAWICON | CA_DETAILSVIEWTITLES | CV_MINI;
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
    PMINIRECORDCORE pRec;
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

    // if the container already has data, empty it & free the records
    pRec = WinSendMsg( hCnr, CM_QUERYRECORD, 0,
                       MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER));
    if (pRec)
        if (WinSendMsg( hCnr, CM_REMOVERECORD, 0,
                        MPFROM2SHORT( 0, CMA_FREE | CMA_INVALIDATE)) != 0)
            printf( "GetData - CM_REMOVERECORD failed\n");

    // free the buffer containing string data
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
#define CNTARGS     (3*CNTOBJS)

do
{
    // see how large the list of objects is
    if (PrfQueryProfileSize( HINI_USERPROFILE, "PM_Workplace:FolderPos",
                             0, &size) == FALSE)
        ERRNBR( 1)

    // get some mem to hold the list
    pMem = (char*)malloc( size);
    if (pMem == 0)
        ERRNBR( 2)

    // get the list of objects
    if (PrfQueryProfileData( HINI_USERPROFILE, "PM_Workplace:FolderPos",
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
    // CNTOBJS objects at a time, each requiring 3 RWSARG structs
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
        pArg[ctr].type = RWSC_OHNDL_OTITLE;     // obj handle to obj title
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
        pArg[ctr++].value = strtoul( ptr, 0, 10);

        // 3rd arg:  nothing needed - it will reuse
        // the object pointer obtained for the 2nd arg
        ctr++;

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
    char        szWork[16];

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

    for (ctr=0, pRec=*ppRec, pBuf=*ppBuf; ctr < cnt; ctr+=3, pRec=pRec->preccNextRecord)
    {
        PULONG      puRec = (PULONG)&pRec[1];
        char *      pText;
        char *      pView;

        // point at the first of the 3 entries for this key;  the value
        // contains a pointer to the key string (this wasn't touched by RWS)
        pDsc = pDsc->pnext;

        // del
        puRec[eDEL] = hChkIco;

        // nbr
        puRec[eNBR] = ++ulTotCnt;

        // key
        puRec[eKEY] = (ULONG)pBuf;
        strcpy( pBuf, (char*)pDsc->value);
        pBuf = strchr( pBuf, 0) + 1;

        // view - get it but don't set it until title & path
        // are known to be OK
        pView = GetView( (char*)pDsc->value, szWork);

        // size
        PrfQueryProfileSize( HINI_USERPROFILE, "PM_Workplace:FolderPos",
                             (char*)pDsc->value, &puRec[eSIZE]);
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
            pText = pView = "Error";

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
            pText = pView = "Error";

        puRec[ePATH] = (ULONG)pBuf;
        strcpy( pBuf, pText);
        pBuf = strchr( pBuf, 0) + 1;

        // view revisited - if title or path were bad,
        // the actual view is changed to error
        puRec[eVIEW] = (ULONG)pBuf;
        strcpy( pBuf, pView);
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

// translates the value after the '@' in the key to a meaningful string

char *          GetView( char * pszKey, char * pszWork)

{
    char *      pText = "Error";
    char *      ptr;
    ULONG       ulView;
    ULONG       ulNdx;

do
{
    // if the key doesn't have a @ there's an error
    ptr = strchr( pszKey, '@');
    if (ptr++ == 0)
        break;

    // try to turn the view into a number
    ulView = strtoul( ptr, 0, 10);

    // if it's zero, this could be an XFolder Status Bar entry
    // or there may not be a view
    if (ulView == 0)
    {
        if (strcmp( ptr, "XFSB") == 0)
            pText = "Xwp";
        else
            pText = "Unknown";

        break;
    }

    // the standard view entries consist of the view's numeric value
    // followed by an index - e.g. if you have 2 details views of a folder
    // open at the same time, the first is @1020, the second is @1021
    ulNdx = ulView % 10;
    ulView /= 10;

    if (ulView == 1)
        pText = "Icon";
    else
    if (ulView == 2)
        pText = "Settings";
    else
    if (ulView == 101)
        pText = "Tree";
    else
    if (ulView == 102)
        pText = "Details";
    else
        pText = "Unknown";

    if (ulNdx)
    {
        sprintf( pszWork, "%s-%d", pText, ulNdx);
        pText = pszWork;
    }

} while (fFalse);

    return (pText);
}

/****************************************************************************/

