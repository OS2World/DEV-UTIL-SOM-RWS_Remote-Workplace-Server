#****************************************************************************/
#* RWS - beta version 0.80                                                  */
#****************************************************************************/
#
#  RWSCLI.MAK
#  Remote Workplace Server - Client component
#
#  tools used:  VACPP v3.65, ILINK, ILIB, and NMAKE32
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
#* The Original Code is Remote Workplace Server - Client makefile.
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
  RWSCLI08.DLL  \
  RWSCLI.RES

RWSCLI08.DLL:   \
  RWSCLI.OBJ    \
  RWSCUTIL.OBJ  \
  RWSCLI.RES    \
  RWSCLI.DEF    \
  RWSCLI.MAK
    @ECHO
    ILINK.EXE /NOLOGO @<<
    /EXEPACK:2 /MAP /OPTFUNC /PACKDATA /SEGMENTS:64 /NOSEGORDER
    /OUT:$@
    $[m,*.OBJ,$**]
    $[m,*.DEF,$**]
<<
    @ECHO
    RC.EXE -n -x2 $[m,*.RES,$**] $@
    @ECHO
    ILIB.EXE /NOLOGO /GENIMPLIB $[m,*.DEF,$**]

RWSCLI.RES:     \
  RWSCLI.RC     \
  ..\RWSERR.H   \
  RWSCLI.MAK

RWSCLI.OBJ:     \
  RWSCLI.C      \
  RWSCLI.H      \
  ..\RWS.H      \
  ..\RWSERR.H   \
  RWSCLI.MAK

RWSCUTIL.OBJ:   \
  RWSCUTIL.C    \
  RWSCLI.H      \
  ..\RWS.H      \
  ..\RWSERR.H   \
  RWSCLI.MAK

.rc.res:
    @ECHO
    RC.EXE -n -r $<

.c.obj:
    @ECHO
    ICC.EXE /C /Ge- /Gs /Ms /Q /Rn /Wall+ppt-ppc-por-trd-uni-par-ext- $<

