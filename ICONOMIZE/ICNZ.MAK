#****************************************************************************/
#* RWS - beta version 0.80                                                  */
#****************************************************************************/
#
#  ICNZ.MAK
#  Remote Workplace Server - demo program "Iconomize"
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
#* The Original Code is Remote Workplace Server - "Iconomize" makefile.
#*
#* The Initial Developer of the Original Code is Richard L. Walsh.
#* 
#* Portions created by the Initial Developer are Copyright (C) 2004
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
  ICONOMIZE.EXE \
  ICNZ.RES

ICONOMIZE.EXE:  \
  ICNZMAIN.OBJ  \
  ICNZCOL.OBJ   \
  ICNZCMD.OBJ   \
  ICNZ.RES      \
  ICNZ.DEF      \
  ICNZ.MAK
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

ICNZ.RES:       \
  ICNZ.RC       \
  ICNZ.DLG      \
  ICNZRC.H      \
  ICNZ.ICO      \
  ICNZERR.ICO   \
  ICNZSORT.PTR  \
  ICNZDOWN.BMP  \
  ICNZUP.BMP    \
  ICNZ.MAK

ICNZMAIN.OBJ:   \
  ICNZMAIN.C    \
  ICNZ.H        \
  ICNZRC.H      \
  ..\RWS.H      \
  ICNZ.MAK

ICNZCOL.OBJ:    \
  ICNZCOL.C     \
  ICNZ.H        \
  ICNZRC.H      \
  ..\RWS.H      \
  ICNZ.MAK

ICNZCMD.OBJ:    \
  ICNZCMD.C     \
  ICNZ.H        \
  ICNZRC.H      \
  ..\RWS.H      \
  ICNZ.MAK

.rc.res:
    @ECHO
    RC.EXE -n -r $<

.c.obj:
    @ECHO
    ICC.EXE /C /Gs /Ms /Q /Wall+ppt-ppc-por-trd-uni-par-ext- $<

