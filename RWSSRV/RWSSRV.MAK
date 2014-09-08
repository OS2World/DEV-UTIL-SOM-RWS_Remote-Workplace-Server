#****************************************************************************/
#* RWS - beta version 0.80                                                  */
#****************************************************************************/
#
#  RWSSRV.MAK
#  Remote Workplace Server - Server component
#
#  tools used:  VACPP v3.65, ILINK, RC, and NMAKE32
#
#****************************************************************************/
#
#* ***** BEGIN LICENSE BLOCK *****
#* Version: MPL 1.1
#*
#* The contents of this file are subject to the Mozilla Public License Version
#* 1.1 (the "License"); you may not use this file except in compliance with
#* the License. You may obtain a copy of the License at
#* http://www.mozilla.org/MPL/
#*
#* Software distributed under the License is distributed on an "AS IS" basis,
#* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
#* for the specific language governing rights and limitations under the
#* License.
#*
#* The Original Code is Remote Workplace Server - Server makefile.
#*
#* The Initial Developer of the Original Code is Richard L. Walsh.
#* 
#* Portions created by the Initial Developer are Copyright (C) 2004-2007
#* the Initial Developer. All Rights Reserved.
#*
#* Contributor(s):
#*
#* ***** END LICENSE BLOCK ***** */
#
#****************************************************************************/
#
#   FYI... the @ECHOs below insert a blank line before each command 
#          is executed;  this makes the output a little easier to read

.SUFFIXES:

.SUFFIXES: .c .rc

ALL:            \
  RWSSRV08.DLL  \
  RWSSRV.RES

RWSSRV08.DLL:   \
  RWSCLS.OBJ    \
  RWSSUTIL.OBJ  \
  RWSSRV.OBJ    \
  RWSSCMD.OBJ   \
  RWSSRV.RES    \
  RWSSRV.DEF    \
  RWSSRV.MAK
    @ECHO
    ILINK.EXE /NOLOGO @<<
    /EXEPACK:2 /MAP /OPTFUNC /PACKDATA /SEGMENTS:64 /NOSEGORDER
    /OUT:$@
    $[m,*.OBJ,$**]
    $[m,*.DEF,$**]
    SOMTK.LIB
<<
    @ECHO
    RC.EXE -n -x2 $[m,*.RES,$**] $@

RWSSRV.RES:     \
  RWSSRV.RC     \
  RWSSRV.ICO    \
  RWSSRV.MAK

RWSCLS.OBJ:     \
  RWSCLS.c      \
  RWSCLS.IH     \
  RWSCLS.h      \
  RWSSRV.MAK

RWSSUTIL.OBJ:   \
  RWSSUTIL.C    \
  RWSSRV.H      \
  ..\RWS.H      \
  ..\RWSERR.H   \
  RWSSRV.MAK

RWSSRV.OBJ:     \
  RWSSRV.C      \
  RWSSRV.H      \
  ..\RWS.H      \
  ..\RWSERR.H   \
  RWSSRV.MAK

RWSSCMD.OBJ:    \
  RWSSCMD.C     \
  RWSSRV.H      \
  ..\RWS.H      \
  ..\RWSERR.H   \
  RWSSRV.MAK

.rc.res:
    @ECHO
    RC.EXE -n -r $<

.c.obj:
    @ECHO
    ICC.EXE /C /Ge- /Gs /Ms /Q /Rn /Wall+ppt-ppc-por-trd-uni-par-ext- $<

