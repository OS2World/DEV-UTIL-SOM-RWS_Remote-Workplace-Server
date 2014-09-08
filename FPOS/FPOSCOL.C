/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// FPOSCOL.C
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

BOOL            SubclassColumnHeadings( void);
MRESULT _System LeftTitleSubProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT _System RightTitleSubProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
ULONG           InColOrMargin( HWND hwnd, MPARAM mp1, ULONG first, PULONG plast);
void            SetSortColumn( ULONG ulCol);
void            ResizeColumn( HWND hTitle, ULONG ulCol);
void        	GetColumnWidths( void);
LONG        	GetAvgCharWidth( void);
void            ResetColumnWidths( void);
void            ResetSplitBar( void);
void            RestoreSort( void);
void        	StoreSort( void);
void            SetSortIndicators( BOOL fLiteral);
BOOL            IsSortIndicatorLiteral( void);

SHORT   _System SortByDel(   PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);
SHORT   _System SortByNbr(   PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);
SHORT   _System SortByKey(   PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);
SHORT   _System SortBySize(  PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);
SHORT   _System SortByPath(  PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);
SHORT   _System SortByTitle( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);
SHORT   _System SortByView(  PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv);

/****************************************************************************/

extern BOOL         fFalse;
extern HWND         hCnr;
extern HWND         hDlg;
extern HWND         hmnuSort;
extern HPOINTER     hSizePtr;
extern HPOINTER     hSortPtr;
extern HPOINTER     hDelIco;
extern HBITMAP      hUpBmp;
extern HBITMAP      hDownBmp;

/****************************************************************************/

SHORT       sSortSense;
char        chAsc;
char        chDesc;
HBITMAP     hbmAsc;
HBITMAP     hbmDesc;
LONG        lAvg = 6;
LONG        lAvgLeft = 3;
LONG        lAvgRight = 3;
LONG        cyTitle = 28 - 4;
ULONG       ulSortCol;
PFNWP       pfnLeft = 0;
PFNWP       pfnRight = 0;

// these strings have to be writeable
char        szDel[]     = "  X  ";
char        szNbr[]     = "  Nbr";
char        szView[]    = "View  ";
char        szTitle[]   = "Title  ";
char        szSize[]    = "  Size";
char        szKey[]     = "Key  ";
char        szPath[]    = "Path  ";

COLINFO     ci[eCNTCOLS] =
    {
        { CFA_ICO,   0, szDel,   0, 0, (PVOID)&SortByDel },
        { CFA_NBR,   0, szNbr,   0, 0, (PVOID)&SortByNbr },
        { CFA_STR,  16, szView,  0, 0, (PVOID)&SortByView },
        { CFA_STR, 256, szTitle, 0, 0, (PVOID)&SortByTitle },
        { CFA_NBR,   0, szSize,  0, 0, (PVOID)&SortBySize },
        { CFA_STR,  16, szKey,   0, 0, (PVOID)&SortByKey },
        { CFA_STR, 260, szPath,  0, 0, (PVOID)&SortByPath }
    };

char *      apszDefView[VIEWCNT] =
        { "Icon", "Tree", "Details", "Settings", "Xwp", "Unknown", "Error" };

char *      apszView[VIEWCNT];
char        achViewBuf[128];

/****************************************************************************/
/****************************************************************************/

BOOL        SubclassColumnHeadings( void)

{
    HWND        hTmp;

    hTmp = WinWindowFromID( hCnr, CID_LEFTCOLTITLEWND);
    if (hTmp)
        pfnLeft = WinSubclassWindow( hTmp, &LeftTitleSubProc);

    hTmp = WinWindowFromID( hCnr, CID_RIGHTCOLTITLEWND);
    if (hTmp)
        pfnRight = WinSubclassWindow( hTmp, &RightTitleSubProc);

    return (pfnLeft && pfnRight);
}

/****************************************************************************/

MRESULT _System LeftTitleSubProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    ULONG       ctr;
    ULONG       ul;

    switch (msg)
    {
        case WM_MOUSEMOVE:
        {
            ctr = LASTLEFTCOL;
            ul = InColOrMargin( hwnd, mp1, FIRSTLEFTCOL, &ctr);
            if (ul == INNEITHER)
                break;

            if (ul == INMARGIN)
                WinSetPointer( HWND_DESKTOP, hSizePtr);
            else
                WinSetPointer( HWND_DESKTOP, hSortPtr);

            return ((MRESULT)TRUE);
        }

        case WM_BUTTON1DOWN:
        {
            ctr = LASTLEFTCOL;
            if (InColOrMargin( hwnd, mp1, FIRSTLEFTCOL, &ctr) != INMARGIN)
                break;

            pfnLeft( hwnd, msg, mp1, mp2);
            ResizeColumn( hwnd, ctr);
            return ((MRESULT)TRUE);
        }

        case WM_BUTTON1CLICK:
        {
            ctr = LASTLEFTCOL;
            if (InColOrMargin( hwnd, mp1, FIRSTLEFTCOL, &ctr) == INCOLUMN)
                SetSortColumn( ctr);

            break;
        }

        case WM_PRESPARAMCHANGED:
        {
            if ((ULONG)mp1 != PP_FONTNAMESIZE)
                break;

            pfnLeft( hwnd, msg, mp1, mp2);
            WinPostMsg( hDlg, WM_COMMAND, (MP)IDM_RESETCOL, 0);
            return (0);
        }
    }

    return (pfnLeft( hwnd, msg, mp1, mp2));
}

/****************************************************************************/

MRESULT _System RightTitleSubProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    ULONG       ctr;
    ULONG       ul;

    switch (msg)
    {
        case WM_MOUSEMOVE:
        {
            ctr = LASTRIGHTCOL;
            ul = InColOrMargin( hwnd, mp1, FIRSTRIGHTCOL, &ctr);
            if (ul == INNEITHER)
                break;

            if (ul == INMARGIN)
                WinSetPointer( HWND_DESKTOP, hSizePtr);
            else
                WinSetPointer( HWND_DESKTOP, hSortPtr);

            return ((MRESULT)TRUE);
        }

        case WM_BUTTON1DOWN:
        {
            ctr = LASTRIGHTCOL;
            if (InColOrMargin( hwnd, mp1, FIRSTRIGHTCOL, &ctr) != INMARGIN)
                break;

            pfnRight( hwnd, msg, mp1, mp2);
            ResizeColumn( hwnd, ctr);
            return ((MRESULT)TRUE);
        }

        case WM_BUTTON1CLICK:
        {
            ctr = LASTRIGHTCOL;
            if (InColOrMargin( hwnd, mp1, FIRSTRIGHTCOL, &ctr) == INCOLUMN)
                SetSortColumn( ctr);

            break;
        }
    }

    return (pfnRight( hwnd, msg, mp1, mp2));
}

/****************************************************************************/

ULONG       InColOrMargin( HWND hwnd, MPARAM mp1, ULONG first, PULONG plast)

{
    ULONG       ulRtn = INNEITHER;
    LONG        x = SHORT1FROMMP( mp1);
    RECTL       rcl;

do
{
    // establish a small margin at the top of the title windows
    // to separate their active areas from the menubar
    if (WinQueryWindowRect( hwnd, &rcl) &&
        rcl.yTop > 16 &&
        SHORT2FROMMP( mp1) >= rcl.yTop-4)
        break;

    // see how far the window has been scrolled to the right
    WinSendMsg( hCnr, CM_QUERYVIEWPORTRECT, (MP)&rcl,
                MPFROM2SHORT( CMA_WORKSPACE, (first == FIRSTRIGHTCOL)));

    // adjust x accordingly
    x += rcl.xLeft;

    // see if x lies within one of the columns or no more than half a
    // character's width to the right of it;  if so, identify whether
    // it's in the margin (right edge +/- half a character) or in the
    // body of the column;  return that info along with the column nbr
    while (first <= *plast)
    {
        if (x <= ci[first].lRight + lAvgLeft)
        {
            if (x >= ci[first].lRight - lAvgRight)
                ulRtn = INMARGIN;
            else
                ulRtn = INCOLUMN;
            *plast = first;
            break;
        }
        first++;
    }

} while (fFalse);

    return (ulRtn);
}

/****************************************************************************/

void        SetSortColumn( ULONG ulCol)

{
    // every change requires us to uncheck the currently checked
    // menu item in order to get the menu to repaint properly
    WinSendMsg( hmnuSort, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_SORTFIRST+ulSortCol, FALSE),
                MPFROM2SHORT( MIA_CHECKED, 0));

    // if the selected column is the current sort column, change the
    // sort direction;  otherwise, remove the sort indicator from the
    // previous sort column, save the new one, and set the direction
    // to ascending

    if (ulSortCol == ulCol)
        sSortSense = -sSortSense;
    else
    {
        if (ci[ulSortCol].flData & CFA_RIGHT)
            ci[ulSortCol].pszTitle[0] = ' ';
        else
            *(strchr( ci[ulSortCol].pszTitle, 0)-1) = ' ';

        if (ulCol != (ULONG)-1)
        {
            ulSortCol = ulCol;
            sSortSense = 1;
        }
    }

    WinSendMsg( hmnuSort, MM_SETITEMCHECKMARK,
                MPFROM2SHORT( IDM_SORTFIRST+ulSortCol, TRUE),
                (MP)(sSortSense > 0 ? hbmAsc : hbmDesc));

    // check the appropriate menu item
    WinSendMsg( hmnuSort, MM_SETITEMATTR,
                MPFROM2SHORT( IDM_SORTFIRST+ulSortCol, FALSE),
                MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED));

    // if the column is right-adjusted, replace the first character with
    // the sort indicator, otherwise replace the last character (in both
    // cases this is a space);  it's done this way to keep the column
    // title from shifting position as the indicator is added & removed
    if (ci[ulSortCol].flData & CFA_RIGHT)
        ci[ulSortCol].pszTitle[0] = (sSortSense > 0 ? chAsc : chDesc);
    else
        *(strchr( ci[ulSortCol].pszTitle, 0)-1) =
                                    (sSortSense > 0 ? chAsc : chDesc);

    // sort the records using the sort function for this column;
    // this will cause the entire window to be repainted so we
    // don't need to do anything to get the column titles updated
    WinSendMsg( hCnr, CM_SORTRECORD, (MP)ci[ulSortCol].pvSort, 0);

    return;
}

/****************************************************************************/

void        ResizeColumn( HWND hTitle, ULONG ulCol)

{
    LONG        lChange;
    TRACKINFO   ti;

do
{
    memset( &ti, 0, sizeof(ti));

    // see how far the window has been scrolled to the right
    WinSendMsg( hCnr, CM_QUERYVIEWPORTRECT, (MP)&ti.rclTrack,
                MPFROM2SHORT( CMA_WORKSPACE, (ulCol >= FIRSTRIGHTCOL)));
    lChange = ti.rclTrack.xLeft;

    // calculate tracking rectangle size & position;  it's 3 pixels wide,
    // centered on the column divider & runs from top to bottom of the cnr

    WinQueryWindowRect( hCnr, &ti.rclTrack);
    ti.rclTrack.xLeft = ci[ulCol].lRight - 1 - lChange;
    WinMapWindowPoints( hTitle, hCnr, (PPOINTL)&ti.rclTrack.xLeft, 1);
    ti.rclTrack.yBottom = 0;
    ti.rclTrack.xRight = ti.rclTrack.xLeft + 3;

    // calculate bounding rectangle size & position;  it's the full height
    // of the cnr and runs from 3 pixels to the right of the current column
    // out to the right edge of the current split window

    WinQueryWindowRect( hTitle, &ti.rclBoundary);
    if (ulCol == FIRSTLEFTCOL || ulCol == FIRSTRIGHTCOL)
        ti.rclBoundary.xLeft = 3 - lChange;
    else
        ti.rclBoundary.xLeft = ci[ulCol-1].lRight + 3 - lChange;
    if (ti.rclBoundary.xLeft < 0)
        ti.rclBoundary.xLeft = 0;
    WinMapWindowPoints( hTitle, hCnr, (PPOINTL)&ti.rclBoundary, 2);
    ti.rclBoundary.yBottom = 0;
    ti.rclBoundary.yTop = ti.rclTrack.yTop;

    // limit tracking rectangle's size
    ti.ptlMinTrackSize.x = 3;
    ti.ptlMinTrackSize.y = ti.rclTrack.yTop;
    ti.ptlMaxTrackSize.x = 3;
    ti.ptlMaxTrackSize.y = ti.rclTrack.yTop;

    // set border & flags
    ti.cxBorder = 3;
    ti.fs = TF_MOVE | TF_ALLINBOUNDARY;

    // save starting value as a negative
    lChange = -ti.rclTrack.xLeft;

    // do it, exit if cancelled
    if (WinTrackRect( hCnr, 0, &ti) == FALSE)
        break;

    // calc change in position, exit if no change
    lChange += ti.rclTrack.xLeft;
    if (lChange == 0)
        break;

    // update fieldinfo for current column, then adjust the extents
    // for this and the remaining columns in the current split window
    ci[ulCol].pfi->cxWidth += lChange;
    do
    {
        ci[ulCol].lRight += lChange;
    } while (ulCol != LASTLEFTCOL && ulCol++ != LASTRIGHTCOL);

    // force a redisplay using the new sizes
    WinSendMsg( hCnr, CM_INVALIDATEDETAILFIELDINFO, 0, 0);

} while (fFalse);

    return;
}

/****************************************************************************/

void        GetColumnWidths( void)

{
    LONG            wide;
    LONG            narrow;
    LONG            width;
    LONG            col;
    LONG            total;
    ULONG           ctr;

    // since this function is called when the font gets changed,
    // we have to calc the average character width each time
    GetAvgCharWidth();

    // the docs say the left & right columns in a window have narrower
    // margins that the other columns;  AFAICT, only the left col does
    wide = 3 * lAvg;
    narrow = (5 * lAvg) / 2;

    for (ctr=0; ctr < eCNTCOLS; ctr++)
    {
        width = (LONG)WinSendMsg( hCnr, CM_QUERYDETAILFIELDINFO,
                                  (MP)ci[ctr].pfi, (MP)CMA_DATAWIDTH);

        if (width == 0)
            col = 0;
        else
            if (ctr == FIRSTLEFTCOL || ctr == FIRSTRIGHTCOL)
                col = width + narrow;
            else
                col = width + wide;

        ci[ctr].pfi->cxWidth = width;

        if (ctr == FIRSTLEFTCOL || ctr == FIRSTRIGHTCOL)
            total = col;
        else
            total += col;
        ci[ctr].lRight = total;
    }

    WinSendMsg( hCnr, CM_INVALIDATEDETAILFIELDINFO, 0, 0);

    return;
}

/****************************************************************************/

LONG        GetAvgCharWidth( void)

{
    HPS         hps = 0;
    FONTMETRICS fm;

    // get the average character width for the current font
    fm.lAveCharWidth = 0;
    hps = WinGetPS( hCnr);
    if (hps && GpiQueryFontMetrics( hps,
               FIELDOFFSET( FONTMETRICS, lAveCharWidth)+sizeof(LONG), &fm))
        lAvg = fm.lAveCharWidth;
    else
        lAvg = 6;

    // if the size is an odd number, lAvgRight will be 1 pixel larger
    lAvgLeft = lAvg / 2;
    lAvgRight = lAvg - lAvgLeft;

    if (hps)
        WinReleasePS( hps);

    return (lAvg);
}

/****************************************************************************/

void        ResetColumnWidths( void)

{
    ULONG       ctr;

    // clear the fixed widths from the FIELDINFO structs
    for (ctr=0; ctr < eCNTCOLS; ctr++)
        ci[ctr].pfi->cxWidth = 0;

    // refresh all the records to recalculate the column widths
    WinSendMsg( hCnr, CM_INVALIDATERECORD, 0, 0);

    // get those widths, then reposition the splitbar
    GetColumnWidths();
    ResetSplitBar();

    return;
}

/****************************************************************************/

void        ResetSplitBar( void)

{
    CNRINFO         cnri;

    // set the splitbar to show 66% of the last left column (i.e. the title)
    cnri.xVertSplitbar =
        ((2 * ci[LASTLEFTCOL].lRight) + ci[LASTLEFTCOL-1].lRight) / 3;

    WinSendMsg( hCnr, CM_SETCNRINFO, (MP)&cnri, (MP)CMA_XVERTSPLITBAR);

    return;
}

/****************************************************************************/
/****************************************************************************/

void        RestoreSort( void)

{
    USHORT  ausSort[3];
    ULONG   ctr;
    char *  ptr;

    // if the ini entry is missing or defective, use default values
    if (PrfQueryProfileSize( HINI_USERPROFILE, "FPOS",
                             "SORT", &ctr) == FALSE ||
        ctr != sizeof( ausSort) ||
        PrfQueryProfileData( HINI_USERPROFILE, "FPOS",
                             "SORT", ausSort, &ctr) == FALSE)
    {
        ausSort[0] = eTITLE;
        ausSort[1] = FALSE;
        ausSort[2] = FALSE;
    }

    // set the sort column, sort sense, and sort indicators
    ulSortCol = (ausSort[0] >= eCNTCOLS ? eTITLE : ausSort[0]);
    sSortSense = (ausSort[1] ? -1 : 1);
    SetSortIndicators( ausSort[2]);

    // if the ini entry is present & valid, parse the buffer
    // and construct an array of PSZs
    if (PrfQueryProfileSize( HINI_USERPROFILE, "FPOS",
                             "VIEWSORT", &ctr) == FALSE ||
        ctr >= sizeof( achViewBuf) ||
        PrfQueryProfileData( HINI_USERPROFILE, "FPOS",
                             "VIEWSORT", achViewBuf, &ctr) == FALSE)
        ctr = 0;
    else
        for (ptr=achViewBuf, ctr=0; *ptr && ctr < VIEWCNT; ctr++)
        {
            apszView[ctr] = ptr;
            ptr = strchr( ptr, 0) + 1;
        }

    // if we didn't end up with the correct number of entries,
    // copy in the default array
    if (ctr != VIEWCNT)
        memcpy( apszView, apszDefView, sizeof( apszView));

    // update the sort info without changing the sort column or direction
    SetSortColumn( (ULONG)-1);

    return;
}

/****************************************************************************/

void        StoreSort( void)

{
    SHORT   asSort[3];
    ULONG   ctr;
    char *  ptr;
    char    szText[128];

    // if these values are all set to their defaults, eliminate any
    // existing ini entry;  otherwise, construct the entry & save it
    if (ulSortCol == eTITLE && sSortSense == 1 && chAsc == 0x1F)
        PrfWriteProfileData( HINI_USERPROFILE, "FPOS", "SORT", 0, 0);
    else
    {
        asSort[0] = (SHORT)ulSortCol;
        asSort[1] = (sSortSense < 0 ? TRUE : FALSE);
        asSort[2] = (chAsc == 0x1F ? FALSE : TRUE);
        PrfWriteProfileData( HINI_USERPROFILE, "FPOS", "SORT",
                             asSort, sizeof(asSort));
    }

    // see if the current view sort order matches the default
    for (ctr=0; ctr < VIEWCNT; ctr++)
        if (*(apszDefView[ctr]) != *(apszView[ctr]))
            break;

    // if current == default, remove any existing ini entry;
    // otherwise, construct a list of strings and save them
    if (ctr == VIEWCNT)
        PrfWriteProfileData( HINI_USERPROFILE, "FPOS", "VIEWSORT", 0, 0);
    else
    {
        for (ctr=0, ptr=szText; ctr < VIEWCNT; ctr++)
        {
            strcpy( ptr, apszView[ctr]);
            ptr = strchr( ptr, 0) + 1;
        }

        *ptr++ = 0;
        PrfWriteProfileData( HINI_USERPROFILE, "FPOS", "VIEWSORT",
                             szText, ptr - szText);
    }

    return;
}

/****************************************************************************/

void        SetSortIndicators( BOOL fLiteral)

{
    if (fLiteral)
    {
        chAsc = 0x1E;
        chDesc = 0x1F;
        hbmAsc = hUpBmp;
        hbmDesc = hDownBmp;
    }
    else
    {
        chAsc = 0x1F;
        chDesc = 0x1E;
        hbmAsc = hDownBmp;
        hbmDesc = hUpBmp;
    }

    return;
}

/****************************************************************************/

BOOL        IsSortIndicatorLiteral( void)

{
    return (chAsc == 0x1E);
}

/****************************************************************************/
/****************************************************************************/

SHORT   _System SortByDel( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    SHORT       sRtn;

    // title, path, view - result is already multiplied by sSortSense
    if (((PULONG)&p1[1])[eDEL] == ((PULONG)&p2[1])[eDEL])
        sRtn = SortByTitle( p1, p2, pv);
    else
    if (((PULONG)&p1[1])[eDEL] == hDelIco)
        sRtn = sSortSense;
    else
        sRtn = -sSortSense;

    return (sRtn);
}

/****************************************************************************/

SHORT   _System SortByNbr( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    // nbr
    return ((SHORT)(((PLONG)&p1[1])[eNBR] - ((PLONG)&p2[1])[eNBR]) * sSortSense);
}

/****************************************************************************/

SHORT   _System SortByKey( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    // key
    return ((SHORT)strcmpi( ((char **)&p1[1])[eKEY],
                            ((char **)&p2[1])[eKEY]) * sSortSense);
}

/****************************************************************************/

SHORT   _System SortBySize( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    SHORT       sRtn;

    // size
    sRtn = (SHORT)(((PLONG)&p1[1])[eSIZE] - ((PLONG)&p2[1])[eSIZE]) * sSortSense;

    // view, title, path - result already multiplied by sSortSense
    if (sRtn == 0)
        sRtn = SortByView( p1, p2, pv);

    return (sRtn);
}

/****************************************************************************/

SHORT   _System SortByPath( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    SHORT       sRtn;
    SHORT       s1;
    SHORT       s2;
    char        ch;

do
{
    // path
    sRtn = (SHORT)strcmpi( ((char **)&p1[1])[ePATH],
                           ((char **)&p2[1])[ePATH]);
    if (sRtn)
        break;

    // view
    ch = *(((char **)&p1[1])[eVIEW]);
    for (s1 = 0; s1 < VIEWCNT; s1++)
        if (ch == *(apszView[s1]))
            break;

    ch = *(((char **)&p2[1])[eVIEW]);
    for (s2 = 0; s2 < VIEWCNT; s2++)
        if (ch == *(apszView[s2]))
            break;

    sRtn = s1 - s2;
    if (sRtn)
        break;

    sRtn = (SHORT)strcmp( ((char **)&p1[1])[eVIEW],
                          ((char **)&p2[1])[eVIEW]);

} while (fFalse);

    return (sRtn * sSortSense);
}

/****************************************************************************/

SHORT   _System SortByTitle( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    SHORT       sRtn;
    SHORT       s1;
    SHORT       s2;
    char        ch;

do
{
    // title
    sRtn = (SHORT)strcmpi( ((char **)&p1[1])[eTITLE],
                           ((char **)&p2[1])[eTITLE]);
    if (sRtn)
        break;

    // path
    sRtn = (SHORT)strcmpi( ((char **)&p1[1])[ePATH],
                           ((char **)&p2[1])[ePATH]);
    if (sRtn)
        break;

    // view
    ch = *(((char **)&p1[1])[eVIEW]);
    for (s1 = 0; s1 < VIEWCNT; s1++)
        if (ch == *(apszView[s1]))
            break;

    ch = *(((char **)&p2[1])[eVIEW]);
    for (s2 = 0; s2 < VIEWCNT; s2++)
        if (ch == *(apszView[s2]))
            break;

    sRtn = s1 - s2;
    if (sRtn)
        break;

    sRtn = (SHORT)strcmp( ((char **)&p1[1])[eVIEW],
                          ((char **)&p2[1])[eVIEW]);

} while (fFalse);

    return (sRtn * sSortSense);
}

/****************************************************************************/

SHORT   _System SortByView( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID pv)

{
    SHORT       sRtn;
    SHORT       s1;
    SHORT       s2;
    char        ch;

do
{
    // view - sort order is determined by the leading
    // character's position in the sort table
    ch = *(((char **)&p1[1])[eVIEW]);
    for (s1 = 0; s1 < VIEWCNT; s1++)
        if (ch == *(apszView[s1]))
            break;

    ch = *(((char **)&p2[1])[eVIEW]);
    for (s2 = 0; s2 < VIEWCNT; s2++)
        if (ch == *(apszView[s2]))
            break;

    sRtn = s1 - s2;
    if (sRtn)
        break;

    // if the leading characters match, see if their
    // entire names match (e.g. Details vs. Details-1)
    sRtn = (SHORT)strcmp( ((char **)&p1[1])[eVIEW],
                          ((char **)&p2[1])[eVIEW]);
    if (sRtn)
        break;

    // title
    sRtn = (SHORT)strcmpi( ((char **)&p1[1])[eTITLE],
                           ((char **)&p2[1])[eTITLE]);
    if (sRtn)
        break;

    // path
    sRtn = (SHORT)strcmpi( ((char **)&p1[1])[ePATH],
                           ((char **)&p2[1])[ePATH]);

} while (fFalse);

    return (sRtn * sSortSense);
}

/****************************************************************************/

