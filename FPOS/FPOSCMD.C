/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// FPOSCMD.C
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

#include "FPOS.H"

/****************************************************************************/

void            PopupMenu( HWND hwnd, PMINIRECORDCORE pRec);
void            Command( HWND hwnd, ULONG ulCmd);
void            ToggleDelete( void);
void            SetDelete( BOOL fDel);
void            ShowObjMenu( HWND hwnd);
void            OpenObject( HWND hwnd);
void            LocateObject( HWND hwnd);
BOOL            SaveChanges( char * pszTitle, BOOL fExit);
MRESULT _System SaveDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT _System OptionsDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
void            InitOptsDlg( HWND hwnd);
void            ChangeSortOrder( HWND hwnd, BOOL fUp);
void        	CloseOptsDlg( HWND hwnd, BOOL fSave);

/****************************************************************************/

extern BOOL         fFalse;
extern BOOL         fRestoreDefaults;
extern HWND         hDlg;
extern HWND         hCnr;
extern HWND         hmnuSingle;
extern HWND         hmnuMultiple;
extern HPOINTER     hDelIco;
extern HPOINTER     hChkIco;
extern ULONG        ulTotCnt;
extern ULONG        ulTotSize;
extern ULONG        ulDelCnt;
extern ULONG        ulDelSize;
extern char *       apszView[VIEWCNT];
extern PMINIRECORDCORE precMenu;
extern char         szError[64];

/****************************************************************************/

void        PopupMenu( HWND hwnd, PMINIRECORDCORE pRec)

{
    HWND        hMenu;
    POINTL      ptl;

do
{
    // only show the menu if we're over an item
    if (pRec == 0)
        break;

    // if 0 or 1 item is selected, show the single object menu;
    // if more than 1 is selected, show the multiple object menu
    if ((pRec->flRecordAttr & CRA_SELECTED) == 0)
    {
        hMenu = hmnuSingle;
        WinSendMsg( hCnr, CM_SETRECORDEMPHASIS, (MP)pRec,
                    MPFROM2SHORT( TRUE, CRA_SOURCE));
    }
    else
    {
        if (WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS, (MP)CMA_FIRST,
                        (MP)CRA_SELECTED) != (MP)pRec)
            hMenu = hmnuMultiple;
        else
        if (WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS, (MP)pRec,
                        (MP)CRA_SELECTED) != 0)
            hMenu = hmnuMultiple;
        else
            hMenu = hmnuSingle;
    }

    // if we're using the single object menu, save the item it refers to 
    // and set the appropriate menu text;  otherwise, set this to zero
    if (hMenu == hmnuMultiple)
        precMenu = 0;
    else
    {
        precMenu = pRec;
        WinSendMsg( hMenu, MM_SETITEMTEXT, (MP)IDM_DELETE,
        (MP)((((PULONG)&pRec[1])[eDEL] == hDelIco) ? "~Undelete" : "~Delete"));
    }

    // popup the menu at the mouse pointer
    WinQueryPointerPos( HWND_DESKTOP, &ptl);
    WinPopupMenu( HWND_DESKTOP, hwnd, hMenu, ptl.x, ptl.y, 0,
                  PU_HCONSTRAIN | PU_VCONSTRAIN |
                  PU_MOUSEBUTTON1 | PU_KEYBOARD);

} while (fFalse);

    return;
}

/****************************************************************************/

void        Command( HWND hwnd, ULONG ulCmd)

{
    switch (ulCmd)
    {
        case IDM_DELETE:
            ToggleDelete();
        break;

        case IDM_DELETEALL:
            SetDelete( TRUE);
        break;

        case IDM_UNDELETEALL:
            SetDelete( FALSE);
        break;

        case IDM_OPEN:
            OpenObject( hwnd);
        break;

        case IDM_LOCATE:
            LocateObject( hwnd);
        break;

        case IDM_MENU:
            ShowObjMenu( hwnd);
        break;

        case IDM_SORTDEL:
        case IDM_SORTNBR:
        case IDM_SORTVIEW:
        case IDM_SORTTITLE:
        case IDM_SORTSIZE:
        case IDM_SORTKEY:
        case IDM_SORTPATH:
            SetSortColumn( ulCmd - IDM_SORTFIRST);
            break;

        case IDM_SORTDONE:
            break;

        case IDM_GETDATA:
            if (SaveChanges( "Reload", TRUE))
                GetData();
            break;

        case IDM_RESETCOL:
            ResetColumnWidths();
            break;

        case IDM_SAVE:
            SaveChanges( "Save now", FALSE);
            break;

        case IDM_OPTDLG:
            WinDlgBox( HWND_DESKTOP,       // parent-window
                       hwnd,               // owner-window
                       OptionsDlgProc,     // dialog proc
                       NULLHANDLE,         // EXE module handle
                       IDD_OPTIONS,        // dialog id
                       NULL);              // pointer to create params
            break;
    }

    return;
}

/****************************************************************************/

void        ToggleDelete( void)

{
    PMINIRECORDCORE pRec;

do
{
    pRec = precMenu;
    precMenu = 0;

    // the single-object menu was used - toggle that object's status
    // then have it repainted
    if (pRec)
    {
        if (((PULONG)&pRec[1])[eDEL] == hDelIco)
        {
            ((PULONG)&pRec[1])[eDEL] = hChkIco;
            ulDelCnt--;
            ulDelSize -= ((PULONG)&pRec[1])[eSIZE];
        }
        else
        {
            ((PULONG)&pRec[1])[eDEL] = hDelIco;
            ulDelCnt++;
            ulDelSize += ((PULONG)&pRec[1])[eSIZE];
        }

        WinSendMsg( hCnr, CM_INVALIDATERECORD, (MP)&pRec,
                    MPFROM2SHORT( 1, CMA_NOREPOSITION));
        break;
    }

    // the Delete key was pressed - get each selected object,
    // toggle its status, then have it repainted
    pRec = (PMINIRECORDCORE)CMA_FIRST;

    while ((pRec = (PMINIRECORDCORE)WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS,
                                    (MP)pRec, (MP)CRA_SELECTED)) != 0)
    {
        if (pRec == (PMINIRECORDCORE)-1)
            break;

        if (((PULONG)&pRec[1])[eDEL] == hDelIco)
        {
            ((PULONG)&pRec[1])[eDEL] = hChkIco;
            ulDelCnt--;
            ulDelSize -= ((PULONG)&pRec[1])[eSIZE];
        }
        else
        {
            ((PULONG)&pRec[1])[eDEL] = hDelIco;
            ulDelCnt++;
            ulDelSize += ((PULONG)&pRec[1])[eSIZE];
        }

        WinSendMsg( hCnr, CM_INVALIDATERECORD, (MP)&pRec,
                    MPFROM2SHORT( 1, CMA_NOREPOSITION));
    }

} while (fFalse);

    UpdateStatus( 0);

    return;
}

/****************************************************************************/

void        SetDelete( BOOL fDel)

{
    PMINIRECORDCORE pRec;

    precMenu = 0;

    // the multiple-object menu was used - get each selected object,
    // set its status, then have it repainted
    pRec = (PMINIRECORDCORE)CMA_FIRST;

    while ((pRec = (PMINIRECORDCORE)WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS,
                                    (MP)pRec, (MP)CRA_SELECTED)) != 0)
    {
        if (pRec == (PMINIRECORDCORE)-1)
            break;

        if (fDel)
        {
            if (((PULONG)&pRec[1])[eDEL] != hDelIco)
            {
                ((PULONG)&pRec[1])[eDEL] = hDelIco;
                ulDelCnt++;
                ulDelSize += ((PULONG)&pRec[1])[eSIZE];
            }
        }
        else
        {
            if (((PULONG)&pRec[1])[eDEL] != hChkIco)
            {
                ((PULONG)&pRec[1])[eDEL] = hChkIco;
                ulDelCnt--;
                ulDelSize -= ((PULONG)&pRec[1])[eSIZE];
            }
        }

        WinSendMsg( hCnr, CM_INVALIDATERECORD, (MP)&pRec,
                    MPFROM2SHORT( 1, CMA_NOREPOSITION));
    }

    UpdateStatus( 0);

    return;
}

/****************************************************************************/

void        ShowObjMenu( HWND hwnd)

{
    ULONG           rc = 0;
    ULONG           ulHndl;
    PRWSHDR         pHdr = 0;
    PMINIRECORDCORE pRec;
    POINTL          ptl;

do
{
    pRec = precMenu;
    precMenu = 0;

    // this should only be called if the single-object menu was used
    if (pRec == 0)
        ERRNBR( 1)

    // get the object's handle from the Key column
    ulHndl = strtoul( ((char**)&pRec[1])[eKEY], 0, 10);

    // get the mouse position & convert to container coordinates
    WinQueryPointerPos( HWND_DESKTOP, &ptl);
    WinMapWindowPoints( HWND_DESKTOP, hCnr, &ptl, 1);

    // this command calls wpDisplayMenu() with the "important" arguments
    // preset by RwsServer;  hwnd is the window that should get the focus
    // when the menu is dismissed, and ptl is the menu location in hwnd
    // coordinates - both arguments are optional;  the command returns
    // the popupmenu's hwnd which we don't use here
    ERRRTN( RwsCall( &pHdr, RWSP_CMD, RWSCMD_POPUPMENU,
                            RWSR_ASIS,  0, 3,
                            RWSI_OHNDL, 0, ulHndl,
                            RWSI_ASIS,  0, hCnr,
                            RWSI_PBUF,  sizeof(POINTL), (ULONG)&ptl))

} while (fFalse);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "ShowObjMenu:  rc= %d %s\n", rc, szError);
    }

    // free the server request block
    RwsFreeMem( pHdr);

    return;
}

/****************************************************************************/

void        OpenObject( HWND hwnd)

{
    ULONG           rc = 0;
    HWND            hRtn;
    ULONG           ulHndl;
    ULONG           ulView;
    char *          ptr;
    PRWSHDR         pHdr = 0;
    PMINIRECORDCORE pRec;

do
{
    pRec = precMenu;
    precMenu = 0;

    // this should only be called if the single-object menu was used
    if (pRec == 0)
        ERRNBR( 1)

    // get the object's handle from the Key column,
    // then get the view this entry refers to
    ulHndl = strtoul( ((char**)&pRec[1])[eKEY], 0, 10);
    ptr = strchr( ((char**)&pRec[1])[eKEY], '@');
    if (ptr)
        ulView = strtoul( ++ptr, 0, 10) / 10;
    else
        ulView = 0;

    // RWSCMD_OPEN is similar to wpViewObject() except that it reliably
    // returns the hwnd of the target object;  it's also preferable in
    // RWS terms because it opens new windows on the WPS's primary (UI)
    // thread rather than on RwsServer's thread
    ERRRTN( RwsCall( &pHdr, RWSP_CMD,   RWSCMD_OPEN,
                            RWSR_ASIS,  0, 3,
                            RWSI_OHNDL, 0, ulHndl,      // object
                            RWSI_ASIS,  0, ulView,      // view
                            RWSI_ASIS,  0, 0))          // fNew = FALSE

    // if this isn't the active window, the focus is not ours to give away
    if (WinQueryActiveWindow( HWND_DESKTOP) != hwnd)
        break;

    // get the hwnd of the target window, then move the focus to it
    hRtn = RwsGetResult( pHdr, 0, 0);
    if (hRtn == 0 || hRtn == (ULONG)-1)
        ERRNBR( 2)

    WinFocusChange( HWND_DESKTOP, hRtn, 0);

} while (fFalse);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "OpenObject:  rc= %d %s\n", rc, szError);
    }

    // free the server request block
    RwsFreeMem( pHdr);

    return;
}

/****************************************************************************/

void        LocateObject( HWND hwnd)

{
    ULONG           rc = 0;
    HWND            hRtn;
    ULONG           ulHndl;
    PRWSHDR         pHdr = 0;
    PMINIRECORDCORE pRec;

do
{
    pRec = precMenu;
    precMenu = 0;

    // this should only be called if the single-object menu was used
    if (pRec == 0)
        ERRNBR( 1)

    // get the object's handle from the Key column,
    ulHndl = strtoul( ((char**)&pRec[1])[eKEY], 0, 10);

    // RWSCMD_LOCATE opens the folder containing the target object,
    // then moves the selection to it, scrolling the window if needed
    ERRRTN( RwsCall( &pHdr, RWSP_CMD,   RWSCMD_LOCATE,
                            RWSR_ASIS,  0, 1,
                            RWSI_OHNDL, 0, ulHndl))     // object

    // if this isn't the active window, the focus is not ours to give away
    if (WinQueryActiveWindow( HWND_DESKTOP) != hwnd)
        break;

    // get the hwnd of the target window, then move the focus to it
    hRtn = RwsGetResult( pHdr, 0, 0);
    if (hRtn == 0 || hRtn == (ULONG)-1)
        ERRNBR( 2)

    WinFocusChange( HWND_DESKTOP, hRtn, 0);

} while (fFalse);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "LocateObject:  rc= %d %s\n", rc, szError);
    }

    // free the server request block
    RwsFreeMem( pHdr);

    return;
}

/****************************************************************************/
/****************************************************************************/

// this is called if the user chooses Exit, Reload, or Save Now;
// if there's anything to delete, it presents one of two confirmation
// dialogs whose title bar identifies the operation

BOOL        SaveChanges( char * pszTitle, BOOL fExit)

{
    BOOL            fRtn = TRUE;
    ULONG           ul;
    PMINIRECORDCORE pRec;
    PMINIRECORDCORE pRecNext;
    char            szText[32];

do
{
    // if nothing to do, exit
    if (ulDelCnt == 0)
        break;

    // show confirmation dialog
    ul = WinDlgBox( HWND_DESKTOP, hDlg, SaveDlgProc, NULLHANDLE,
                    (fExit ? IDD_SAVEEXIT : IDD_SAVE), pszTitle);

    // if the answer was No or Cancel, exit;
    // returning false cancels the operation
    if (ul != MBID_YES)
    {
        if (ul != MBID_NO)
            fRtn = FALSE;
        break;
    }

    // get the first record
    pRecNext = WinSendMsg( hCnr, CM_QUERYRECORD, 0,
                           MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER));

    while (pRecNext != 0)
    {
        // if there's an error, we have to abort
        if ((LONG)pRecNext == -1)
        {
            printf( "invalid record - aborting\n");
            fRtn = FALSE;
            break;
        }

        // identify the next record before we delete the current one
        pRec = pRecNext;
        pRecNext = WinSendMsg( hCnr, CM_QUERYRECORD, pRec,
                               MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER));

        // if this record isn't marked for deletion, continue
        if (((PULONG)&pRec[1])[eDEL] != hDelIco)
            continue;

        // save the key for use with the error msgs below
        strcpy( szText, ((char **)&pRec[1])[eKEY]);

        // delete the ini entry corresponding to this record
        if (PrfWriteProfileData( HINI_USERPROFILE, "PM_Workplace:FolderPos",
                                 ((char **)&pRec[1])[eKEY], 0, 0) == FALSE)
        {
            fRtn = FALSE;
            printf( "PrfWriteProfileData() failed for key %s\n", szText);
        }

        // if the user chose Exit or Reload, the records will be
        // removed by GetData(), so there's no need to do that here
        if (fExit)
            continue;

        // update the stats, even if the deletion failed because the most
        // common cause of failure is that the key has already been deleted
        ulTotCnt--;
        ulDelCnt--;
        ulTotSize -= ((PULONG)&pRec[1])[eSIZE];
        ulDelSize -= ((PULONG)&pRec[1])[eSIZE];

        // remove the record & free it
        if (WinSendMsg( hCnr, CM_REMOVERECORD, (MP)&pRec,
                        MPFROM2SHORT( 1, CMA_FREE)) == (MRESULT)-1)
        {
            fRtn = FALSE;
            printf( "CM_REMOVERECORD failed for key %s\n", szText);
        }
    }

    // if this was a 'Save Now', update the cnr & the status line
    if (fExit == FALSE)
    {
        WinSendMsg( hCnr, CM_INVALIDATERECORD, 0, 0);
        UpdateStatus( (fRtn ? 0 : "Error(s) deleting items"));
    }

    // signal that the requested operation should proceed
    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

MRESULT _System SaveDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    if (msg == WM_INITDLG)
    {
        char    szText[128];

        sprintf( szText, "%d entries (%d bytes)", ulDelCnt, ulDelSize);
        WinSetDlgItemText( hwnd, IDC_TEXT, szText);
        sprintf( szText, "FPos - %s", (char*)mp2);
        WinSetWindowText( hwnd, szText);
        WinRestoreWindowPos( "FPOS", "SAVEPOS", hwnd);
        return (0);
    }

    if (msg == WM_SAVEAPPLICATION)
        WinStoreWindowPos( "FPOS", "SAVEPOS", hwnd);

    return (WinDefDlgProc( hwnd, msg, mp1, mp2));
}

/****************************************************************************/
/****************************************************************************/

MRESULT _System OptionsDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    switch (msg)
    {
        case WM_COMMAND:
            switch (SHORT1FROMMP( mp1))
            {
                case IDC_SORTUP:
                    ChangeSortOrder( hwnd, TRUE);
                    return (0);

                case IDC_SORTDOWN:
                    ChangeSortOrder( hwnd, FALSE);
                    return (0);

                case IDC_OK:
                    CloseOptsDlg( hwnd, TRUE);
                    return (0);
            }
            break;

        case WM_INITDLG:
            InitOptsDlg( hwnd);
            return (0);

        case WM_SAVEAPPLICATION:
            WinStoreWindowPos( "FPOS", "OPTPOS", hwnd);
            break;

    } //end switch (msg)

    return (WinDefDlgProc( hwnd, msg, mp1, mp2));
}

/****************************************************************************/

void        InitOptsDlg( HWND hwnd)

{
    HWND        hLB;
    ULONG       ctr;
    SHORT       ndx;

    WinSendDlgItemMsg( hwnd, IDC_RESTOREDEF, BM_SETCHECK,
                       (MP)fRestoreDefaults, 0);

    if (IsSortIndicatorLiteral())
        WinSendDlgItemMsg( hwnd, IDC_LITERAL, BM_SETCHECK, (MP)TRUE, 0);
    else
        WinSendDlgItemMsg( hwnd, IDC_INTUITIVE, BM_SETCHECK, (MP)TRUE, 0);

    hLB = WinWindowFromID( hwnd, IDC_SORTORDER);
    WinSendMsg( hLB, LM_DELETEALL, 0, 0);

    for (ctr=0; ctr < VIEWCNT; ctr++)
    {
        ndx = (SHORT)WinSendMsg( hLB, LM_INSERTITEM,
                                 (MP)LIT_END, (MP)apszView[ctr]);
        if (ndx >= 0)
            WinSendMsg( hLB, LM_SETITEMHANDLE, (MP)ndx, (MP)apszView[ctr]);
    }

    WinSendMsg( hLB, LM_SELECTITEM, 0, (MP)TRUE);

    WinRestoreWindowPos( "FPOS", "OPTPOS", hwnd);

    return;
}

/****************************************************************************/

void        ChangeSortOrder( HWND hwnd, BOOL fUp)

{
    HWND        hLB;
    char *      ptr;
    SHORT       ndx;

do
{
    hLB = WinWindowFromID( hwnd, IDC_SORTORDER);

    ndx = (SHORT)WinSendMsg( hLB, LM_QUERYSELECTION, (MP)LIT_FIRST, 0);
    if (ndx < 0)
        break;

    if ((ndx == 0 && fUp) || (ndx == VIEWCNT-1 && fUp == FALSE))
        break;

    ptr = (char*)WinSendMsg( hLB, LM_QUERYITEMHANDLE, (MP)ndx, 0);
    if (ptr == 0)
        break;

    WinSendMsg( hLB, LM_DELETEITEM, (MP)ndx, 0);
    ndx = (SHORT)WinSendMsg( hLB, LM_INSERTITEM,
                             (MP)(fUp ? ndx-1 : ndx+1), (MP)ptr);
    WinSendMsg( hLB, LM_SETITEMHANDLE, (MP)ndx, (MP)ptr);
    WinSendMsg( hLB, LM_SELECTITEM, (MP)ndx, (MP)TRUE);

} while (fFalse);

    return;
}

/****************************************************************************/

void        CloseOptsDlg( HWND hwnd, BOOL fSave)

{
    HWND        hLB;
    char *      ptr;
    ULONG       ctr;

do
{
    if (fSave == FALSE)
        break;

    fRestoreDefaults = (BOOL)WinSendDlgItemMsg( hwnd, IDC_RESTOREDEF,
                                                BM_QUERYCHECK, 0, 0);

    SetSortIndicators( (BOOL)WinSendDlgItemMsg(
                             hwnd, IDC_LITERAL, BM_QUERYCHECK, 0, 0));

    hLB = WinWindowFromID( hwnd, IDC_SORTORDER);

    for (ctr=0; ctr < VIEWCNT; ctr++)
    {
        ptr = (char *)WinSendMsg( hLB, LM_QUERYITEMHANDLE, (MP)ctr, 0);
        if (ptr == 0)
            break;

        apszView[ctr] = ptr;
    }

    // update the sort info without changing the sort column or direction
    SetSortColumn( (ULONG)-1);

} while (fFalse);

    WinDismissDlg( hwnd, 0);

    return;
}

/****************************************************************************/
/****************************************************************************/

