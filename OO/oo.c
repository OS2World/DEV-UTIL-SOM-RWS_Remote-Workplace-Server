/****************************************************************************/
//
// oo.c
//
// a WPS commandline tool - powered by Remote Workplace Server v0.80
//
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
 * The Original Code is 'oo - a WPS commandline tool'
 *
 * The Initial Developer of the Original Code is Richard L. Walsh.
 * 
 * Portions created by the Initial Developer are Copyright (C) 2007
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
#include "..\RWS.H"

/****************************************************************************/

void        Morph( void);
BOOL        ParseArgs( int argc, char * argv[]);
int         ParseExe( char * pArg);
int         ParseOption( char * pArg);
BOOL        ParseObject( char * pArg, char * pPath, PULONG pType);
BOOL        ParseHandle( char * pArg, char * pPath, PULONG pType);
BOOL        ParseView( char * pArg);
BOOL        UppercaseArg( char * pArg);
BOOL        CallRws( void);
ULONG       ListObjectsCmd( ULONG type, ULONG value);
void        ListOneObject( ULONG ulObj);
ULONG       ShowInfoCmd( ULONG type, ULONG value);
void        ErrMsg( char * msg, ULONG rc);
void        ShowHelp( void);

/****************************************************************************/

#define MP              MPARAM
#define ERRBRK(x)       {pErr = x; break;}

typedef struct _OOCMD
{
    char    letter;
    char    opts[3];
    ULONG   flags;
    ULONG   cmd;
    ULONG   minArg;
    char *  pszHelp;
} OOCMD;

#define OOF_LINK        1
#define OOF_HANDLE      2
#define OOF_FORCEDEL    4

typedef enum OO_CMD {
    cmd_Assign, cmd_Copy, cmd_Delete, cmd_Goto, cmd_Help, cmd_Info,
    cmd_List, cmd_Move, cmd_New, cmd_Open, cmd_Popup, cmd_Shadow,
    cmd_Using, cmd_Qmark, cmd_Lock, cmd_Unlock, cmd_Cnt };

/****************************************************************************/

char    szHelpHead[] =
"  %s  --a WPS commandline tool v1.10  --  (C)Copyright 2007 RL Walsh--\n"
"      --features & options not listed here are described in 'oo.txt'--\n\n";

char    szHelpOpen[] =
"Open       %s  %-6s  object [view]    open object in selected view\n";

char    szHelpGoto[] =
"GoTo       %s  %-6s  object           go to object & highlight it\n";

char    szHelpPopup[] =
"PopupMenu  %s  %-6s  object           popup object's WPS menu\n";

char    szHelpInfo[] =
"Info       %s  %-6s  object           info about an object\n";

char    szHelpList[] =
"List       %s  %-6s  folder           list non-file objects in folder\n";

char    szHelpAssign[] =
"Assign     %s  %-6s  object string    assign setup string\n";

char    szHelpUsing[] =
"OpenUsing  %s  %-6s  file program     open file using program object\n";

char    szHelpDelete[] =
"Delete     %s  %-6s  object [Force]   delete object ('f' to force)\n";

char    szHelpCopy[] =
"Copy       %s  %-6s  object folder    copy object to folder\n";

char    szHelpMove[] =
"Move       %s  %-6s  object folder    move object to folder\n";

char    szHelpShadow[] =
"Shadow     %s  %-6s  object folder    put shadow of object in folder\n";

char    szHelpNew[] =
"New        %s  %-6s  class [title folder setup option]  new object\n";

char    szHelpBody[] =
"options - [o] if a shadow, use the original  - [h] show handles only\n"
"\n"
"object  - the full path & name of any WPS object, file, or directory\n"
"        - the name of any file or directory found on the PATH or OOPATH\n"
"        - a WPS object ID   - a WPS object's handle (in decimal or hex)\n"
"Use SET OOPATH= to add other directories to the search path\n"
"\n"
"view    - [I]con, [T]ree, [D]etails (folders)  - [S]ettings (any object)\n"
"        - [R]unning (programs & data files)";

int     aiHelp[] = { cmd_Open, cmd_Goto, cmd_Popup, cmd_Info,
                     cmd_List, cmd_Assign, cmd_Using, cmd_Delete,
                     cmd_Copy, cmd_Move, cmd_Shadow,  cmd_New};

#define HELP_CNT    (sizeof(aiHelp) / sizeof(int))

/****************************************************************************/

// lots of global variables - for this kind of program, why not?

char        szCopyright[] = "oo v1.10 - (C)Copyright 2007  R.L.Walsh";
BOOL        fFalse = FALSE;

int         gCmd = 0;
int         gDef = 0;

char *      pszErr          = "\?";
char *      pszRwsErr       = "RWS Error";

OOCMD   cmds[] = {
    { 'A',  "S",  OOF_LINK,  cmd_Assign,        2,  szHelpAssign},
    { 'C',  "O",  0,         cmd_Copy,          2,  szHelpCopy},
    { 'D',  "O",  0,         cmd_Delete,        1,  szHelpDelete},
    { 'G',  "O",  0,         cmd_Goto,          1,  szHelpGoto},
    { 'H',  "",   0,         cmd_Help,          0,  0},
    { 'I',  "OH", 0,         cmd_Info,          1,  szHelpInfo},
    { 'L',  "H",  OOF_LINK,  cmd_List,          1,  szHelpList},
    { 'M',  "O",  0,         cmd_Move,          2,  szHelpMove},
    { 'N',  "",   0,         cmd_New,           1,  szHelpNew},
    { 'O',  "",   OOF_LINK,  cmd_Open,          1,  szHelpOpen},
    { 'P',  "O",  0,         cmd_Popup,         1,  szHelpPopup},
    { 'S',  "",   OOF_LINK,  cmd_Shadow,        2,  szHelpShadow},
    { 'U',  "",   OOF_LINK,  cmd_Using,         2,  szHelpUsing},
    {'\?',  "",   0,         cmd_Qmark,         0,  0},
    { '+',  "",   OOF_LINK,  cmd_Lock,          0,  0},
    { '-',  "",   OOF_LINK,  cmd_Unlock,        0,  0},
};

/****************************************************************************/

ULONG   rwsView = 0;
ULONG   rwsType = 0;
ULONG   rwsType2 = 0;
ULONG   typeTitle = 0;
ULONG   typeHndl = 0;

char *  pszClass = 0;
char *  pszTitle = 0;
char *  pszSetup = 0;
char *  pszOption = 0;

char    szPath[CCHMAXPATH];
char    szPath2[CCHMAXPATH];

/****************************************************************************/
/****************************************************************************/

int         main( int argc, char * argv[])

{
    BOOL    fInit = FALSE;
    HAB     hab = 0;
    HMQ     hmq = 0;
    ULONG   rc;
    int     nRtn = 16;

do
{
    // avoid compatibility problems
    // RWSFULLVERSION for oo v1.1 GA is 0x08000100
    if (RwsQueryVersion( 0) < RWSFULLVERSION) {
        ErrMsg( "Please use a newer version of the RWS08 dlls", 0);
        break;
    }

    // see if the commandline args are valid
    if (!ParseArgs( argc, argv)) {
        nRtn = 4;
        break;
    }

    // show help
    if (gCmd == cmd_Help) {
        ShowHelp();
        nRtn = 2;
        break;
    }

    // change this into a PM process, then init PM
    Morph();

    hab = WinInitialize( 0);
    if (!hab)
        break;

    hmq = WinCreateMsgQueue( hab, 0);
    if (!hmq)
        break;

    // init RWS08
    rc = RwsClientInit( TRUE);
    if (rc) {
        ErrMsg( "RwsClientInit failed", rc);
        break;
    }
    fInit = TRUE;

    // pass a command to the WPS
    if (CallRws())
        nRtn = 0;
    else
        nRtn = 8;

} while (fFalse);

    // clean up
    if (fInit)
        RwsClientTerminate();
    if (hmq)
        WinDestroyMsgQueue( hmq);
    if (hab)
        WinTerminate( hab);

    return (nRtn);
}

/****************************************************************************/

// enable PM even if started as a VIO app

void        Morph( void)

{
    PPIB    ppib;
    PTIB    ptib;

    DosGetInfoBlocks( &ptib, &ppib);
    ppib->pib_ultype = 3;
    return;
}

/****************************************************************************/

BOOL        ParseArgs( int argc, char * argv[])

{
    BOOL        fRtn = FALSE;
    int         ctr;
    char *      pArg;
    char *      ptr;
    char *      pErr = 0;

do {
    // determine the default command from the exe name
    gCmd = gDef = ParseExe( argv[0]);

    // if there are no args, display help
    if (argc < 2) {
        gCmd = cmd_Help;
        fRtn = TRUE;
        break;
    }

    // trim leading & trailing whitespace from args
    for (ctr = 1; ctr < argc; ctr++) {
        argv[ctr] += strspn( argv[ctr], " \t");
        if (!*(argv[ctr]))
            break;

        ptr = strchr( argv[ctr], 0) - 1;
        while (*ptr == ' ' || *ptr == '\t')
            ptr--;
        ptr[1] = 0;
    }

    // this could happen if you enter "  " as an arg
    if (ctr < argc)
        ERRBRK( "empty parameter")

    // point at first arg
    ctr = 1;
    pArg = argv[ctr];

    // parse switch, if any - this will set the command & options
    if (*pArg == '/' || *pArg == '-') {
        gCmd = ParseOption( pArg);
        if (gCmd < 0)
            ERRBRK( "invalid command")

        ctr++;
    }

    // if RWS is given a shadow, should it find the original object?
    if (cmds[gCmd].flags & OOF_LINK) {
        typeTitle = RWSI_SFTITLE;
        typeHndl = RWSI_SHNDL;
    }
    else {
        typeTitle = RWSI_OFTITLE;
        typeHndl = RWSI_OHNDL;
    }

    switch (gCmd) {

        case cmd_Assign:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            fRtn = ParseObject( argv[ctr], szPath, &rwsType);
            if (fRtn)
                pszSetup = argv[ctr + 1];

            break;

        case cmd_Copy:
        case cmd_Move:
        case cmd_Shadow:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            if (ParseObject( argv[ctr], szPath, &rwsType))
                fRtn = ParseObject( argv[ctr + 1], szPath2, &rwsType2);

            break;

        case cmd_Delete:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            if (!(fRtn = ParseObject( argv[ctr], szPath, &rwsType)))
                break;

            if (++ctr < argc) {
                UppercaseArg( argv[ctr]);
                if (*argv[ctr] == 'F')
                    cmds[gCmd].flags |= OOF_FORCEDEL;
                else
                    pErr = "invalid Delete option ignored";
            }
            break;

        case cmd_Info:
        case cmd_List:
        case cmd_Popup:
        case cmd_Goto:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            fRtn = ParseObject( argv[ctr], szPath, &rwsType);
            break;

        case cmd_Open:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            if (!(fRtn = ParseObject( argv[ctr], szPath, &rwsType)))
                break;

            rwsView = 0;
            if (++ctr < argc)
                ParseView( argv[ctr]);
            break;

        case cmd_Using:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            if (ParseObject( argv[ctr], szPath, &rwsType))
                fRtn = ParseObject( argv[ctr + 1], szPath2, &rwsType2);
            break;

        case cmd_New:
            if (ctr + cmds[gCmd].minArg > argc)
                ERRBRK( "missing parameter(s)")

            pszClass = argv[ctr];

            if (++ctr >= argc)
                pszTitle = pszClass;
            else
                pszTitle = argv[ctr];

            if (++ctr >= argc) {
                WinQueryActiveDesktopPathname( szPath, sizeof(szPath));
                rwsType = RWSI_OPATH;
            }
            else
                if (!ParseObject( argv[ctr], szPath, &rwsType))
                    break;

            if (++ctr >= argc)
                pszSetup = "";
            else
                pszSetup = argv[ctr];

            if (++ctr >= argc)
                pszOption = "";
            else
                pszOption = argv[ctr];

            fRtn = TRUE;
            break;

        case cmd_Lock:
            pszClass = RWSCLASSNAME;
            pszTitle = pszClass;
            pszSetup = "";
            pszOption = "";

            if (ctr < argc) {
                if (!ParseObject( argv[ctr], szPath, &rwsType))
                    break;
            }
            else {
                PTIB    ptib;
                PPIB    ppib;

                DosGetInfoBlocks( &ptib, &ppib);
                DosQueryModuleName( ppib->pib_hmte, sizeof(szPath), szPath);
                ptr = strrchr( szPath, '\\');
                if (ptr)
                    *ptr = 0;
                else
                    WinQueryActiveDesktopPathname( szPath, sizeof(szPath));
                rwsType = RWSI_OPATH;
            }                

            gCmd = cmd_New;
            fRtn = TRUE;
            break;

        case cmd_Unlock: {
            if (ctr < argc) {
                if (!ParseObject( argv[ctr], szPath, &rwsType))
                    break;
                if (rwsType != RWSI_OPATH)
                    ERRBRK( "fully-qualified folder name required")
            }
            else {
                PTIB    ptib;
                PPIB    ppib;

                DosGetInfoBlocks( &ptib, &ppib);
                DosQueryModuleName( ppib->pib_hmte, sizeof(szPath), szPath);
                ptr = strrchr( szPath, '\\');
                if (ptr)
                    *ptr = 0;
                else
                    WinQueryActiveDesktopPathname( szPath, sizeof(szPath));
            }

            ptr = strchr( szPath, 0);
            if (ptr[-1] != '\\')
                *ptr++ = '\\';
            strcpy( ptr, RWSCLASSNAME);
            rwsType = RWSI_SFTITLE;

            gCmd = cmd_Delete;
            cmds[gCmd].flags |= OOF_FORCEDEL;
            fRtn = TRUE;
            break;
        }

        case cmd_Help:
        case cmd_Qmark:
            gCmd = cmd_Help;
            fRtn = TRUE;
            break;

        default:
            ERRBRK( "logic error 1")
    }

} while (fFalse);

    if (pErr)
        ErrMsg( pErr, 0);

    return (fRtn);
}

/****************************************************************************/

int         ParseExe( char * pArg)

{
    int     iRtn = cmd_Open;
    int     ctr;
    char *  ptr;

do {
    // just in case
    if (!pArg || !*pArg)
        break;

    // point at the first character of the exe's name
    ptr = strrchr( pArg, '\\');
    if (ptr)
        pArg = ptr + 1;
    else
        pArg += strspn( pArg, " \t");

    ptr = strchr( pArg, '.');
    if (ptr)
        *ptr = 0;

    UppercaseArg( pArg);

    // the name of the exe controls the default command;  if renamed,
    // the exe's name has to be 2 characters:  the first must be an 'o';
    // the 2nd must be one of the switch characters

    if (pArg[0] != 'O' || !pArg[1] || pArg[2] != 0)
        break;

    pArg++;

    for (ctr = 0; ctr < cmd_Qmark; ctr++)
        if (cmds[ctr].letter == *pArg) {
            iRtn = ctr;
            break;
        }

} while (fFalse);

    return (iRtn);
}

/****************************************************************************/

int         ParseOption( char * pArg)

{
    int     iRtn = -1;
    int     ctr;
    BOOL    fLink = FALSE;
    BOOL    fInvalid = FALSE;
    char *  ptr;

do {
    pArg++;
    UppercaseArg( pArg);

    for (ctr = 0; ctr < cmd_Cnt; ctr++)
        if (cmds[ctr].letter == *pArg) {
            iRtn = ctr;
            break;
        }

    if (ctr >= cmd_Cnt)
        break;

    // decode valid options, signal invalid opts
    for (pArg++, ctr = 0; *pArg && ctr < 2; pArg++, ctr++) {
        ptr = strchr( cmds[iRtn].opts, *pArg);
        if (!ptr)
            fInvalid = TRUE;
        else
        if (*ptr == 'O' || *ptr == 'S')
            fLink = TRUE;
        else
        if (*ptr == 'H')
            cmds[iRtn].flags |= OOF_HANDLE;
    }

    // this avoids toggling the link flag more than once
    if (fLink)
        cmds[iRtn].flags ^= OOF_LINK;

    if (fInvalid)
        ErrMsg( "invalid option(s) ignored", 0);

} while (fFalse);

    return (iRtn);
}

/****************************************************************************/

BOOL        ParseObject( char * pArg, char * pPath, PULONG pType)

{
    BOOL        fRtn = FALSE;
    char *      ptr;
    char *      pErr = 0;
    FILESTATUS3 fs3;

do {
    // object ID
    if (*pArg == '<') {
        ptr = strchr( pArg, 0);
        if (ptr[-1] != '>')
            ERRBRK( "malformed object ID")
        strcpy( pPath, pArg);
        *pType = typeTitle;
        fRtn = TRUE;
        break;
    }

    // possibly a handle
    if (*pArg >= '0' && *pArg <= '9') {
        fRtn = ParseHandle( pArg, pPath, pType);
        if (fRtn)
            break;
    }

    // no wildcards
    if (strchr( pArg, '*') || strchr( pArg, '\?'))
        ERRBRK( "'*' and '\?' are not supported")

    // is this an unqualified name?
    ptr = strchr( pArg, '\\');
    if (!ptr)
        ptr = strchr( pArg, '/');

    // if unqualified, search the OOPATH & PATH environment variables
    if (!ptr) {
        if (DosSearchPath( 7, "OOPATH", pArg, pPath, CCHMAXPATH) &&
            DosSearchPath( 7, "PATH", pArg, pPath, CCHMAXPATH))
            ERRBRK( "PATH search failed")
        *pType = RWSI_OPATH;
        fRtn = TRUE;
        break;
    }

    // if an explicit search path was specified (as indicated by the
    // presence of a semicolon), search it;  if this fails, continue
    ptr = strrchr( pArg, ';');
    if (ptr && ptr[1] != 0 && !strchr( ptr, '\\') && !strchr( pArg, '/')) {
        *ptr = 0;
        if (!DosSearchPath( 5, pArg, &ptr[1], pPath, CCHMAXPATH)) {
            *pType = RWSI_OPATH;
            fRtn = TRUE;
            break;
        }
        *ptr = ';';
    }

    // remove unneeded trailing (back)slash
    ptr = strchr( pArg, 0) - 1;
    if (ptr > pArg && (*ptr == '\\' || *ptr == '/') && ptr[-1] != ':')
        *ptr = 0;

    // expand partially-qualified names
    if (DosQueryPathInfo( pArg, FIL_QUERYFULLNAME, pPath, CCHMAXPATH))
        ERRBRK( "invalid or malformed path")

    // if we actually find something, then this is a file or directory;
    // otherwise, assume this is an abstract object's title
    if (DosQueryPathInfo( pPath, FIL_STANDARD, &fs3, sizeof(fs3)))
        *pType = typeTitle;
    else
        *pType = RWSI_OPATH;

    fRtn = TRUE;

} while (fFalse);

    if (pErr)
        ErrMsg( pErr, 0);

    return (fRtn);
}

/****************************************************************************/

BOOL        ParseHandle( char * pArg, char * pPath, PULONG pType)

{
    BOOL        fRtn = FALSE;
    BOOL        fHex = FALSE;
    ULONG       cnt;
    ULONG       ctr;
    char *      ptr;

do {
    // eliminate leading zeros
    while (*pArg && *pArg == '0')
        pArg++;

    if (!*pArg)
        break;

    // a leading '0x' is helpful but not necessary
    if (*pArg == 'x' || *pArg == 'X') {
        fHex = TRUE;
        pArg++;
    }

    // handles are either 5 hex digits or 5/6 decimal digits
    cnt = strlen( pArg);
    if (cnt < 5 || cnt > 6)
        break;

    // confirm all characters are numeric
    for (ptr = pArg, ctr = 0; ctr < cnt; ptr++, ctr++) {
        if (*ptr >= '0' && *ptr <= '9')
            continue;

        if ((*ptr >= 'A' && *ptr <= 'F') ||
            (*ptr >= 'a' && *ptr <= 'f')) {
            fHex = TRUE;
            continue;
        }

        break;
    }
    if (ctr < cnt)
        break;

    // all hex handles are 5 digits & start with a 1, 2, or 3
    // (5-digit decimal handles would have to start with 6, 7, 8, or 9)
    if (!fHex && cnt == 5 && *pArg >= '1' && *pArg <= '3')
        fHex = TRUE;

    // convert the string to number, then see if it's within range
    cnt = strtoul( pArg, 0, ((fHex) ? 16 : 10));
    if (!cnt || cnt <= 0x10000 || cnt >= 0x3ffff)
        break;

    // for no good reason, use the path field to store the handle
    *((PULONG)pPath) = cnt;
    *pType = typeHndl;

    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

BOOL        ParseView( char * pArg)

{
    UppercaseArg( pArg);

    if (!strcmp( pArg, "I") || !strcmp( pArg, "ICON"))
        rwsView = 1;
    else
    if (!strcmp( pArg, "D") || !strcmp( pArg, "DETAILS"))
        rwsView = 102;
    else
    if (!strcmp( pArg, "T") || !strcmp( pArg, "TREE"))
        rwsView = 101;
    else
    if (!strcmp( pArg, "S") || !strcmp( pArg, "SETTINGS"))
        rwsView = 2;
    else
    if (!strcmp( pArg, "R") || !strcmp( pArg, "RUNNING"))
        rwsView = 4;
    else
    if (!strcmp( pArg, "X") || !strcmp( pArg, "TEXTVIEW"))
        rwsView = 46914;
    else
    if (!strcmp( pArg, "U") || !strcmp( pArg, "USER"))
        rwsView = 25856;
    else
    if (!strcmp( pArg, "U1") || !strcmp( pArg, "USER+1"))
        rwsView = 25857;
    else
        ErrMsg( "unknown view ignored - opening default view", 0);

    return (TRUE);
}

/****************************************************************************/

BOOL        UppercaseArg( char * pArg)

{
    BOOL        fRtn = TRUE;
    COUNTRYCODE cc = {0,0};

    // since PM hasn't been initialized when this is called,
    // we shouldn't use WinUpper() - even though it works
    if (DosMapCase( strlen( pArg), &cc, pArg))
        fRtn = FALSE;

    return (fRtn);
}

/****************************************************************************/

BOOL        CallRws( void)

{
    ULONG       ulVal;
    PRWSHDR     pHdr = 0;
    ULONG   	rc = 0;

do {
    // if szPath contains a handle, put it ulVal; otherwise, put its
    // address there (all values passed to RWS are cast to ULONG)
    if (rwsType == typeHndl)
        ulVal = *((PULONG)szPath);
    else
        ulVal = (ULONG)szPath;

    switch (gCmd) {

      case cmd_Assign: {
        rc = RwsCall( &pHdr, RWSP_MNAM, (ULONG)"wpSetup",   // call type
                             RWSR_ASIS, 0,                  // return type
                             2,                             // nbr of args
                             rwsType,   0, ulVal,           // object
                             RWSI_PSTR, 0, pszSetup);       // setup string
        if (rc)
            break;

        RwsFreeMem( pHdr);
        pHdr = 0;

        rc = RwsCall( &pHdr, RWSP_MNAM, (ULONG)"wpSaveDeferred", // call type
                             RWSR_ASIS, 0,                  // return type
                             1,                             // nbr of args
                             rwsType,   0, ulVal);          // object
        break;
      }

      case cmd_Copy:
      case cmd_Shadow: {
        ULONG       ulVal2;
        ULONG       ulMethod;
        ULONG       ulHndl;

        if (gCmd == cmd_Shadow)
            ulMethod = (ULONG)"wpCreateShadowObject";
        else
            ulMethod = (ULONG)"wpCopyObject";

        if (rwsType2 == typeHndl)
            ulVal2 = *((PULONG)szPath2);
        else
            ulVal2 = (ULONG)szPath2;

        // ensure the folder gets converted from a shadow to an original
        if (rwsType2 == RWSI_OHNDL)
            rwsType2 = RWSI_SHNDL;
        else
        if (rwsType2 == RWSI_OFTITLE)
            rwsType2 = RWSI_SFTITLE;

        rc = RwsCall( &pHdr, RWSP_MNAM,  ulMethod,          // call type
                             RWSR_OHNDL, 0,                 // return type
                             3,                             // nbr of args
                             rwsType,    0, ulVal,          // object
                             rwsType2,   0, ulVal2,         // folder
                             RWSI_ASIS,  0, 0);             // lock
        if (rc)
            break;

        ulHndl = RwsGetResult( pHdr, 0, 0);
        printf( "%05X\n", ulHndl);
        break;
      }

      case cmd_Delete: {
        rc = RwsCall( &pHdr, RWSP_CMD,  RWSCMD_DELETE,  // call type
                             RWSR_ASIS, 0,              // return type
                             2,                         // nbr of args
                             rwsType,   0, ulVal,       // object
                             RWSI_ASIS, 0, (cmds[gCmd].flags & OOF_FORCEDEL));
                                                        // force delete flag
        break;
      }

      case cmd_Goto: {
        HWND        hwnd;

        rc = RwsCall( &pHdr, RWSP_CMD,  RWSCMD_LOCATE,  // call type
                             RWSR_ASIS, 0,              // return type
                             1,                         // nbr of args
                             rwsType, 0, ulVal);        // object
        if (rc)
            break;

        // the WPS may return an hwnd or a task handle;
        // if this appears to be an hwnd, bring the window to the top
        hwnd = RwsGetResult( pHdr, 0, 0);
        if (hwnd != (ULONG)-1 && (hwnd & 0x80000000))
            WinSetActiveWindow( HWND_DESKTOP, hwnd);

        break;
      }

      case cmd_Info: {
        rc = ShowInfoCmd( rwsType, ulVal);
        break;
      }

      case cmd_List: {
        rc = ListObjectsCmd( rwsType, ulVal);
        break;
      }

      case cmd_Move: {
        ULONG       ulVal2;

        if (rwsType2 == typeHndl)
            ulVal2 = *((PULONG)szPath2);
        else
            ulVal2 = (ULONG)szPath2;

        // ensure the folder gets converted from a shadow to an original
        if (rwsType2 == RWSI_OHNDL)
            rwsType2 = RWSI_SHNDL;
        else
        if (rwsType2 == RWSI_OFTITLE)
            rwsType2 = RWSI_SFTITLE;

        rc = RwsCall( &pHdr, RWSP_MNAM, (ULONG)"wpMoveObject",  // call type
                             RWSR_ASIS, 0,                      // return type
                             2,                                 // nbr of args
                             rwsType,   0, ulVal,               // object
                             rwsType2,  0, ulVal2);             // folder
        break;
      }

      case cmd_New: {
        ULONG   ulHndl;

        // ensure the folder gets converted from a shadow to an original
        if (rwsType == RWSI_OHNDL)
            rwsType = RWSI_SHNDL;
        else
        if (rwsType == RWSI_OFTITLE)
            rwsType = RWSI_SFTITLE;

        rc = RwsCall( &pHdr, RWSP_MNAM,  (ULONG)"wpclsNew",     // call type
                             RWSR_OHNDL, 0,                     // return type
                             5,                                 // nbr of args
                             RWSI_CNAME, 0, pszClass,           // class
                             RWSI_PSTR,  0, pszTitle,           // title
                             RWSI_PSTR,  0, pszSetup,           // setup
                             rwsType,    0, ulVal,              // folder
                             RWSI_ASIS,  0, 0);                 // lock
        if (rc)
            break;

        ulHndl = RwsGetResult( pHdr, 0, 0);
        printf( "%05X\n", ulHndl);
        break;
      }


      case cmd_Open: {
        HWND        hwnd;

        rc = RwsCall( &pHdr, RWSP_CMD,  RWSCMD_OPEN,    // call type
                             RWSR_ASIS, 0,              // return type
                             3,                         // nbr of args
                             rwsType,   0, ulVal,       // object
                             RWSI_ASIS, 0, rwsView,     // view to open
                             RWSI_ASIS, 0, FALSE);      // new window flag

        if (rc)
            break;

        // the WPS may return an hwnd or a task handle;
        // if this appears to be an hwnd, bring the window to the top
        hwnd = RwsGetResult( pHdr, 0, 0);
        if (hwnd != (ULONG)-1 && (hwnd & 0x80000000))
            WinSetActiveWindow( HWND_DESKTOP, hwnd);

        break;
      }

     case cmd_Popup: {
        rc = RwsCall( &pHdr, RWSP_CMD,  RWSCMD_POPUPMENU,   // call type
                             RWSR_ASIS, 0,                  // return type
                             3,                             // nbr of args
                             rwsType,   0, ulVal,           // object
                             RWSI_ASIS, 0, 0,               // owner hwnd (none)
                             RWSI_ASIS, 0, 0);              // position (at mouse)
        break;
      }

      case cmd_Using: {
        ULONG       ulVal2;

        if (rwsType2 == typeHndl)
            ulVal2 = *((PULONG)szPath2);
        else
            ulVal2 = (ULONG)szPath2;

        rc = RwsCall( &pHdr, RWSP_CMD,  RWSCMD_OPENUSING,   // call type
                             RWSR_ASIS, 0,                  // return type
                             2,                             // nbr of args
                             rwsType,  0, ulVal,            // file
                             rwsType2, 0, ulVal2);          // program object
        break;
      }

      default: {
        ErrMsg( "logic error 2", 0);
        break;
      }

    } // end switch


} while (fFalse);

    if (rc)
        ErrMsg( pszRwsErr, rc);

    // always free the memory allocated by RWS
    if (pHdr)
        RwsFreeMem( pHdr);

    return ((rc) ? FALSE : TRUE);
}

/****************************************************************************/

ULONG       ListObjectsCmd( ULONG type, ULONG value)

{
    ULONG       rc;
    BOOL        fHndlOnly = FALSE;
    ULONG       ulFlags = 0;
    ULONG       cnt;
    ULONG       ctr;
    PULONG      pObj;
    PRWSHDR     pHdr = 0;
    char *      ptr;

do {

    if (cmds[gCmd].flags & OOF_HANDLE)
        fHndlOnly = TRUE;

    // change the existing type from input-only (RWSI_*) to a
    // conversion (RWSC_*) to get the folder object's f/q name
    GETP( type) = GET_CONV;
    GETQ( type) = CONV_OBJPATH;

    // get a list of up to 512 object handles
    rc = RwsCall( &pHdr, RWSP_CMD,  RWSCMD_LISTOBJECTS, // call type
                         RWSR_ASIS, 0,                  // return type
                         3,                             // nbr of args
                         type,        0,    value,      // object
                         RWSI_PBUF,   2048, 0,          // alloc a 2k buffer
                         RWSI_PULONG, 0,    &ulFlags);  // in/out flags

    if (rc)
        break;

    // display the nbr of handles/objects returned
    cnt = RwsGetResult( pHdr, 0, 0);
    ptr = (char*)RwsGetResult( pHdr, 1, 0);
    if (ptr == (char*)-1)
        ptr = pszErr;

    printf( "%d objects in %s\n", cnt, ptr);

    if (!cnt)
        break;

    pObj = (PULONG)RwsGetResult( pHdr, 2, 0);
    if (pObj == (PULONG)-1)
        ERRNBR( 2040)       //RWSCLI_BADARGNDX

    // step through each handle;  if this is a full listing,
    // get additional info, then display it
    for (ctr = 0; ctr < cnt; ctr++)
        if (fHndlOnly)
            printf( "%05X\n", pObj[ctr]);
        else
            ListOneObject( pObj[ctr]);

    // more than 512 objects
    if (ulFlags & LISTOBJ_OVERFLOW)
        printf( "overflow - additional objects were not listed\n");

} while (fFalse);

    // always free the memory allocated by RWS
    if (pHdr)
        RwsFreeMem( pHdr);

    return (rc);
}

/****************************************************************************/

void        ListOneObject( ULONG ulObj)

{
    ULONG       rc;
    char *      ptr;
    PRWSHDR     pHdr = 0;
    PRWSDSC     pArg1;
    PRWSDSC     pArg2;
    PRWSDSC     pArg3;

do {
    // use RWSConvert to get the object's ID, class, & title
    rc = RwsCall( &pHdr, RWSP_CONV, 0,              // call type
                         RWSR_ASIS, 0,              // return type
                         3,                         // nbr of args
                         RWSC_OHNDL_OID,  0, ulObj, // handle to object ID
                         RWSC_PREV_CNAME, 0, 0,     // same obj to class name
                         RWSC_PREV_ONAME, 0, 0);    // same obj to obj title
    if (rc)
        break;

    // get pointers to the data associated with each arg
    rc = RwsGetArgPtr( pHdr, 1, &pArg1);
    if (rc)
        break;
    pArg2 = pArg1->pnext;
    pArg3 = pArg2->pnext;

    // if we got a title, replace any CR or LF with a caret ('^')
    if (!pArg3->rc)
        for (ptr = (char*)pArg3->pget; *ptr; ptr++)
            if (*ptr == '\n' || *ptr == '\r')
                *ptr = '^';

    // display the info, checking for conversion errors
    printf( "%X    %-16s  %-16s  %s\n",
            ulObj,
            (pArg1->rc ? "----" : (char*)pArg1->pget),
            (pArg2->rc ? pszErr : (char*)pArg2->pget),
            (pArg3->rc ? pszErr : (char*)pArg3->pget));

} while (fFalse);

    // if there was a major failure, indicate as much in the output
    // data (sent to stdout) & with an error message (sent to stderr)
    if (rc) {
        printf( "%X    %-16s  %-16s  %s\n", ulObj, pszErr, pszErr, pszErr);
        ErrMsg( pszRwsErr, rc);
    }        

    // always free the memory allocated by RWS
    if (pHdr)
        RwsFreeMem( pHdr);

    return;
}

/****************************************************************************/

ULONG       ShowInfoCmd( ULONG type, ULONG value)

{
    ULONG       rc = 0;
    PRWSHDR     pHdr = 0;
    PRWSDSC     pArg1;
    PRWSDSC     pArg2;
    PRWSDSC     pArg3;
    PRWSDSC     pArg4;
    PRWSDSC     pArg5;

do {
    // change the existing type from input-only (RWSI_*)
    // to a conversion (RWSC_*) to get the object's handle
    GETP( type) = GET_CONV;
    GETQ( type) = CONV_OBJHNDL;

    // if the user asked for handle only, do a single conversion,
    // then display the result;  if the input was a handle, this
    // doesn't convert anything but does confirm the handle is valid
    if (cmds[gCmd].flags & OOF_HANDLE) {
        rc = RwsCall( &pHdr, RWSP_CONV, 0,
                             RWSR_ASIS, 0,
                             1,
                             type, 0, value);
        if (rc)
            break;

        rc = RwsGetArgPtr( pHdr, 1, &pArg1);
        if (rc)
            break;

        printf( "%X\n", ((type == RWSI_OHNDL) ? pArg1->value :
                            ((pArg1->rc) ? 0 : *((PULONG)pArg1->pget))));
        break;
    }

    // for a full display, perform 5 conversions:  input to
    // handle, object ID, class name, path to object, & object title
    // (we could ask for object's f/q title but that won't always
    // give us the real names of filesystem objects)
    rc = RwsCall( &pHdr, RWSP_CONV, 0,
                         RWSR_ASIS, 0,
                         5,
                         type,              0, value,
                         RWSC_PREV_OID,     0, 0,
                         RWSC_PREV_CNAME,   0, 0,
                         RWSC_PREV_OFLDR,   0, 0,
                         RWSC_PREV_ONAME,   0, 0);
    if (rc)
        break;

    // get pointers to each of the args
    rc = RwsGetArgPtr( pHdr, 1, &pArg1);
    if (rc)
        break;
    pArg2 = pArg1->pnext;
    pArg3 = pArg2->pnext;
    pArg4 = pArg3->pnext;
    pArg5 = pArg4->pnext;

    // display the info, checking for conversion errors
    printf( "%X  %s  %s  \"%s\\%s\"\n",
            (pArg1->rc ? 0      : *((PULONG)pArg1->pget)),
            (pArg2->rc ? "----" : (char*)pArg2->pget),
            (pArg3->rc ? pszErr : (char*)pArg3->pget),
            (pArg4->rc ? pszErr : (char*)pArg4->pget),
            (pArg5->rc ? pszErr : (char*)pArg5->pget));

} while (fFalse);

    // always free the memory allocated by RWS
    if (pHdr)
        RwsFreeMem( pHdr);

    return (rc);
}

/****************************************************************************/

void        ErrMsg( char * msg, ULONG rc)

{
    ULONG   cnt;
    char    szErr[64];
    char    szText[300];

    // if there's an rc, it's an RWS error code - try to get
    // the code's description so we can display it too
    if (rc) {
        if (!RwsGetRcString( rc, sizeof(szErr), szErr))
            sprintf( szText, "%s - rc= %d (%s)\r\n", msg, rc, szErr);
        else
            sprintf( szText, "%s - rc= %d\r\n", msg, rc);
    }
    else
        sprintf( szText, "%s\r\n", msg);

    // send the error to stderr (note that 'oo' is built with
    // VACPP's no-runtime-environment library which doesn't
    // support fprintf() or stderr - thus, it uses DosWrite()
    DosWrite( 2, szText, strlen( szText), &cnt);

    return;
}

/****************************************************************************/

void        ShowHelp( void)

{
    int     ctr;
    int     ndx;
    char *  ptr;
    char    szExe[4] = "oo";
    char    szSwitch[8];

    // reconstruct the exe's name based on the default command
    szExe[1] = (cmds[gDef].letter | 0x20);

    // display the heading
    printf( szHelpHead, szExe);

    // display the individual commands in the order specified by aiHelp
    for (ctr = 0; ctr < HELP_CNT; ctr++) {
        ndx = aiHelp[ctr];
        ptr = szSwitch;

        // construct the switch & options string
        if (ndx == gDef)
            *ptr++ = '[';
        *ptr++ = '/';
        *ptr++ = cmds[ndx].letter;

        if (cmds[ndx].opts[0]) {
            if (ndx != gDef)
                *ptr++ = '[';
            *ptr++ = cmds[ndx].opts[0] | 0x20;
            if (cmds[ndx].opts[1])
                *ptr++ = cmds[ndx].opts[1] | 0x20;
            *ptr++ = ']';
        }
        else
            if (ndx == gDef)
                *ptr++ = ']';

        *ptr = 0;
        printf( cmds[ndx].pszHelp, szExe, szSwitch);
    }

    // display the remainder
    printf( szHelpBody);

    return;
}

/****************************************************************************/

