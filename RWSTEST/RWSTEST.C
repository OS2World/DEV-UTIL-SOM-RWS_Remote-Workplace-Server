/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSTEST.C
// Remote Workplace Shell - demo program "RwsTest"

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
 * The Original Code is Remote Workplace Server - "RwsTest" demo program.
 *
 * The Initial Developer of the Original Code is Richard L. Walsh.
 * 
 * Portions created by the Initial Developer are Copyright (C) 2004, 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

/****************************************************************************/
/*

RwsTest lets you experiment with RWS.  You can invoke any procedure that
takes up to four arguments or you can do up to four object conversions.
If the RWS object class isn't already registered, RwsTest will do so.

Note:  RwsTest requires you to identify whether your input is hex, decimal,
or a string.  This is NOT a requirement of RWS - I was just too lazy to
write some code that would figure it out automatically (and wrong, probably).

*/
/****************************************************************************/

#define INCL_DOS
#define INCL_PM
#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "..\RWS.H"
#include "RWSTEST.H"

/****************************************************************************/

void        Morph( void);
MRESULT _System MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL        InitDlg( HWND hwnd);
void        ClearDlg( HWND hwnd);
BOOL        FillCB( HWND hwnd, ULONG ulID, PSZUL * pszul);
BOOL        FillList( HWND hwnd, ULONG ulID, PSZULNDX * List);
void        SwitchTypeLists( HWND hwnd, HWND hCB);
void        SetXDS( HWND hwnd, HWND hCB, ULONG ulID);
void        Run( HWND hwnd);
BOOL        GetProc( HWND hwnd, PRWSBLD pBld, char ** ppBuf);
BOOL        GetRtn( HWND hwnd, PRWSBLD pBld);
ULONG       GetArg( HWND hwnd, PRWSARG pArg, ULONG ulID, char ** ppBuf);
void        ShowResults( HWND hwnd, PRWSBLD pBld);

/****************************************************************************/

char        szCopyright[] = "RWS Test v" RWSVERSION " - (C)Copyright 2004, 2005  R.L.Walsh\r\n";
BOOL        fFalse = FALSE;
BOOL        fConv = FALSE;

char        argbuf[1024];
char        rtnbuf[1024];

/****************************************************************************/

PSZUL   pszulXDS[] =
    {
        {"hex",                 XDS_HEX},
        {"dec",                 XDS_DEC},
        {"str",                 XDS_STR},
        {0,                     0}
    };

PSZULNDX    ProcList[] =
    {
        {"RWSP_MNAM",           RWSP_MNAM,              XDS_STR},
        {"RWSP_CONV",           RWSP_CONV,              XDS_HEX},
        {"RWSP_CMD",            RWSP_CMD,               XDS_DEC},
        {"RWSP_KORD",           RWSP_KORD,              XDS_DEC},
        {"RWSP_MPFN",           RWSP_MPFN,              XDS_HEX},
        {"RWSP_KPFN",           RWSP_KPFN,              XDS_HEX},
        {"RWSP_MNAMI",          RWSP_MNAMI,             XDS_STR},
        {"RWSP_MPFNI",          RWSP_MPFNI,             XDS_HEX},
        {"RWSP_KORDI",          RWSP_KORDI,             XDS_DEC},
        {"RWSP_KPFNI",          RWSP_KPFNI,             XDS_HEX},
        {"RWSP_MNAMZ",          RWSP_MNAMZ,             XDS_STR},
        {"RWSP_MPFNZ",          RWSP_MPFNZ,             XDS_HEX},
        {"RWSP_KORDZ",          RWSP_KORDZ,             XDS_DEC},
        {"RWSP_KPFNZ",          RWSP_KPFNZ,             XDS_HEX},
        {0,                     0,                      0}
    };

PSZULNDX    ArgList[] =
    {
        {"(none)",              0xffffffff,     MAKEULONG(LIT_NONE, 0)},
        {"RWSI_ASIS",           RWSI_ASIS,              XDS_HEX},
        {"RWSI_PSTR",           RWSI_PSTR,              XDS_STR},
        {"RWSI_PBUF",           RWSI_PBUF,              XDS_HEX},
        {"RWSI_PPVOID",         RWSI_PPVOID,            XDS_HEX},
        {"RWSI_OPATH",          RWSI_OPATH,             XDS_STR},
        {"RWSI_OFTITLE",        RWSI_OFTITLE,           XDS_STR},
        {"RWSI_OHNDL",          RWSI_OHNDL,             XDS_HEX},
        {"RWSI_OHWND",          RWSI_OHWND,             XDS_HEX},
        {"RWSI_OPREC",          RWSI_OPREC,             XDS_HEX},
        {"RWSI_SFTITLE",        RWSI_SFTITLE,           XDS_STR},
        {"RWSI_SHNDL",          RWSI_SHNDL,             XDS_HEX},
        {"RWSI_SPREC",          RWSI_SPREC,             XDS_HEX},
        {"RWSI_CNAME",          RWSI_CNAME,             XDS_STR},
        {"RWSI_COBJ",           RWSI_COBJ,              XDS_HEX},
        {"RWSI_COPATH",         RWSI_COPATH,            XDS_STR},
        {"RWSI_COFTITLE",       RWSI_COFTITLE,          XDS_STR},
        {"RWSI_COHNDL",         RWSI_COHNDL,            XDS_HEX},
        {"RWSI_COHWND",         RWSI_COHWND,            XDS_HEX},
        {"RWSI_COPREC",         RWSI_COPREC,            XDS_HEX},
        {"RWSI_CSFTITLE",       RWSI_CSFTITLE,          XDS_STR},
        {"RWSI_CSHNDL",         RWSI_CSHNDL,            XDS_HEX},
        {"RWSI_CSPREC",         RWSI_CSPREC,            XDS_HEX},
        {"RWSI_SOMID",          RWSI_SOMID,             XDS_STR},
        {"RWSI_SOMIDFREE",      RWSI_SOMIDFREE,         XDS_STR},
        {"RWSI_SOMCLSMGR",      RWSI_SOMCLSMGR,         XDS_HEX},
        {"RWSI_POBJSTR",        RWSI_POBJSTR,           XDS_STR},
        {"RWSI_PSOMSTR",        RWSI_PSOMSTR,           XDS_STR},
        {"RWSI_POBJBUF",        RWSI_POBJBUF,           XDS_HEX},
        {"RWSI_PSOMBUF",        RWSI_PSOMBUF,           XDS_HEX},
        {"RWSO_OPATH",          RWSO_OPATH,             XDS_HEX},
        {"RWSO_OFTITLE",        RWSO_OFTITLE,           XDS_HEX},
        {"RWSO_ONAME",          RWSO_ONAME,             XDS_HEX},
        {"RWSO_OTITLE",         RWSO_OTITLE,            XDS_HEX},
        {"RWSO_OID",            RWSO_OID,               XDS_HEX},
        {"RWSO_OFLDR",          RWSO_OFLDR,             XDS_HEX},
        {"RWSO_OHNDL",          RWSO_OHNDL,             XDS_HEX},
        {"RWSO_OHWND",          RWSO_OHWND,             XDS_HEX},
        {"RWSO_OPREC",          RWSO_OPREC,             XDS_HEX},
        {"RWSO_CNAME",          RWSO_CNAME,             XDS_HEX},
        {"RWSO_SOMID",          RWSO_SOMID,             XDS_HEX},
        {"RWSO_SOMIDFREE",      RWSO_SOMIDFREE,         XDS_HEX},
        {"RWSO_PPSTR",          RWSO_PPSTR,             XDS_HEX},
        {"RWSO_PPBUF",          RWSO_PPBUF,             XDS_HEX},
        {"RWSO_PPBUFCNTRTN",    RWSO_PPBUFCNTRTN,       XDS_HEX},
        {"RWSO_PPBUFCNTARG1",   RWSO_PPBUFCNTARG1,      XDS_HEX},
        {"RWSO_PPBUFCNTARG2",   RWSO_PPBUFCNTARG2,      XDS_HEX},
        {"RWSO_PPBUFCNTARG3",   RWSO_PPBUFCNTARG3,      XDS_HEX},
        {"RWSO_PPBUFCNTARG4",   RWSO_PPBUFCNTARG4,      XDS_HEX},
        {"RWSO_PPBUFCNTARG5",   RWSO_PPBUFCNTARG5,      XDS_HEX},
        {"RWSO_PPBUFCNTARG6",   RWSO_PPBUFCNTARG6,      XDS_HEX},
        {"RWSO_PPBUFCNTULONG",  RWSO_PPBUFCNTULONG,     XDS_HEX},
        {"RWSO_PPBUFCNTUSHORT", RWSO_PPBUFCNTUSHORT,    XDS_HEX},
        {"RWSO_PPBUFCNTULZERO", RWSO_PPBUFCNTULZERO,    XDS_HEX},
        {"RWSO_PPSTROBJFREE",   RWSO_PPSTROBJFREE,      XDS_HEX},
        {"RWSO_PPSTRSOMFREE",   RWSO_PPSTRSOMFREE,      XDS_HEX},
        {"RWSO_PPBUFOBJFREE",   RWSO_PPBUFOBJFREE,      XDS_HEX},
        {"RWSO_PPBUFSOMFREE",   RWSO_PPBUFSOMFREE,      XDS_HEX},
        {0,                     0,                      0}
    };

PSZUL   pszulRtn[] =
    {
        {"RWSR_ASIS",           RWSR_ASIS},
        {"RWSR_VOID",           RWSR_VOID},
        {"RWSR_OPATH",          RWSR_OPATH},
        {"RWSR_OFTITLE",        RWSR_OFTITLE},
        {"RWSR_ONAME",          RWSR_ONAME},
        {"RWSR_OTITLE",         RWSR_OTITLE},
        {"RWSR_OID",            RWSR_OID},
        {"RWSR_OFLDR",          RWSR_OFLDR},
        {"RWSR_OHNDL",          RWSR_OHNDL},
        {"RWSR_OHWND",          RWSR_OHWND},
        {"RWSR_OPREC",          RWSR_OPREC},
        {"RWSR_CNAME",          RWSR_CNAME},
        {"RWSR_SOMID",          RWSR_SOMID},
        {"RWSR_SOMIDFREE",      RWSR_SOMIDFREE},
        {"RWSR_PSTR",           RWSR_PSTR},
        {"RWSR_PBUF",           RWSR_PBUF},
        {"RWSR_PBUFCNTARG1",    RWSR_PBUFCNTARG1},
        {"RWSR_PBUFCNTARG2",    RWSR_PBUFCNTARG2},
        {"RWSR_PBUFCNTARG3",    RWSR_PBUFCNTARG3},
        {"RWSR_PBUFCNTARG4",    RWSR_PBUFCNTARG4},
        {"RWSR_PBUFCNTARG5",    RWSR_PBUFCNTARG5},
        {"RWSR_PBUFCNTARG6",    RWSR_PBUFCNTARG6},
        {"RWSR_PBUFCNTULONG",   RWSR_PBUFCNTULONG},
        {"RWSR_PBUFCNTUSHORT",  RWSR_PBUFCNTUSHORT},
        {"RWSR_PBUFCNTULZERO",  RWSR_PBUFCNTULZERO},
        {"RWSR_PPSTR",          RWSR_PPSTR},
        {"RWSR_PPBUF",          RWSR_PPBUF},
        {"RWSR_PSTROBJFREE",    RWSR_PSTROBJFREE},
        {"RWSR_PBUFOBJFREE",    RWSR_PBUFOBJFREE},
        {"RWSR_PPSTROBJFREE",   RWSR_PPSTROBJFREE},
        {"RWSR_PPBUFOBJFREE",   RWSR_PPBUFOBJFREE},
        {"RWSR_PSTRSOMFREE",    RWSR_PSTRSOMFREE},
        {"RWSR_PBUFSOMFREE",    RWSR_PBUFSOMFREE},
        {"RWSR_PPSTRSOMFREE",   RWSR_PPSTRSOMFREE},
        {"RWSR_PPBUFSOMFREE",   RWSR_PPBUFSOMFREE},
        {0,                     0}
    };

PSZULNDX    ConvList[] =
    {
        {"(none)",              0xffffffff,     MAKEULONG(LIT_NONE, 0)},
        {"RWSC_OPATH_OBJ",      RWSC_OPATH_OBJ,         XDS_STR},
        {"RWSC_OPATH_OTITLE",   RWSC_OPATH_OTITLE,      XDS_STR},
        {"RWSC_OPATH_OID",      RWSC_OPATH_OID,         XDS_STR},
        {"RWSC_OPATH_OHNDL",    RWSC_OPATH_OHNDL,       XDS_STR},
        {"RWSC_OPATH_OHWND",    RWSC_OPATH_OHWND,       XDS_STR},
        {"RWSC_OPATH_OPREC",    RWSC_OPATH_OPREC,       XDS_STR},
        {"RWSC_OPATH_CLASS",    RWSC_OPATH_CLASS,       XDS_STR},
        {"RWSC_OPATH_CNAME",    RWSC_OPATH_CNAME,       XDS_STR},
        {"RWSC_OFTITLE_OBJ",    RWSC_OFTITLE_OBJ,       XDS_STR},
        {"RWSC_OFTITLE_ONAME",  RWSC_OFTITLE_ONAME,     XDS_STR},
        {"RWSC_OFTITLE_OID",    RWSC_OFTITLE_OID,       XDS_STR},
        {"RWSC_OFTITLE_OHNDL",  RWSC_OFTITLE_OHNDL,     XDS_STR},
        {"RWSC_OFTITLE_OHWND",  RWSC_OFTITLE_OHWND,     XDS_STR},
        {"RWSC_OFTITLE_OPREC",  RWSC_OFTITLE_OPREC,     XDS_STR},
        {"RWSC_OFTITLE_CLASS",  RWSC_OFTITLE_CLASS,     XDS_STR},
        {"RWSC_OFTITLE_CNAME",  RWSC_OFTITLE_CNAME,     XDS_STR},
        {"RWSC_OHNDL_OBJ",      RWSC_OHNDL_OBJ,         XDS_HEX},
        {"RWSC_OHNDL_OPATH",    RWSC_OHNDL_OPATH,       XDS_HEX},
        {"RWSC_OHNDL_OFTITLE",  RWSC_OHNDL_OFTITLE,     XDS_HEX},
        {"RWSC_OHNDL_ONAME",    RWSC_OHNDL_ONAME,       XDS_HEX},
        {"RWSC_OHNDL_OTITLE",   RWSC_OHNDL_OTITLE,      XDS_HEX},
        {"RWSC_OHNDL_OID",      RWSC_OHNDL_OID,         XDS_HEX},
        {"RWSC_OHNDL_OFLDR",    RWSC_OHNDL_OFLDR,       XDS_HEX},
        {"RWSC_OHNDL_OHWND",    RWSC_OHNDL_OHWND,       XDS_HEX},
        {"RWSC_OHNDL_OPREC",    RWSC_OHNDL_OPREC,       XDS_HEX},
        {"RWSC_OHNDL_CLASS",    RWSC_OHNDL_CLASS,       XDS_HEX},
        {"RWSC_OHNDL_CNAME",    RWSC_OHNDL_CNAME,       XDS_HEX},
        {"RWSC_OHWND_OBJ",      RWSC_OHWND_OBJ,         XDS_HEX},
        {"RWSC_OHWND_OPATH",    RWSC_OHWND_OPATH,       XDS_HEX},
        {"RWSC_OHWND_OFTITLE",  RWSC_OHWND_OFTITLE,     XDS_HEX},
        {"RWSC_OHWND_ONAME",    RWSC_OHWND_ONAME,       XDS_HEX},
        {"RWSC_OHWND_OTITLE",   RWSC_OHWND_OTITLE,      XDS_HEX},
        {"RWSC_OHWND_OID",      RWSC_OHWND_OID,         XDS_HEX},
        {"RWSC_OHWND_OFLDR",    RWSC_OHWND_OFLDR,       XDS_HEX},
        {"RWSC_OHWND_OHNDL",    RWSC_OHWND_OHNDL,       XDS_HEX},
        {"RWSC_OHWND_OPREC",    RWSC_OHWND_OPREC,       XDS_HEX},
        {"RWSC_OHWND_CLASS",    RWSC_OHWND_CLASS,       XDS_HEX},
        {"RWSC_OHWND_CNAME",    RWSC_OHWND_CNAME,       XDS_HEX},
        {"RWSC_OPREC_OBJ",      RWSC_OPREC_OBJ,         XDS_HEX},
        {"RWSC_OPREC_OPATH",    RWSC_OPREC_OPATH,       XDS_HEX},
        {"RWSC_OPREC_OFTITLE",  RWSC_OPREC_OFTITLE,     XDS_HEX},
        {"RWSC_OPREC_ONAME",    RWSC_OPREC_ONAME,       XDS_HEX},
        {"RWSC_OPREC_OTITLE",   RWSC_OPREC_OTITLE,      XDS_HEX},
        {"RWSC_OPREC_OID",      RWSC_OPREC_OID,         XDS_HEX},
        {"RWSC_OPREC_OFLDR",    RWSC_OPREC_OFLDR,       XDS_HEX},
        {"RWSC_OPREC_OHNDL",    RWSC_OPREC_OHNDL,       XDS_HEX},
        {"RWSC_OPREC_OHWND",    RWSC_OPREC_OHWND,       XDS_HEX},
        {"RWSC_OPREC_CLASS",    RWSC_OPREC_CLASS,       XDS_HEX},
        {"RWSC_OPREC_CNAME",    RWSC_OPREC_CNAME,       XDS_HEX},
        {"RWSC_SFTITLE_OBJ",    RWSC_SFTITLE_OBJ,       XDS_STR},
        {"RWSC_SFTITLE_OPATH",  RWSC_SFTITLE_OPATH,     XDS_STR},
        {"RWSC_SFTITLE_OFTITLE",RWSC_SFTITLE_OFTITLE,   XDS_STR},
        {"RWSC_SFTITLE_ONAME",  RWSC_SFTITLE_ONAME,     XDS_STR},
        {"RWSC_SFTITLE_OID",    RWSC_SFTITLE_OID,       XDS_STR},
        {"RWSC_SFTITLE_OFLDR",  RWSC_SFTITLE_OFLDR,     XDS_STR},
        {"RWSC_SFTITLE_OHNDL",  RWSC_SFTITLE_OHNDL,     XDS_STR},
        {"RWSC_SFTITLE_OHWND",  RWSC_SFTITLE_OHWND,     XDS_STR},
        {"RWSC_SFTITLE_OPREC",  RWSC_SFTITLE_OPREC,     XDS_STR},
        {"RWSC_SFTITLE_CLASS",  RWSC_SFTITLE_CLASS,     XDS_STR},
        {"RWSC_SFTITLE_CNAME",  RWSC_SFTITLE_CNAME,     XDS_STR},
        {"RWSC_SHNDL_OBJ",      RWSC_SHNDL_OBJ,         XDS_HEX},
        {"RWSC_SHNDL_OPATH",    RWSC_SHNDL_OPATH,       XDS_HEX},
        {"RWSC_SHNDL_OFTITLE",  RWSC_SHNDL_OFTITLE,     XDS_HEX},
        {"RWSC_SHNDL_ONAME",    RWSC_SHNDL_ONAME,       XDS_HEX},
        {"RWSC_SHNDL_OTITLE",   RWSC_SHNDL_OTITLE,      XDS_HEX},
        {"RWSC_SHNDL_OID",      RWSC_SHNDL_OID,         XDS_HEX},
        {"RWSC_SHNDL_OFLDR",    RWSC_SHNDL_OFLDR,       XDS_HEX},
        {"RWSC_SHNDL_OHNDL",    RWSC_SHNDL_OHNDL,       XDS_HEX},
        {"RWSC_SHNDL_OHWND",    RWSC_SHNDL_OHWND,       XDS_HEX},
        {"RWSC_SHNDL_OPREC",    RWSC_SHNDL_OPREC,       XDS_HEX},
        {"RWSC_SHNDL_CLASS",    RWSC_SHNDL_CLASS,       XDS_HEX},
        {"RWSC_SHNDL_CNAME",    RWSC_SHNDL_CNAME,       XDS_HEX},
        {"RWSC_SPREC_OBJ",      RWSC_SPREC_OBJ,         XDS_HEX},
        {"RWSC_SPREC_OPATH",    RWSC_SPREC_OPATH,       XDS_HEX},
        {"RWSC_SPREC_OFTITLE",  RWSC_SPREC_OFTITLE,     XDS_HEX},
        {"RWSC_SPREC_ONAME",    RWSC_SPREC_ONAME,       XDS_HEX},
        {"RWSC_SPREC_OTITLE",   RWSC_SPREC_OTITLE,      XDS_HEX},
        {"RWSC_SPREC_OID",      RWSC_SPREC_OID,         XDS_HEX},
        {"RWSC_SPREC_OFLDR",    RWSC_SPREC_OFLDR,       XDS_HEX},
        {"RWSC_SPREC_OHNDL",    RWSC_SPREC_OHNDL,       XDS_HEX},
        {"RWSC_SPREC_OHWND",    RWSC_SPREC_OHWND,       XDS_HEX},
        {"RWSC_SPREC_OPREC",    RWSC_SPREC_OPREC,       XDS_HEX},
        {"RWSC_SPREC_CLASS",    RWSC_SPREC_CLASS,       XDS_HEX},
        {"RWSC_SPREC_CNAME",    RWSC_SPREC_CNAME,       XDS_HEX},
        {"RWSC_PREV_OPATH",     RWSC_PREV_OPATH,        XDS_HEX},
        {"RWSC_PREV_OFTITLE",   RWSC_PREV_OFTITLE,      XDS_HEX},
        {"RWSC_PREV_ONAME",     RWSC_PREV_ONAME,        XDS_HEX},
        {"RWSC_PREV_OTITLE",    RWSC_PREV_OTITLE,       XDS_HEX},
        {"RWSC_PREV_OID",       RWSC_PREV_OID,          XDS_HEX},
        {"RWSC_PREV_OFLDR",     RWSC_PREV_OFLDR,        XDS_HEX},
        {"RWSC_PREV_OHNDL",     RWSC_PREV_OHNDL,        XDS_HEX},
        {"RWSC_PREV_OHWND",     RWSC_PREV_OHWND,        XDS_HEX},
        {"RWSC_PREV_OPREC",     RWSC_PREV_OPREC,        XDS_HEX},
        {"RWSC_PREV_CLASS",     RWSC_PREV_CLASS,        XDS_HEX},
        {"RWSC_PREV_CNAME",     RWSC_PREV_CNAME,        XDS_HEX},
        {"RWSC_OBJ_OPATH",      RWSC_OBJ_OPATH,         XDS_HEX},
        {"RWSC_OBJ_OFTITLE",    RWSC_OBJ_OFTITLE,       XDS_HEX},
        {"RWSC_OBJ_ONAME",      RWSC_OBJ_ONAME,         XDS_HEX},
        {"RWSC_OBJ_OTITLE",     RWSC_OBJ_OTITLE,        XDS_HEX},
        {"RWSC_OBJ_OID",        RWSC_OBJ_OID,           XDS_HEX},
        {"RWSC_OBJ_OFLDR",      RWSC_OBJ_OFLDR,         XDS_HEX},
        {"RWSC_OBJ_OHNDL",      RWSC_OBJ_OHNDL,         XDS_HEX},
        {"RWSC_OBJ_OHWND",      RWSC_OBJ_OHWND,         XDS_HEX},
        {"RWSC_OBJ_OPREC",      RWSC_OBJ_OPREC,         XDS_HEX},
        {"RWSC_OBJ_CLASS",      RWSC_OBJ_CLASS,         XDS_HEX},
        {"RWSC_OBJ_CNAME",      RWSC_OBJ_CNAME,         XDS_HEX},
        {"RWSC_CLASS_CNAME",    RWSC_CLASS_CNAME,       XDS_HEX},
        {"RWSC_CNAME_CLASS",    RWSC_CNAME_CLASS,       XDS_STR},
        {"RWSC_NULL_SOMCLSMGR", RWSC_NULL_SOMCLSMGR,    XDS_HEX},
        {"RWSC_ADDR_PSTR",      RWSC_ADDR_PSTR,         XDS_HEX},
        {"RWSC_ADDR_PBUF",      RWSC_ADDR_PBUF,         XDS_HEX},
        {0,                     0,                      0}
    };


/****************************************************************************/

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
    // RWSFULLVERSION for RwsTest v0.80 is 0x08000100
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
        DosBeep( 440, 150);

    if (hwnd)
        WinDestroyWindow( hwnd);
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

MRESULT _System MainWndProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
 
{
    switch (msg)
    {
        case WM_INITDLG:
            InitDlg( hwnd);
            break;

        case WM_COMMAND:
            if ((USHORT)mp1 == IDC_OK)
                Run( hwnd);
            else
            if ((USHORT)mp1 == IDC_CLEAR)
                ClearDlg( hwnd);
            break;

        case WM_CONTROL:
            if (SHORT2FROMMP( mp1) != CBN_LBSELECT)
                break;

            switch (SHORT1FROMMP( mp1))
            {
                case IDC_PROCTYPE:
                    SwitchTypeLists( hwnd, (HWND)mp2);
                    break;

                case IDC_ARG1TYPE:
                case IDC_ARG2TYPE:
                case IDC_ARG3TYPE:
                case IDC_ARG4TYPE:
                case IDC_ARG5TYPE:
                case IDC_ARG6TYPE:
                    SetXDS( hwnd, (HWND)mp2, SHORT1FROMMP( mp1));
                    break;
            }
            break;

        case WM_CHAR:
        // defeat default button action
            if (((CHARMSG(&msg)->fs & (USHORT)KC_VIRTUALKEY) &&
                   CHARMSG(&msg)->vkey == VK_ENTER) ||
                ((CHARMSG(&msg)->fs & (USHORT)KC_CHAR) &&
                   CHARMSG(&msg)->chr == '\r'))
                break;

            return (WinDefDlgProc( hwnd, msg, mp1, mp2));

        case WM_CLOSE:
            WinPostMsg( hwnd, WM_QUIT, NULL, NULL);
            break;

        case WM_DESTROY:
            if (RwsClientTerminate())
                printf( "\r\nRwsClientTerminate failed\r\n");
            return (WinDefDlgProc( hwnd, msg, mp1, mp2));

        default:
            return (WinDefDlgProc( hwnd, msg, mp1, mp2));

    } //end switch (msg)

    return (0);
}

/****************************************************************************/

BOOL        InitDlg( HWND hwnd)

{
    BOOL        fRtn = FALSE;
    ULONG       rc;
    HPOINTER    hIcon;
    char *      pErr = "\r\nDialog initialization failed\r\n";

do
{
    hIcon = WinLoadPointer( HWND_DESKTOP, 0, IDI_RWSTEST);
    if (hIcon)
        WinSendMsg( hwnd, WM_SETICON, (MP)hIcon, 0);

    if (!WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_INSERT, (MP)szCopyright, 0))
        break;

    if (!WinSendDlgItemMsg( hwnd, IDC_PROCNAME, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0) ||
        !WinSendDlgItemMsg( hwnd, IDC_ARG1DATA, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0) ||
        !WinSendDlgItemMsg( hwnd, IDC_ARG2DATA, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0) ||
        !WinSendDlgItemMsg( hwnd, IDC_ARG3DATA, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0) ||
        !WinSendDlgItemMsg( hwnd, IDC_ARG4DATA, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0) ||
        !WinSendDlgItemMsg( hwnd, IDC_ARG5DATA, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0) ||
        !WinSendDlgItemMsg( hwnd, IDC_ARG6DATA, EM_SETTEXTLIMIT, (MP)RWSSTRINGSIZE, 0))
        break;

    if (!FillList( hwnd, IDC_PROCTYPE, ProcList) ||
        !FillList( hwnd, IDC_ARG1TYPE, ArgList) ||
        !FillList( hwnd, IDC_ARG2TYPE, ArgList) ||
        !FillList( hwnd, IDC_ARG3TYPE, ArgList) ||
        !FillList( hwnd, IDC_ARG4TYPE, ArgList) ||
        !FillList( hwnd, IDC_ARG5TYPE, ArgList) ||
        !FillList( hwnd, IDC_ARG6TYPE, ArgList) ||
        !FillCB( hwnd, IDC_RTNTYPE,  pszulRtn))
        break;

    if (!FillCB( hwnd, IDC_PROCXDS, pszulXDS) ||
        !FillCB( hwnd, IDC_ARG1XDS, pszulXDS) ||
        !FillCB( hwnd, IDC_ARG2XDS, pszulXDS) ||
        !FillCB( hwnd, IDC_ARG3XDS, pszulXDS) ||
        !FillCB( hwnd, IDC_ARG4XDS, pszulXDS) ||
        !FillCB( hwnd, IDC_ARG5XDS, pszulXDS) ||
        !FillCB( hwnd, IDC_ARG6XDS, pszulXDS))
        break;

    fConv = FALSE;
    WinSendDlgItemMsg( hwnd, IDC_PROCTYPE, LM_SELECTITEM, 0, (MP)TRUE);
    ClearDlg( hwnd);

    rc = RwsClientInit( TRUE);
    if (rc == 0)
        fRtn = TRUE;
    else
    {
        sprintf( rtnbuf, "\r\nRwsClientInit failed - rc= %d\r\n", rc);
        pErr = rtnbuf;
    }

} while (fFalse);

    if (fRtn == FALSE)
    {
        WinEnableControl( hwnd, IDC_OK, FALSE);
        WinEnableControl( hwnd, IDC_CLEAR, FALSE);
        WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_SETSEL, (MP)0x7fffffff, (MP)0x7fffffff);
        WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_INSERT, (MP)pErr, 0);
        WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_INSERT,
                           (MP)"RWSTest is disabled - please exit", 0);
    }

    return (fRtn);
}

/****************************************************************************/

void        ClearDlg( HWND hwnd)

{
    WinSetDlgItemText( hwnd, IDC_PROCNAME, "");
    WinSetDlgItemText( hwnd, IDC_ARG1DATA, "");
    WinSetDlgItemText( hwnd, IDC_ARG2DATA, "");
    WinSetDlgItemText( hwnd, IDC_ARG3DATA, "");
    WinSetDlgItemText( hwnd, IDC_ARG4DATA, "");
    WinSetDlgItemText( hwnd, IDC_ARG5DATA, "");
    WinSetDlgItemText( hwnd, IDC_ARG6DATA, "");

    WinSetDlgItemText( hwnd, IDC_RTNSIZE,  "0");
    WinSetDlgItemText( hwnd, IDC_ARG1SIZE, "0");
    WinSetDlgItemText( hwnd, IDC_ARG2SIZE, "0");
    WinSetDlgItemText( hwnd, IDC_ARG3SIZE, "0");
    WinSetDlgItemText( hwnd, IDC_ARG4SIZE, "0");
    WinSetDlgItemText( hwnd, IDC_ARG5SIZE, "0");
    WinSetDlgItemText( hwnd, IDC_ARG6SIZE, "0");

    WinSendDlgItemMsg( hwnd, IDC_RTNTYPE,  LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG1TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG2TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG3TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG4TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG5TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG6TYPE, LM_SELECTITEM, 0, (MP)TRUE);

    return;
}

/****************************************************************************/

// fills a combobox from an array of PSZUL structs

BOOL        FillCB( HWND hwnd, ULONG ulID, PSZUL * pszul)

{
    BOOL    fRtn = FALSE;
    HWND    hCtl;

    hCtl = WinWindowFromID( hwnd, ulID);
    if (hCtl)
    {
        WinSendMsg( hCtl, LM_DELETEALL, 0, 0);
        for (fRtn = TRUE; pszul->psz; pszul++)
            if ((SHORT)WinSendMsg( hCtl, LM_INSERTITEM, (MP)LIT_END,
                                   (MP)pszul->psz) < 0)
            {
                fRtn = FALSE;
                break;
            }
    }

    return (fRtn);
}

/****************************************************************************/

// fills a combobox from an array of PSZULNDX structs

BOOL        FillList( HWND hwnd, ULONG ulID, PSZULNDX * List)

{
    BOOL    fRtn = FALSE;
    HWND    hCtl;

    hCtl = WinWindowFromID( hwnd, ulID);
    if (hCtl)
    {
        WinSendMsg( hCtl, LM_DELETEALL, 0, 0);
        for (fRtn = TRUE; List->psz; List++)
            if ((SHORT)WinSendMsg( hCtl, LM_INSERTITEM, (MP)LIT_END,
                                   (MP)List->psz) < 0)
            {
                fRtn = FALSE;
                break;
            }
    }

    return (fRtn);
}

/****************************************************************************/

// there are too many types to list them all at once;  this switches between
// the two lists depending on whether RWSP_CONV is selected

void        SwitchTypeLists( HWND hwnd, HWND hCB)

{
    BOOL        fConvert;
    SHORT       ndx;

do
{
    ndx = (SHORT)WinSendMsg( hCB, LM_QUERYSELECTION, (MP)(USHORT)LIT_FIRST, 0);
    if (ndx < 0)
        break;

    WinSendDlgItemMsg( hwnd, IDC_PROCXDS, LM_SELECTITEM,
                       (MP)ProcList[ndx].ndx, (MP)TRUE);

    if (ProcList[ndx].ul == RWSP_CONV)
        fConvert = TRUE;
    else
        fConvert = FALSE;

    if (fConv == fConvert)
        break;

    fConv = fConvert;

    if (fConvert)
    {
        FillList( hwnd, IDC_ARG1TYPE, ConvList);
        FillList( hwnd, IDC_ARG2TYPE, ConvList);
        FillList( hwnd, IDC_ARG3TYPE, ConvList);
        FillList( hwnd, IDC_ARG4TYPE, ConvList);
        FillList( hwnd, IDC_ARG5TYPE, ConvList);
        FillList( hwnd, IDC_ARG6TYPE, ConvList);
    }
    else
    {
        FillList( hwnd, IDC_ARG1TYPE, ArgList);
        FillList( hwnd, IDC_ARG2TYPE, ArgList);
        FillList( hwnd, IDC_ARG3TYPE, ArgList);
        FillList( hwnd, IDC_ARG4TYPE, ArgList);
        FillList( hwnd, IDC_ARG5TYPE, ArgList);
        FillList( hwnd, IDC_ARG6TYPE, ArgList);
    }

    WinSendDlgItemMsg( hwnd, IDC_ARG1TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG2TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG3TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG4TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG5TYPE, LM_SELECTITEM, 0, (MP)TRUE);
    WinSendDlgItemMsg( hwnd, IDC_ARG6TYPE, LM_SELECTITEM, 0, (MP)TRUE);

} while (fFalse);

    return;
}

/****************************************************************************/

// sets the hex/decimal/string selection depending on the selected type

void        SetXDS( HWND hwnd, HWND hCB, ULONG ulID)

{
    SHORT       ndx;

    ndx = (SHORT)WinSendMsg( hCB, LM_QUERYSELECTION, (MP)(USHORT)LIT_FIRST, 0);
    if (ndx >= 0)
    {
        if (fConv)
            WinSendDlgItemMsg( hwnd, ulID+3, LM_SELECTITEM,
                               (MP)ConvList[ndx].ndx, (MP)TRUE);
        else
            WinSendDlgItemMsg( hwnd, ulID+3, LM_SELECTITEM,
                               (MP)ArgList[ndx].ndx, (MP)TRUE);
    }

    return;
}

/****************************************************************************/

// gathers user input then submits it to RwsClient

void        Run( HWND hwnd)

{
    ULONG       rc;
    ULONG   	ctr;
    ULONG   	ulID;
    PRWSHDR     pHdr = 0;
    PRWSBLD     pBld;
    PRWSARG     pArg;
    char *      pBuf;
    char *      pErr = 0;

do
{
    ctr = sizeof(RWSBLD) + 6*sizeof(RWSARG);
    memset( (PVOID)argbuf, 0, ctr);

    pBld = (PRWSBLD)argbuf;
    pArg = CALCARGPTR( pBld);
    pBuf = &argbuf[ctr];

    pBld->ppHdr = &pHdr;

    if (GetProc( hwnd, pBld, &pBuf) == FALSE)
        ERRBREAK( "proc is missing or invalid")

    if (GetRtn( hwnd, pBld) == FALSE)
        ERRBREAK( "invalid return type")

    for (ctr=0, ulID=IDC_ARG1TYPE; ctr < 6; ctr++, ulID += 4)
    {
        rc = GetArg( hwnd, &pArg[ctr], ulID, &pBuf);
        if (rc)
            break;
        pBld->argCnt++;
    }
    if (rc == 2)
    {
        sprintf( rtnbuf, "\r\nArg%d is invalid\r\n", ctr+1);
        pErr = rtnbuf;
        break;
    }

    rc = RwsCallIndirect( pBld);
    if (rc)
    {
        pErr = rtnbuf + sprintf( rtnbuf, "\r\nRwsCallIndirect rc= %d (", rc);
        RwsGetRcString( rc, 256, pErr);
        strcat( pErr, ")\r\n");
        pErr = rtnbuf;
        break;
    }

    ShowResults( hwnd, pBld);

} while (fFalse);

    RwsFreeMem( pHdr);

    if (pErr)
    {
        WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_SETSEL, (MP)0x7fffffff, (MP)0x7fffffff);
        WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_SETFIRSTCHAR, (MP)0x7fffffff, 0);
        WinSendDlgItemMsg( hwnd, IDC_RESULT, MLM_INSERT, (MP)pErr, 0);
    }

    return;
}

/****************************************************************************/

// figure out the type of procedure request & interpret the input

BOOL        GetProc( HWND hwnd, PRWSBLD pBld, char ** ppBuf)

{
    BOOL        fRtn = FALSE;
    ULONG       ul;
    SHORT       ndx;

do
{
    ndx = (SHORT)WinSendDlgItemMsg( hwnd, IDC_PROCTYPE, LM_QUERYSELECTION,
                                    (MP)(USHORT)LIT_FIRST, 0);
    if (ndx < 0)
        break;

    pBld->callType = ProcList[ndx].ul;

    if (pBld->callType == RWSP_CONV)
    {
        pBld->callValue = 0;
        fRtn = TRUE;
        break;
    }

    ul = WinQueryDlgItemText( hwnd, IDC_PROCNAME, RWSSTRINGSIZE, *ppBuf);

    ndx = (SHORT)WinSendDlgItemMsg( hwnd, IDC_PROCXDS, LM_QUERYSELECTION,
                                    (MP)(USHORT)LIT_FIRST, 0);
    if (ndx < 0 || ndx > XDS_STR)
        break;

    if (ndx == XDS_HEX)
        pBld->callValue = (ULONG)strtoul( *ppBuf, 0, 16);
    else
    if (ndx == XDS_DEC)
        pBld->callValue = (ULONG)strtol( *ppBuf, 0, 10);
    else
    {
        pBld->callValue = (ULONG)*ppBuf;
        *ppBuf += ul + 1;
        if (*(char*)pBld->callValue == '_')
            pBld->callValue++;
    }

    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

// figure out the type of return

BOOL        GetRtn( HWND hwnd, PRWSBLD pBld)

{
    BOOL        fRtn = FALSE;
    SHORT       ndx;

do
{
    ndx = (SHORT)WinSendDlgItemMsg( hwnd, IDC_RTNTYPE, LM_QUERYSELECTION,
                                    (MP)(USHORT)LIT_FIRST, 0);
    if (ndx < 0)
        break;

    pBld->rtnType = pszulRtn[ndx].ul;

    if (WinQueryDlgItemShort( hwnd, IDC_RTNSIZE,
                              (PSHORT)(PVOID)&pBld->rtnSize, FALSE) == FALSE)
        pBld->rtnSize = 0;
    else
        pBld->rtnSize &= (ULONG)0xffff;

    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

// get each arg & interpret the input based on the hex/decimal/string field

ULONG       GetArg( HWND hwnd, PRWSARG pArg, ULONG ulID, char ** ppBuf)

{
    ULONG       rc = 2;
    ULONG       ul;
    char *      ptr;
    SHORT       ndx;

do
{
    ndx = (SHORT)WinSendDlgItemMsg( hwnd, ulID, LM_QUERYSELECTION,
                                    (MP)(USHORT)LIT_FIRST, 0);

    if (ndx <= 0)
    {
        if (ndx == 0)
            rc = 1;
        break;
    }

    if (fConv)
        pArg->type = ConvList[ndx].ul;
    else
        pArg->type = ArgList[ndx].ul;

    if (WinQueryDlgItemShort( hwnd, ulID+1, (PSHORT)(PVOID)&(pArg->size),
                              FALSE) == FALSE)
        pArg->size = 0;

    ul = WinQueryDlgItemText( hwnd, ulID+2, RWSSTRINGSIZE, *ppBuf);

    ndx = (SHORT)WinSendDlgItemMsg( hwnd, ulID+3, LM_QUERYSELECTION,
                                    (MP)(USHORT)LIT_FIRST, 0);
    if (ndx < 0 || ndx > XDS_STR)
        break;

    if (ndx == XDS_HEX)
        pArg->value = (ULONG)strtoul( *ppBuf, 0, 16);
    else
    if (ndx == XDS_DEC)
        pArg->value = (ULONG)strtol( *ppBuf, 0, 10);
    else
    {
        if (ul == 0)
            break;
        pArg->value = (ULONG)*ppBuf;
        *ppBuf += ul + 1;
        ptr = strchr( (char*)pArg->value, '\\');
        if (ptr && ptr[1] == 'n')
        {
            ptr[0] = 0x0d;
            ptr[1] = 0x0a;
        }
    }

    rc = 0;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// a cheesy display routine

void        ShowResults( HWND hwnd, PRWSBLD pBld)

{
    ULONG       ul;
    ULONG       size;
    ULONG       ctr;
    ULONG       ndx;
    PULONG      ptr;
    PRWSHDR     pHdr;
    HWND        hCtl;

do
{
    hCtl = WinWindowFromID( hwnd, IDC_RESULT);
    WinSendMsg( hCtl, MLM_SETSEL, (MP)0x7fffffff, (MP)0x7fffffff);
    WinSendMsg( hCtl, MLM_INSERT, (MP)"\r\n", 0);

    pHdr = *pBld->ppHdr;
    sprintf( rtnbuf, "pHdr= %x\r\n", pHdr);
    WinSendMsg( hCtl, MLM_INSERT, (MP)rtnbuf, 0);

    for (ctr=0; ctr <= pHdr->Cnt; ctr++)
    {
        ul = RwsGetResult( pHdr, ctr, &size);

        if (ul == (ULONG)-1)
            sprintf( rtnbuf, "arg%d - Error getting result!\r\n", ctr);
        else
        if (size == (ULONG)-1)
            sprintf( rtnbuf, "arg%d - string= %s\r\n", ctr, ul);
        else
        if (size == 0)
            sprintf( rtnbuf, "arg%d - value= %x\r\n", ctr, ul);
        else
        {
            sprintf( rtnbuf, "arg%d - size= %d  value*= %x\r\n", ctr, size, ul);

            // this is pretty poor
            if (size < 256)
                for (ndx=0, ptr=(PULONG)ul; ndx < size; ndx+=16, ptr+=4)
                {
                    WinSendMsg( hCtl, MLM_INSERT, (MP)rtnbuf, 0);
                    sprintf( rtnbuf, "%08x %08x %08x %08x\r\n",
                             ptr[0], ptr[1], ptr[2], ptr[3]);
                }
        }
        WinSendMsg( hCtl, MLM_INSERT, (MP)rtnbuf, 0);
    }

} while (fFalse);

    return;
}

/****************************************************************************/

