/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// ICNZCMD.C
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

#define INCL_DOSERRORS
#include "ICNZ.H"

/****************************************************************************/

void            PopupMenu( HWND hwnd, PMINIRECORDCORE pRec);
void            Command( HWND hwnd, ULONG ulCmd);
void            ToggleDelete( void);
void            SetDelete( BOOL fDel);
void            ShowObjMenu( HWND hwnd);
void            OpenObject( HWND hwnd);
void            LocateObject( HWND hwnd);
void            SaveIcon( HWND hwnd);
BOOL        	ConfirmPath( char * pszPath);
BOOL            SaveChanges( char * pszTitle, BOOL fExit);
MRESULT _System SaveDlgProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);


/****************************************************************************/

extern BOOL         fFalse;
extern BOOL         fRestoreDefaults;
extern HWND         hDlg;
extern HWND         hCnr;
extern HWND         hmnuMain;
extern HWND         hmnuSingle;
extern HWND         hmnuMultiple;
extern HPOINTER     hErrIcon;
extern ULONG        ulTotCnt;
extern ULONG        ulTotSize;
extern ULONG        ulDelCnt;
extern ULONG        ulDelSize;
extern ICONINFO     iiClear;
extern PMINIRECORDCORE precMenu;
extern char         szError[64];

/****************************************************************************/

// displays one of two context menus, depending on the number
// of items selected (zero or one item vs. multiple items)

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
        (MP)((pRec->flRecordAttr & CRA_INUSE) ? "~Undelete" : "~Delete"));
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

        case IDM_SAVEICON:
            SaveIcon( hwnd);
        break;

        case IDM_SORTCUST:
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

        case IDM_INTUITIVE:
        case IDM_LITERAL:
            SetSortIndicators( ulCmd == IDM_LITERAL);
            SetSortColumn( (ULONG)-1);
            break;

        case IDM_RESTOREDEF:
            fRestoreDefaults ^= TRUE;
            WinSendMsg( hmnuMain, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_RESTOREDEF, TRUE),
                MPFROM2SHORT( MIA_CHECKED, (fRestoreDefaults ? MIA_CHECKED : 0)));
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
        if (pRec->flRecordAttr & CRA_INUSE)
        {
            pRec->flRecordAttr ^= CRA_INUSE;
            ulDelCnt--;
            ulDelSize -= ((PULONG)&pRec[1])[eSIZE];
        }
        else
        {
            pRec->flRecordAttr ^= CRA_INUSE;
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

        if (pRec->flRecordAttr & CRA_INUSE)
        {
            pRec->flRecordAttr ^= CRA_INUSE;
            ulDelCnt--;
            ulDelSize -= ((PULONG)&pRec[1])[eSIZE];
        }
        else
        {
            pRec->flRecordAttr ^= CRA_INUSE;
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
            if ((pRec->flRecordAttr & CRA_INUSE) == 0)
            {
                pRec->flRecordAttr ^= CRA_INUSE;
                ulDelCnt++;
                ulDelSize += ((PULONG)&pRec[1])[eSIZE];
            }
        }
        else
        {
            if (pRec->flRecordAttr & CRA_INUSE)
            {
                pRec->flRecordAttr ^= CRA_INUSE;
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
    ulHndl = strtoul( ((char**)&pRec[1])[eKEY], 0, 16) + 0x20000;

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
    ulHndl = strtoul( ((char**)&pRec[1])[eKEY], 0, 16) + 0x20000;

    // RWSCMD_OPEN is similar to wpViewObject() except that it reliably
    // returns the hwnd of the target object;  it's also preferable in
    // RWS terms because it opens new windows on the WPS's primary (UI)
    // thread rather than on RwsServer's thread
    ERRRTN( RwsCall( &pHdr, RWSP_CMD,   RWSCMD_OPEN,
                            RWSR_ASIS,  0, 3,
                            RWSI_OHNDL, 0, ulHndl,      // object
                            RWSI_ASIS,  0, 0,           // view
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
    ulHndl = strtoul( ((char**)&pRec[1])[eKEY], 0, 16) + 0x20000;

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

// saves the custom icon data to file

void        SaveIcon( HWND hwnd)

{
    ULONG           rc = 0;
    ULONG           ul;
    HFILE           hFile = 0;
    PRWSHDR         pHdr = 0;
    PMINIRECORDCORE pRec;
    ICONINFO *      pII;
    char *          ptr;
    char *          pErr = 0;
    FILEDLG         fd;

do
{
    // this should only be called if the single-object menu was used
    pRec = precMenu;
    precMenu = 0;
    if (pRec == 0)
        ERRNBR( 1)

    // init the filedlg struct
    memset( &fd, 0, sizeof( fd));
    fd.cbSize = sizeof( FILEDLG);
    fd.fl = FDS_ENABLEFILELB | FDS_SAVEAS_DIALOG | FDS_CENTER;
    fd.pszTitle = "Iconomize - Save Icon";
    fd.pszOKButton = "Save";

    // replace any dots & illegal characters in the filename stem
    // with exclamation points, then append .ico
    strcpy( fd.szFullFile, ((char**)&pRec[1])[ePATH]);
    ul = strlen( fd.szFullFile) - strlen( ((char**)&pRec[1])[eTITLE]);
    ptr = &fd.szFullFile[ul];
    while ((ptr = strpbrk( ptr, ".\\/\"<>|:")) != 0)
        *ptr++ = '!';
    strcat( fd.szFullFile, ".ICO");

    // display the file dialog
    if (WinFileDlg( HWND_DESKTOP, hwnd, &fd) == FALSE)
        ERRNBR( 2)

    if (fd.lReturn != DID_OK)
        break;

    // get the object's handle from the Key column,
    ul = strtoul( ((char**)&pRec[1])[eKEY], 0, 16) + 0x20000;

    // get the data for the custom icon
    ERRRTN( RwsCall( &pHdr, RWSP_MNAM, (ULONG)"wpQueryIconData",
                            RWSR_ASIS, 0, 2,
                            RWSI_OHNDL, 0, ul,
                            RWSI_PBUF, 0x6000, 0))

    // get the ptr to the ICONINFO that RwsServer put in arg2
    pII = (ICONINFO*)RwsGetResult( pHdr, 2, 0);
    if ((LONG)pII == -1)
        ERRNBR( 3)

    // sanity check:  if the data size doesn't match
    // the value we already have, exit
    if (pII->cbIconData != ((PULONG)&pRec[1])[eSIZE])
        ERRNBR( 4)

    // ensure the path exists;  if it doesn't, this will create it
    if (ConfirmPath( fd.szFullFile) == FALSE)
        ERRNBR( 5)

    // open the file
    rc = DosOpen( fd.szFullFile, &hFile, &ul, pII->cbIconData, 0,
         OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_FAIL_IF_EXISTS,
         OPEN_FLAGS_SEQUENTIAL | OPEN_SHARE_DENYWRITE | OPEN_ACCESS_WRITEONLY,
         0);

    // if the file already exists, reset rc to zero so no error msg
    // will be shown, then see if the user wants to replace it;
    // if not, exit, otherwise, try to open it again
    if (rc)
    {
        if (rc != ERROR_OPEN_FAILED)
            ERRNBR( 6)

        rc = 0;
        if (WinMessageBox( HWND_DESKTOP, hwnd,
                           "File already exists.  Do you want to replace it?",
                           "Iconomize - Save Icon", 99, 
                           MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 |
                           MB_MOVEABLE) != MBID_YES)
            break;

        if (DosOpen( fd.szFullFile, &hFile, &ul, pII->cbIconData, 0,
                     OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
                     OPEN_FLAGS_SEQUENTIAL | OPEN_SHARE_DENYWRITE |
                     OPEN_ACCESS_WRITEONLY, 0))
            ERRNBR( 7)
    }

    // write the icon data
    if (DosWrite( hFile, pII->pIconData, pII->cbIconData, &ul))
        ERRNBR( 8)

    // signal success
    pErr = "Icon saved";

} while (fFalse);

    // if the file got opened, close it
    if (hFile)
        DosClose( hFile);

    // free the request block RwsClient allocated on our behalf
    RwsFreeMem( pHdr);

    // if there was an error, generate a msg
    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        sprintf( fd.szFullFile, "Save Icon failed - rc= %d %s", rc, szError);
        pErr = fd.szFullFile;
    }

    // display the result (if any)
    UpdateStatus( pErr);

    return;
}

/****************************************************************************/

// receives a fully-qualified filename & validates its path,
// creating those directories which don't exist yet

BOOL        ConfirmPath( char * pszPath)

{
    ULONG           rc = ERROR_PATH_NOT_FOUND;
    FILESTATUS3     fs;
    char *          pEnd;
    char *          ptr;

do
{
    // find the backslash which separates file from path
    ptr = pEnd = strrchr( pszPath, '\\');
    if (pEnd == 0)
        break;

    // isolate the trailing path component (which is known not to exist)
    // by replacing the backslash with a null, then see if the remainder
    // exists;  repeat until a valid path is found or there are no more
    // backslashes (i.e. until the drive designator is reached)
    do {
        *ptr = 0;
        rc = DosQueryPathInfo( pszPath, FIL_STANDARD, &fs, sizeof( fs));
        if (rc != ERROR_FILE_NOT_FOUND && rc != ERROR_PATH_NOT_FOUND)
            break;
    } while ((ptr = strrchr( pszPath, '\\')) != 0);

    // any rc other than file not found (which occurs when
    // we get back to the drive) is a true error, so exit
    if (rc && rc != ERROR_FILE_NOT_FOUND)
        break;

    // now scan forward looking for the each null inserted above;
    // restore the backslash then create the directory;  repeat
    // until we reach the null separating the file from the path
    ptr = pszPath;
    while ((ptr = strchr( ptr, 0)) < pEnd)
    {
        *ptr = '\\';
        rc = DosCreateDir( pszPath, 0);
        if (rc)
            break;
    }

} while (fFalse);

    // restore the backslash separating file from path
    if (pEnd)
        *pEnd = '\\';

    // any rc other than zero signals an error
    return (rc ? FALSE : TRUE);
}

/****************************************************************************/
/****************************************************************************/

// this is called if the user chooses Exit, Reload, or Save Now;
// if there's anything to delete, it presents one of two confirmation
// dialogs whose title bar identifies the operation

BOOL        SaveChanges( char * pszTitle, BOOL fExit)

{
    BOOL            fRtn = TRUE;
    ULONG           rc = 0;
    ULONG           ul;
    PMINIRECORDCORE pRec;
    PMINIRECORDCORE pRecNext;
    PRWSHDR         pHdr = 0;
    PRWSDSC         pArg = 0;
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

    // build a server request block;  we'll reuse it for each object
    // whose icon needs to be reset (note:  RwsClient will reject our
    // request if the object handle is zero, so we supply a dummy value)
    ERRRTN( RwsBuild( &pHdr, RWSP_MNAM, (ULONG)"wpSetIconData",
                      RWSR_ASIS, 0, 2,
                      RWSI_OHNDL, 0, 1,
                      RWSI_PBUF, sizeof( iiClear), (ULONG)&iiClear))

    // get a ptr to the first arg (the object handle)
    ERRRTN( RwsGetArgPtr( pHdr, 1, &pArg))

    // get the first record
    pRecNext = WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS,
                           (MP)CMA_FIRST, (MP)CRA_INUSE);

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
        pRecNext = WinSendMsg( hCnr, CM_QUERYRECORDEMPHASIS,
                               pRec, (MP)CRA_INUSE);

        // save the key for use with the error msgs below
        strcpy( szText, ((char **)&pRec[1])[eKEY]);

        // put the object's handle into the arg's "give" buffer
        // (i.e. the space used to hold data that needs to be
        // converted - in this case, it's a DWORD at pArg[1])
        *((PULONG)CALCGIVEPTR( pArg)) = strtoul( szText, 0, 16) + 0x20000;

        // remove the custom icon for this object using the pre-built
        // request;  if there's an error, note it but continue
        ul = RwsDispatch( pHdr);
        if (ul)
        {
            fRtn = FALSE;
            RwsGetRcString( ul, sizeof( szError), szError);
            printf( "default icon reset failed for key %s  rc= %x %s\n",
                    szText, ul, szError);
        }

        // if the user chose Exit or Reload, the records will
        // be removed and the icons destroyed by EmptyCnr(),
        // so there's no need to do that here
        if (fExit)
            continue;

        // if the reset succeeded, update the stats
        if (ul == 0)
        {
            ulTotCnt--;
            ulDelCnt--;
            ulTotSize -= ((PULONG)&pRec[1])[eSIZE];
            ulDelSize -= ((PULONG)&pRec[1])[eSIZE];
        }

        // delete the icons (unless it's the icon used for errors)
        if (((PULONG)&pRec[1])[eCUST] != hErrIcon)
            WinDestroyPointer( ((PULONG)&pRec[1])[eCUST]);

        if (((PULONG)&pRec[1])[eDEF] != hErrIcon)
            WinDestroyPointer( ((PULONG)&pRec[1])[eDEF]);

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

    // free the request block RwsClient allocated on our behalf
    RwsFreeMem( pHdr);

    if (rc)
    {
        RwsGetRcString( rc, sizeof( szError), szError);
        printf( "SaveChanges:  rc= %d %s\n", rc, szError);
    }

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
        sprintf( szText, "Iconomize - %s", (char*)mp2);
        WinSetWindowText( hwnd, szText);
        WinRestoreWindowPos( "ICONOMIZE", "SAVEPOS", hwnd);
        return (0);
    }

    if (msg == WM_SAVEAPPLICATION)
        WinStoreWindowPos( "ICONOMIZE", "SAVEPOS", hwnd);

    return (WinDefDlgProc( hwnd, msg, mp1, mp2));
}

/****************************************************************************/
/****************************************************************************/

