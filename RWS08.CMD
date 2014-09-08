/*-------------------------------------------------------------------*/
/* RWS08.CMD                                                         */
/* REXX script to register/ deregister the "RWS08" WPS class         */
/*                                                                   */
/* Remote Workplace Server                                           */
/* (C)Copyright 2004-2005, 2007  R.L.Walsh - all rights reserved     */
/*                                                                   */
/*-------------------------------------------------------------------*/

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
call SysLoadFuncs

say ''
say 'Rws08.cmd registers and/or deregisters the RWS08 WPS class'
say 'used by Remote Workplace Server v0.80.  Would you like to:'
say ''
say '   1 - deregister RWS08'
say '   2 - register RWS08'
say '   X - exit without any changes'
say ''

do FOREVER
    call charout ,'Enter 1, 2, or X:  '
    pull answer

    if answer = '1' then
      do
        call Dereg
        leave
      end
    else
    if answer = '2' then
      do
        call Reg
        leave
      end
    else
    if answer = 'X' then
        leave
end
exit

/*-------------------------------------------------------------------*/

Dereg : procedure

    /* deregister RWS08, regardless of whether it's actually present */

    Call SysDeregisterObjectClass 'RWS08'
    return

/*-------------------------------------------------------------------*/

Reg : procedure

    say ''
    Call Dereg
    rc = 0

    /* look for the dll in the current directory */
    newpath = Stream('rwssrv08.dll', 'c', 'query exists')
    if newpath <> '' then
        rc = SysRegisterObjectClass('RWS08',newpath)

    /* if that failed, look on the LIBPATH */
    if rc = 0 then
        rc = SysRegisterObjectClass('RWS08',rwssrv08)

    /* if registration failed, the class list  */
    /* contains an invalid entry, so remove it */
    if rc = 0 then
      do
        Call Dereg
        say 'Unable to register class RWS08 because rwssrv08.dll'
        say 'could not be found in the current directory or LIBPATH.'
      end

    /* if registration succeeded, display the        */
    /* fully-qualified name of the dll that was used */
    else
      do
        call SysQueryClassList "list."
        do ndx = list.0 to 1 by -1
            parse var list.ndx class newpath
            if class = 'RWS08' then
                leave
        end
        say 'Successfully registered class RWS08 using' newpath
      end

    /* give the user a chance to read the message */
    say ''
    say 'press Enter to continue...'
    pull
    return

/*-------------------------------------------------------------------*/

