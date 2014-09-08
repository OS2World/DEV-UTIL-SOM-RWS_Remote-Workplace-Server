#****************************************************************************/
#* RWS - beta version 0.80                                                  */
#****************************************************************************/
#
#  RWSTEST.MAK
#  Remote Workplace Shell - demo program "RwsTest"
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
#* The Original Code is Remote Workplace Server - "RwsTest" makefile.
#*
#* The Initial Developer of the Original Code is Richard L. Walsh.
#* 
#* Portions created by the Initial Developer are Copyright (C) 2004, 2005
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
  RWSTEST.EXE   \
  RWSTEST.RES

RWSTEST.EXE:    \
  RWSTEST.OBJ   \
  RWSTEST.RES   \
  RWSTEST.DEF   \
  RWSTEST.MAK
    @ECHO
    ILINK.EXE /NOLOGO @<<
    /EXEPACK:2 /MAP /OPTFUNC /PACKDATA /SEGMENTS:64 /NOSEGORDER
    /STACK:0xf000 /PM:PM
    /OUT:$@
    $[m,*.OBJ,$**]
    $[m,*.DEF,$**]
    ..\RWSCLI08.LIB
<<
    @ECHO
    RC.EXE -n -x2 $[m,*.RES,$**] $@

RWSTEST.RES:    \
  RWSTEST.RC    \
  RWSTEST.DLG   \
  RWSTEST.ICO   \
  RWSTEST.H     \
  RWSTEST.MAK

RWSTEST.OBJ:    \
  RWSTEST.C     \
  RWSTEST.H     \
  ..\RWS.H      \
  RWSTEST.MAK

.rc.res:
    @ECHO
  	RC.EXE -n -r $<

.c.obj:
    @ECHO
    ICC.EXE /C /Gs /Ms /Q /Wall+ppt-ppc-por-trd-uni-par-ext- $<

