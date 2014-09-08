/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSTEST.H
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

#define MP              MPARAM

#define XDS_HEX         0
#define XDS_DEC         1
#define XDS_STR         2

#define ERRBREAK(x)     { pErr = "\r\n"x"\r\n"; break; }

typedef struct _PSZUL
{
    char *      psz;
    ULONG       ul;
} PSZUL;

typedef struct _PSZULNDX
{
    char *      psz;
    ULONG       ul;
    ULONG       ndx;
} PSZULNDX;


/****************************************************************************/

#define IDI_RWSTEST             1

#define IDD_MAIN                101

#define IDC_OK                  111
#define IDC_CLEAR               112

#define IDC_PROCTYPE            201
#define IDC_PROCNAME            203
#define IDC_PROCXDS             204
#define IDC_RTNTYPE             205
#define IDC_RTNSIZE             206

#define IDC_ARG1TYPE            211
#define IDC_ARG1SIZE            212
#define IDC_ARG1DATA            213
#define IDC_ARG1XDS             214

#define IDC_ARG2TYPE            215
#define IDC_ARG2SIZE            216
#define IDC_ARG2DATA            217
#define IDC_ARG2XDS             218

#define IDC_ARG3TYPE            219
#define IDC_ARG3SIZE            220
#define IDC_ARG3DATA            221
#define IDC_ARG3XDS             222

#define IDC_ARG4TYPE            223
#define IDC_ARG4SIZE            224
#define IDC_ARG4DATA            225
#define IDC_ARG4XDS             226

#define IDC_ARG5TYPE            227
#define IDC_ARG5SIZE            228
#define IDC_ARG5DATA            229
#define IDC_ARG5XDS             230

#define IDC_ARG6TYPE            231
#define IDC_ARG6SIZE            232
#define IDC_ARG6DATA            233
#define IDC_ARG6XDS             234

#define IDC_RESULT              235

/****************************************************************************/

