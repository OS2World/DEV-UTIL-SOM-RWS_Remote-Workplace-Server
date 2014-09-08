/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSSOMK.H
// Remote Workplace Server - som.dll ordinals

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
 * The Original Code is Remote Workplace Server - Project headers.
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

// SOM Kernel functions exported by ordinal
// taken from a som.dll dated 1999-06-30, size 92,009

/****************************************************************************/

#define SOMK_SOMCalloc                              1
#define SOMK_SOMClassCClassData                     2
#define SOMK_SOMClassClassData                      3
#define SOMK_SOMClassInitFuncName                   4
#define SOMK_SOMClassMgrCClassData                  5
#define SOMK_SOMClassMgrClassData                   6
#define SOMK_SOMClassMgrObject                      7
#define SOMK_SOMDeleteModule                        8
#define SOMK_SOMError                               9
#define SOMK_SOMFree                               10
#define SOMK_SOMLoadModule                         11
#define SOMK_SOMMalloc                             12
#define SOMK_SOMObjectCClassData                   13
#define SOMK_SOMObjectClassData                    14
#define SOMK_SOMOutCharRoutine                     15
#define SOMK_SOMRealloc                            16
#define SOMK_SOM_AssertLevel                       17
#define SOMK_SOM_TraceLevel                        18
#define SOMK_SOM_WarnLevel                         19
#define SOMK_SOMClassMgrNewClass                   20
#define SOMK_SOMClassNewClass                      21
#define SOMK_SOMObjectNewClass                     22
#define SOMK_somAssert                             23
#define SOMK_somBeginPersistentIds                 24
#define SOMK_somCheckArgs                          25
#define SOMK_somCheckId                            26
#define SOMK_somCompareIds                         27
#define SOMK_somConstructClass                     28
#define SOMK_somEndPersistentIds                   29
#define SOMK_somEnvironmentNew                     30
#define SOMK_somIdFromString                       31
#define SOMK_somLPrintf                            32
#define SOMK_somParentResolve                      33
#define SOMK_somPrefixLevel                        34
#define SOMK_somPrintf                             35
#define SOMK_somRegisterId                         36
#define SOMK_somResolve                            37
#define SOMK_somSaveMetrics                        38
#define SOMK_somSetExpectedIds                     39
#define SOMK_somStringFromId                       40
#define SOMK_somTest                               41
#define SOMK_somTestCls                            42
#define SOMK_somTotalRegIds                        43
#define SOMK_somUniqueKey                          44
#define SOMK_somVprintf                            45
#define SOMK_somWriteMetrics                       46
#define SOMK_somDataResolve                        47
#define SOMK_somClassResolve                       48
#define SOMK_somBuildClass                         49
#define SOMK_somParentNumResolve                   50
#define SOMK_somCreateDynamicClass                 51
#define SOMK_somExceptionId                        52
#define SOMK_somExceptionValue                     53
#define SOMK_somExceptionFree                      54
#define SOMK_somSetException                       55
#define SOMK_SOM_IdTable                           56
#define SOMK_SOM_IdTableSize                       57
#define SOMK_somGetGlobalEnvironment               58
#define SOMK_somIsObj                              60
#define SOMK_somResolveByName                      61
#define SOMK_somStartCriticalSection               62
#define SOMK_somEndCriticalSection                 63
#define SOMK_va_SOMObject_somDispatchA             64
#define SOMK_va_SOMObject_somDispatchD             65
#define SOMK_va_SOMObject_somDispatchL             66
#define SOMK_va_SOMObject_somDispatchV             67
#define SOMK_va_SOMObject_somDispatch              68
#define SOMK_somApply                              69
#define SOMK_SOM_MajorVersion                      70
#define SOMK_SOM_MinorVersion                      71
#define SOMK_somDataResolveChk                     72
#define SOMK_somAncestorResolve                    74
#define SOMK_somfixMsgTemplate                     80
#define SOMK_somGetClassFromMToken                 82
#define SOMK_somEnvironmentEnd                     83
#define SOMK_somAbnormalEnd                        84
#define SOMK_somSetOutChar                         85
#define SOMK_somRegisterClassLibrary               86
#define SOMK_SOMParentDerivedMetaclassClassData    87
#define SOMK_somMainProgram                        88
#define SOMK_somUnregisterClassLibrary             89
#define SOMK_SOMCreateMutexSem                     90
#define SOMK_SOMRequestMutexSem                    91
#define SOMK_SOMReleaseMutexSem                    92
#define SOMK_SOMDestroyMutexSem                    93
#define SOMK_SOMGetThreadId                        94
#define SOMK_SOM_MaxThreads                        95
#define SOMK_somva_SOMObject_somDispatchA          96
#define SOMK_somva_SOMObject_somDispatchD          97
#define SOMK_somva_SOMObject_somDispatchL          98
#define SOMK_somva_SOMObject_somDispatchV          99
#define SOMK_somva_SOMObject_somDispatch          100
#define SOMK_somva_SOMObject_somClassDispatch     101
#define SOMK_somFreeThreadData                    132
#define SOMK_somResolveTerminal                   133
#define SOMK_somIdMarshal                         135
#define SOMK_somMakeUserRdStub                    361
#define SOMK_somPCallResolve                      362

/****************************************************************************/

