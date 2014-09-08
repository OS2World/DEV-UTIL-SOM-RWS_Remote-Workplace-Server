/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// ICNZ.H
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

#define INCL_DOS
#define INCL_PM
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ICNZRC.H"
#include "..\RWS.H"
#include "..\RWSERR.H"

/****************************************************************************/

#define ERRMSG(x)       {pErr = (x); break;}

// my favorite macro
#define MP              MPARAM

#define CFA_ICO         (CFA_BITMAPORICON  | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_CENTER)
#define CFA_NBR         (CFA_ULONG  | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_RIGHT)
#define CFA_STR         (CFA_STRING | CFA_HORZSEPARATOR | CFA_SEPARATOR | CFA_LEFT)

#define CFA_ALIGNMASK   (CFA_LEFT | CFA_CENTER | CFA_RIGHT)
#define CFA_LEFTRIGHT   (CFA_LEFT | CFA_RIGHT)

enum    eCols           { eCUST, eDEF, eTITLE, eSIZE, eKEY, ePATH, eCNTCOLS};

#define FIRSTLEFTCOL    eCUST
#define LASTLEFTCOL     eTITLE
#define FIRSTRIGHTCOL   eSIZE
#define LASTRIGHTCOL    ePATH

#define INNEITHER       0
#define INCOLUMN        1
#define INMARGIN        2

// undocumented menu msg - thanks to Martin LaFaix
#define MM_SETITEMCHECKMARK 0x0210

// flag used to retrieve the width of a column's data -
// documented in PM Ref->Container Controls->Container Views->
// Details View->Determining the Width of a Column in Details View
#define CMA_DATAWIDTH       0x0200

/****************************************************************************/

typedef struct _COLINFO {
    ULONG               flData;         // data flags
    ULONG               ulExtra;        // extra bytes for strings
    PSZ                 pszTitle;       // column title
    PFIELDINFO          pfi;            // fieldinfo for this column
    LONG                lRight;         // right edge of column
    PVOID               pvSort;         // sort function for this column
} COLINFO;

/****************************************************************************/

// in ICNZMAIN.C
ULONG           GetData( void);
void            UpdateStatus( char * pszMsg);
void            EmptyCnr( void);

// in ICNZCOL.C
BOOL            SubclassColumnHeadings( void);
void            RestoreSort( void);
void            StoreSort( void);
void            GetColumnWidths( void);
void            SetSortColumn( ULONG ulCol);
void        	ResetColumnWidths( void);
void        	ResetSplitBar( void);
void            SetSortIndicators( BOOL fLiteral);
BOOL            IsSortIndicatorLiteral( void);

// in ICNZCMD.C
void            Command( HWND hwnd, ULONG ulCmd);
void            PopupMenu( HWND hwnd, PMINIRECORDCORE pRec);
BOOL            SaveChanges( char * pszTitle, BOOL fExit);

/****************************************************************************/

