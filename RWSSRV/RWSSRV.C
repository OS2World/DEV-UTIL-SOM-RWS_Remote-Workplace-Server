/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSSRV.C
// Remote Workplace Server - Server core functions

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
 * The Original Code is Remote Workplace Server - Server component.
 *
 * The Initial Developer of the Original Code is Richard L. Walsh.
 * 
 * Portions created by the Initial Developer are Copyright (C) 2004-2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

/****************************************************************************/

// This is the server component of RWS.  It is invoked solely via IPC.
// Currently, it uses a single message posted to its object-window by
// the client.  The msg params identify a block of gettable shared
// memory containing the request and the window which originated it.

// It interprets the data passed to it, invokes the requested method or
// function, then interprets the return value and any in/out arguments.
// If the request is for data conversion only, it interprets the input
// data, then reinterprets each argument as though it were the value
// returned by a function (e.g. objectID -> object -> title).

// When complete, it posts a reply to the originating window containing
// a return code and a pointer to the memory.  Upon successful completion,
// the data block contains *all* data used in the transaction, both raw
// and interpreted.  This allows the client to reuse data such as object
// and function pointers and avoid the overhead of repeated conversions.

/****************************************************************************/

#include "RWSSRV.H"

/****************************************************************************/

ULONG           Resolve( PRWSHDR pHdr, HWND hReply);
ULONG           ResolveCallType( PRWSHDR pHdr);
ULONG           ResolveConvert( PRWSHDR pHdr, HWND hReply);
ULONG           ResolveFunction( PRWSHDR pHdr, HWND hReply);

ULONG           Execute( PRWSDSC pProc, PRWSDSC pRtn, ULONG cntArg);
ULONG           ResolveProc( PRWSDSC pProc, PRWSDSC pArg1);
ULONG           ResolveArgIn( PRWSDSC pArg,  PRWSDSC pArg1);
ULONG           ResolveArgOut( PRWSDSC pArg, PRWSDSC pRtn, HWND hReply);
ULONG           ResolveRtn( PRWSDSC pArg,  PRWSDSC pRtn, HWND hReply);
ULONG           OutGetString( PRWSDSC pArg, PRWSDSC pRtn);
ULONG           OutGetBuffer( PRWSDSC pArg, PRWSDSC pRtn);
ULONG           OutGetCnt( PBYTE pBuf,  PRWSDSC pRtn, ULONG ndx);
ULONG           OutGetConvert( ULONG ulValue, PRWSDSC pArg, HWND hReply);
BOOL            OutGetIcon( HPOINTER hIcon, HPOINTER * pBuf,
                            HWND hClient, BOOL fMini);

SOMObject *     ObjFromHandle( HOBJECT hobj);
SOMObject *     ObjFromPath( PSZ pszName);
SOMObject *     ObjFromCnrRec( ULONG ulItemID);
SOMObject *     ObjFromWindow( HWND hSrc);
SOMObject *     ObjFromShadow( SOMObject * pShadow);
SOMObject *     ObjFromFQTitle( PSZ pszName, BOOL fShadow);
SOMClass *      ClassFromObj( SOMObject * pObj);
SOMClass *      ClassFromName( PSZ pszClass);

HWND            ObjToHwnd( SOMObject * pObj);
BOOL            ObjToName( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            ObjToPath( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            ObjToObjID( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            ObjToTitle( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            ObjToFolder( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            ObjToFQTitle( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            ClassToName( SOMObject * pObj, char * pBuf, ULONG cbBuf);
BOOL            SomIDToString( somId id, char * pBuf, ULONG cbBuf);
BOOL            ObjToIcon( SOMObject * pObj, HPOINTER * pBuf,
                           HWND hClient, BOOL fMini);

// suppress an "unreferenced static variable" warning
void *          SrvDummy( void) { return (void*)__somtmp_SOMObject; }

/****************************************************************************/
/* Primary Functions                                                        */
/****************************************************************************/

// this top-level function does basic validation,
// then passes the call to the appropriate handler

ULONG           Resolve( PRWSHDR pHdr, HWND hReply)

{
    ULONG           rc;
    BOOL            fJump = FALSE;
    RWSXCPTREGREC   rwsregrec;

do
{
    // if exception handling hasn't been turned off, setup our handler
    if (fUseHandler)
    {
        rwsregrec.regrec.prev_structure = 0;
        rwsregrec.regrec.ExceptionHandler = &RwsExceptionHandler;

        // fill the jmp_buf in our expanded registration record;
        // if we're here because longjmp was called, rc will be
        // RWSSRV_EXCEPTION

        rc = setjmp( rwsregrec.jmpbuf);
        if (rc)
        {
            fJump = TRUE;
            somPrintf( RWSSRVNAME ": Access violation handled in Resolve()\n");
            break;
        }

        // if we can't set the handler, abort
        if (DosSetExceptionHandler( &rwsregrec.regrec))
            ERRNBR( RWSSRV_SETHANDLER)
    }

    // validate arg count & proc type
    ERRRTN( ResolveCallType( pHdr))

    // call a handler based on the proc type
    switch (GIVEP( (CALCPROCPTR( pHdr))->type))
    {
        case PROC_CONV:
            rc = ResolveConvert( pHdr, hReply);
            break;

        case PROC_CMD:
            rc = ResolveCommand( pHdr, hReply);
            break;

        default:
            rc = ResolveFunction( pHdr, hReply);
            break;
    }

} while (fFalse);

    // save the result code in the header
    if (!(rc & RWSF_NOREPLY))
        pHdr->Rc = LOUSHORT(rc);

    // unset the exception handler;  this may produce a spurious error
    // if an exception occured and longjmp() already unwound it, so
    // suppress the error msg if a longjmp occurred
    if (fUseHandler && DosUnsetExceptionHandler( &rwsregrec.regrec) && !fJump)
        somPrintf( RWSSRVNAME ": Resolve - DosUnsetExceptionHandler failed\n");

    return (rc);
}

/****************************************************************************/

// validates the arg count so we don't have to worry about it again,
// then does basic validation for each type of call

ULONG           ResolveCallType( PRWSHDR pHdr)

{
    ULONG       rc = 0;
    ULONG       ctr;
    PRWSDSC     pProc;
    PRWSDSC     pArg;

do
{
    pProc = CALCPROCPTR( pHdr);

    // starting at the return descriptor, confirm the arg chain
    // exactly matches the arg count (note:  return is "arg[0]")
    for (ctr=0, pArg=pProc->pnext; ctr < pHdr->Cnt && pArg; ctr++)
        pArg = pArg->pnext;

    // if there are too few or too many args, exit
    if (pArg == 0 || pArg->pnext != 0)
        ERRNBR( RWSSRV_BADARGCNT)

    // this descriptor should be identified appropriately
    if (pProc->flags != DSC_PROC)
        ERRNBR( RWSSRV_PROCBADDSC)

    // do basic validation of the procedure types
    // coding note:  the goal here is to make the requirements
    // for each proc type perfectly clear, not to save space
    switch (GIVEP( pProc->type))
    {
        // the name of a method that will be resolved later
        case PROC_MNAM:
            if (pHdr->Cnt > RWSMAXARGS)
                ERRNBR( RWSSRV_PROCTOOMANYARGS)
            if (pHdr->Cnt == 0)
                ERRNBR( RWSSRV_PROCTOOFEWARGS)
            if (GIVEQ( pProc->type) > PRTN_LASTGIVEQ)
                ERRNBR( RWSSRV_PROCBADGIVEQ)
            if (*CALCGIVEPTR( pProc) == 0)
                ERRNBR( RWSSRV_PROCNOMETHODNAME)
            break;

        // the ordinal for a function in the SOM kernel
        // (i.e. som.dll) that will be resolved later
        case PROC_KORD:
            if (pHdr->Cnt > RWSMAXARGS)
                ERRNBR( RWSSRV_PROCTOOMANYARGS)
            if (GIVEQ( pProc->type) > PRTN_LASTGIVEQ)
                ERRNBR( RWSSRV_PROCBADGIVEQ)
            if (*(PULONG)CALCGIVEPTR( pProc) == 0)
                ERRNBR( RWSSRV_PROCNOORD)
            break;

        // a data conversion call
        case PROC_CONV:
            if (pHdr->Cnt == 0)
                ERRNBR( RWSSRV_PROCTOOFEWARGS)
            if (GIVEQ( pProc->type) != PRTN_NA)
                ERRNBR( RWSSRV_PROCBADGIVEQ)
            break;

        // an RWS command
        case PROC_CMD:
            if (GIVEQ( pProc->type) != PRTN_NA)
                ERRNBR( RWSSRV_PROCBADGIVEQ)
            if (pProc->value < RWSCMD_FIRST || pProc->value > RWSCMD_LAST)
                ERRNBR( RWSSRV_PROCBADCMD)
            break;

        // pointer to a function that implements a method
        case PROC_MPFN:
            if (pHdr->Cnt > RWSMAXARGS)
                ERRNBR( RWSSRV_PROCTOOMANYARGS)
            if (pHdr->Cnt == 0)
                ERRNBR( RWSSRV_PROCTOOFEWARGS)
            if (GIVEQ( pProc->type) > PRTN_LASTGIVEQ)
                ERRNBR( RWSSRV_PROCBADGIVEQ)
            if (pProc->value == 0)
                ERRNBR( RWSSRV_PROCNOPFN)
            break;

        // pointer to a function in the SOM kernel (i.e. som.dll)
        case PROC_KPFN:
            if (pHdr->Cnt > RWSMAXARGS)
                ERRNBR( RWSSRV_PROCTOOMANYARGS)
            if (GIVEQ( pProc->type) > PRTN_LASTGIVEQ)
                ERRNBR( RWSSRV_PROCBADGIVEQ)
            if (pProc->value == 0)
                ERRNBR( RWSSRV_PROCNOPFN)
            break;

        // anything else is an error
        default:
            rc = RWSSRV_PROCBADTYPE;
            break;
    }

} while (fFalse);

    pProc->rc = LOUSHORT(rc);

    return (rc);
}

/****************************************************************************/

// handles data conversion:  input arguments are converted (typically into
// object pointers), then reconverted as though each were the value returned
// by a function - there is no procedure call;  this function's return code
// is determined by examining the return code for each individual conversion
// and counting the number that were successful;  this permits some to fail
// without the entire call failing;  the function's return value is this count

ULONG           ResolveConvert( PRWSHDR pHdr, HWND hReply)

{
    ULONG           rc = 0;
    BOOL            fJump = FALSE;
    ULONG           ctr;
    PRWSDSC         pRtn;
    PRWSDSC         pArg1;
    PRWSDSC         pArg;
    RWSXCPTREGREC   rwsregrec;

do
{
    // if exception handling is on, init our expanded registration record,
    // then set the handler;  if that fails, abort
    if (fUseHandler)
    {
        rwsregrec.regrec.prev_structure = 0;
        rwsregrec.regrec.ExceptionHandler = &RwsExceptionHandler;

        // if we're here because the handler called longjmp(), exit
        ERRRTN( setjmp( rwsregrec.jmpbuf))

        rc = setjmp( rwsregrec.jmpbuf);
        if (rc) {
            fJump = TRUE;
            somPrintf( RWSSRVNAME ": Exception handled in ResolveConvert() preamble\n");
            break;
        }
        if (DosSetExceptionHandler( &rwsregrec.regrec))
            ERRNBR( RWSSRV_SETHANDLER)
    }

    // ptrs to items we'll be using repeatedly
    pRtn = CALCRTNPTR( pHdr);
    pArg1 = pRtn->pnext;

    // for each argument, convert or copy the input data, then handle the
    // result as though it were a returned value in need of conversion;
    // increment the counter if successful
    for (pArg = pArg1, ctr = 0; pArg; pArg = pArg->pnext)
    {
        // if handling is on, call setjmp on each iteration, primarily to
        // capture any register variables;  doing this has a side-effect:
        // because longjmp unwinds the exception chain back to the state
        // it was in when setjmp was last called, our handler will remain
        // in place even if an exception occurs

        if (fUseHandler)
        {
            rc = setjmp( rwsregrec.jmpbuf);
            if (rc)
            {
                somPrintf( RWSSRVNAME ": Exception handled in ResolveConvert() loop - pArg= %x\n", pArg);

                // if pArg were somehow reset to zero (it happened during
                // development), we'd end up in an endless exception loop,
                // so check for that & bail out if true, leaving rc as-is
                if (pArg == 0)
                    break;

                // signal that the current arg caused an exception,
                // clear rc, then move on to the next one
                pArg->rc = rc;
                rc = 0;
                continue;
            }
        }

        if (ResolveArgIn( pArg, pArg1) == 0 &&
            ResolveRtn( pArg, pRtn, hReply) == 0)
            ctr++;
    }

    // set the return code if it isn't already set;
    // indicate failure only if all failed
    if (rc == 0 && ctr == 0)
        rc = RWSSRV_CONVERTFAILED;

    // return the count of successful conversions
    pRtn->value = ctr;

} while (fFalse);

    // if our exception handler is set, unset it - this may produce
    // a spurious error if setjmp() was only called once
    if (fUseHandler && DosUnsetExceptionHandler( &rwsregrec.regrec) && !fJump)
        somPrintf( RWSSRVNAME ": ResolveConvert - DosUnsetExceptionHandler failed\n");

    return (rc);
}

/****************************************************************************/

// handles method & function calls:  converts input args, resolves the
// requested procedure to a PFN then executes it;  on return, converts
// the return value and any in/out args

ULONG           ResolveFunction( PRWSHDR pHdr, HWND hReply)

{
    ULONG       rc = 0;
    PRWSDSC     pProc;
    PRWSDSC     pRtn;
    PRWSDSC     pArg1;
    PRWSDSC     pArg;

do
{
    // ptrs to items we'll be using repeatedly
    pProc = CALCPROCPTR( pHdr);
    pRtn = pProc->pnext;
    pArg1 = pRtn->pnext;

    // process each argument, converting or copying data as needed;
    // note that the first arg is number 1, not zero
    for (pArg = pArg1; pArg; pArg = pArg->pnext)
        ERRRTN( ResolveArgIn( pArg, pArg1))

    if (rc)
        break;

    // identify the type of procedure & get its PFN as needed;
    // for method calls, arg1 must be a ptr to the object whose
    // method will be invoked
    ERRRTN( ResolveProc( pProc, pArg1))

    // execute the procedure
    ERRRTN( Execute( pProc, pRtn, pHdr->Cnt))

    // process the returned value
    // (yes, there is a reason this gets pRtn twice)
    ERRRTN( ResolveRtn( pRtn, pRtn, hReply))

    // process any in/out args;  ignore the error
    // if the function returned zero for an arg
    for (pArg = pArg1; pArg; pArg = pArg->pnext)
    {
        rc = ResolveArgOut( pArg, pRtn, hReply);
        if (rc && rc != RWSSRV_OUTVALUEZERO)
            break;
        rc = 0;
    }

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// this executes the requested function or method call using a kludgy
// system for handling va_lists;  it then determines if the procedure
// succeeded or failed based on flags set by the client

ULONG           Execute( PRWSDSC pProc, PRWSDSC pRtn, ULONG cntArg)

{
    ULONG       rc = 0;
    ULONG       ctr;
    PRWSDSC     pArg;
    ULONG       args[RWSMAXARGS]; // this is what limits functions to 16 args

    // better to pass known garbage than unknown garbage
    memset( args, 0, sizeof(args));

    // make the args easier to access
    for (ctr=0, pArg=pRtn; ctr < cntArg; ctr++)
    {
        pArg = pArg->pnext;
        args[ctr] = pArg->value;
    }

    // call the procedure (yes, this is ugly & clumsy)
    // note that the typedef for calls with no args is
    // different than the one for calls with 1-16 args
    if (cntArg == 0)
        pRtn->value = ((PFNRWSPROCV)(pProc->value))();
    else
    if (cntArg == 1)
        pRtn->value = ((PFNRWSPROC)(pProc->value))( args[0]);
    else
    if (cntArg == 2)
        pRtn->value = ((PFNRWSPROC)(pProc->value))(
                        args[0], args[1]);
    else
    if (cntArg <= 4)
        pRtn->value = ((PFNRWSPROC)(pProc->value))(
                        args[0], args[1], args[2], args[3]);
    else
    if (cntArg <= 8)
        pRtn->value = ((PFNRWSPROC)(pProc->value))(
                        args[0], args[1], args[2], args[3],
                        args[4], args[5], args[6], args[7]);
    else
    if (cntArg <= 12)
        pRtn->value = ((PFNRWSPROC)(pProc->value))(
                        args[0],  args[1],  args[2],  args[3],
                        args[4],  args[5],  args[6],  args[7],
                        args[8],  args[9],  args[10], args[11]);
    else
    if (cntArg <= 16)
        pRtn->value = ((PFNRWSPROC)(pProc->value))(
                        args[0],  args[1],  args[2],  args[3],
                        args[4],  args[5],  args[6],  args[7],
                        args[8],  args[9],  args[10], args[11],
                        args[12], args[13], args[14], args[15]);

    // if the procedure actually returns void, discard whatever
    // ended up in the return value;  otherwise, use the returned
    // value to determine if the procedure succeeded or failed
    if (pRtn->type == RWSR_VOID)
        pRtn->value = 0;
    else
        if ((GIVEQ( pProc->type) == PRTN_BOOL && pRtn->value == 0) ||
            (GIVEQ( pProc->type) == PRTN_ZERO && pRtn->value != 0))
            rc = RWSSRV_FUNCTIONFAILED;

    return (rc);
}

/****************************************************************************/

// resolves the requested procedure to a PFN as needed

ULONG           ResolveProc( PRWSDSC pProc, PRWSDSC pArg1)

{
    ULONG       rc = 0;
    PBYTE       ptr = CALCGIVEPTR( pProc);

do
{
    // determine the type of call, then get its PFN
    switch (GIVEP( pProc->type))
    {
      // method or function ptr provided by client
      case PROC_MPFN:
      case PROC_KPFN:
        break;

      // method lookup by name;  this requires us to know the
      // target object which should be in the first arg
      case PROC_MNAM:
        if (pArg1->value == 0)
            ERRNBR( RWSSRV_PROCNOWPOBJ)

        pProc->value = (ULONG)somResolveByName( (SOMObject*)pArg1->value, ptr);
        if (pProc->value == 0)
            rc = RWSSRV_PROCRESOLVEFAILED;
        break;

      // SOM function identified by its ordinal
      case PROC_KORD:
        if (DosQueryProcAddr( hmodSOM, *(PULONG)ptr, 0, (PFN*)&pProc->value))
            rc = RWSSRV_PROCQRYPROCFAILED;
        break;

      // anything else is an error
      default:
        rc = RWSSRV_PROCBADTYPE;
        break;
    }

} while (fFalse);

    pProc->rc = LOUSHORT(rc);

    return (rc);
}

/****************************************************************************/

// converts or copies data that will be given to the procedure;
// also confirms that in/out args are at least minimally valid

ULONG           ResolveArgIn( PRWSDSC pArg, PRWSDSC pArg1)

{
    ULONG       rc = 0;
    PBYTE       ptr = CALCGIVEPTR( pArg);

do
{
    // sanity check
    if (pArg->flags != DSC_ARG && pArg->flags != DSC_CONV)
        ERRNBR( RWSSRV_ARGBADDSC)

    switch (GIVEP( pArg->type))
    {
      // a DWORD that doesn't require any action
      case GIVE_ASIS:
        break;

      // pointers to buffers supplied by the client
      case GIVE_PSTR:
      case GIVE_PBUF:
      case GIVE_PPVOID:
        if (pArg->value == 0)
            rc = RWSSRV_ARGNOVALUE;
        break;

      // buffers supplied by the client that have to be copied
      // into memory belonging to the target object or to SOM;
      // the procedure will be given a ptr to the mem we allocate,
      case GIVE_COPYSTR:
      case GIVE_COPYBUF:
      {
        ULONG       err;

        if (pArg->cbgive == 0)
            ERRNBR( RWSSRV_ARGNOGIVESIZE)

        if ((GIVEQ( pArg->type) & (BYTE)COPY_MEMMASK) == COPY_OBJMEM)
        {
            if (pArg1->value == 0)
                ERRNBR( RWSSRV_ARGNOWPOBJ)
            pArg->value = (ULONG)_wpAllocMem( (SOMObject*)pArg1->value,
                                              pArg->cbgive, &err);
            if (pArg->value == 0 || err)
                ERRNBR( RWSSRV_WPALLOCFAILED)
        }
        else
        if ((GIVEQ( pArg->type) & (BYTE)COPY_MEMMASK) == COPY_SOMMEM)
        {
            pArg->value = (ULONG)SOMMalloc( pArg->cbgive);
            if (pArg->value == 0)
                ERRNBR( RWSSRV_SOMALLOCFAILED)
        }
        else
            ERRNBR( RWSSRV_ARGBADGIVEQ)

        memcpy( (char*)pArg->value, ptr, pArg->cbgive);
        break;
      } // GIVE_COPY*

      // data to be converted into an object ptr, class ptr, or somId
      case GIVE_CONV:
      {
        switch (GIVEQ( pArg->type))
        {
          case CONV_OBJHNDL:
            pArg->value = (ULONG)ObjFromHandle( *(PULONG)ptr);
            break;

          case CONV_OBJHWND:
            pArg->value = (ULONG)ObjFromWindow( *(PULONG)ptr);
            break;

          case CONV_OBJPREC:
            pArg->value = (ULONG)ObjFromCnrRec( *(PULONG)ptr);
            break;

          case CONV_OBJPATH:
            pArg->value = (ULONG)ObjFromPath( ptr);
            break;

          case CONV_OBJFQTITLE:
            pArg->value = (ULONG)ObjFromFQTitle( ptr, FALSE);
            break;

          case CONV_SHDFQTITLE:
            pArg->value = (ULONG)ObjFromShadow( ObjFromFQTitle( ptr, TRUE));
            break;

          case CONV_SHDHNDL:
            pArg->value = (ULONG)ObjFromShadow( ObjFromHandle( *(PULONG)ptr));
            break;

          case CONV_SHDPREC:
            pArg->value = (ULONG)ObjFromShadow( ObjFromCnrRec( *(PULONG)ptr));
            break;

          case CONV_CLSOBJ:
            pArg->value = (ULONG)ClassFromObj( *(SOMObject**)ptr);
            break;

          case CONV_CLSOBJHNDL:
            pArg->value = (ULONG)ClassFromObj( ObjFromHandle( *(PULONG)ptr));
            break;

          case CONV_CLSOBJHWND:
            pArg->value = (ULONG)ClassFromObj( ObjFromWindow( *(PULONG)ptr));
            break;

          case CONV_CLSOBJPREC:
            pArg->value = (ULONG)ClassFromObj( ObjFromCnrRec( *(PULONG)ptr));
            break;

          case CONV_CLSSHDHNDL:
            pArg->value = (ULONG)ClassFromObj( ObjFromShadow( ObjFromHandle( *(PULONG)ptr)));
            break;

          case CONV_CLSSHDPREC:
            pArg->value = (ULONG)ClassFromObj( ObjFromShadow( ObjFromCnrRec( *(PULONG)ptr)));
            break;

          case CONV_CLSNAME:
            pArg->value = (ULONG)ClassFromName( ptr);
            break;

          case CONV_CLSOBJPATH:
            pArg->value = (ULONG)ClassFromObj( ObjFromPath( ptr));
            break;

          case CONV_CLSOBJFQTITLE:
            pArg->value = (ULONG)ClassFromObj( ObjFromFQTitle( ptr, FALSE));
            break;

          case CONV_CLSSHDFQTITLE:
            pArg->value = (ULONG)ClassFromObj( ObjFromShadow( ObjFromFQTitle( ptr, TRUE)));
            break;

          case CONV_SOMID:
            pArg->value = (ULONG)somIdFromString( ptr);
            break;

          // input is ignored
          case CONV_SOMCLSMGR:
            pArg->value = (ULONG)SOMClassMgrObject;
            break;

          // *ptr is the address of the previous argument; copy its
          // value & rc;  if rc indicates an output conversion failure,
          // assume the input was OK and clear rc;  otherwise, propagate
          // the error to this arg
          case CONV_PREVIOUS:
            pArg->value = (*(PRWSDSC*)ptr)->value;
            rc = (*(PRWSDSC*)ptr)->rc;
            if (rc == RWSSRV_OUTGETCONVFAILED ||
                rc == RWSSRV_OUTBADGETCONV)
                rc = 0;
            break;

          default:
            pArg->value = 0;
            rc = RWSSRV_ARGBADGIVECONV;
            break;

        } // switch (GIVEQ)

        // a simple way to determine if the conversion failed
        if (pArg->value == 0 && rc == 0)
            rc = RWSSRV_ARGGIVECONVFAILED;
        break;

      } // case GIVE_CONV

      default:
        rc = RWSSRV_ARGBADGIVE;
        break;

    } // switch (GIVEP)

    if (rc)
        break;

    // a basic validity check:  these imply that we expect to get data back;
    // if so, the procedure needs a place to put the raw data & we need a
    // place to put the copied/converted data;  without these, crash-city
    switch (GETP( pArg->type))
    {
      case GET_PPSTR:
      case GET_PPBUF:
      case GET_CONV:
        if (pArg->value == 0 || pArg->pget == 0 || pArg->cbget == 0)
            rc = RWSSRV_ARGNULLPTR;
        break;
    }

} while (fFalse);

    pArg->rc = LOUSHORT(rc);

    return (rc);
}

/****************************************************************************/

// converts or copies data that was returned by the procedure in an
// in/out arg into a buffer supplied by the client;  in all cases
// except GET_ASIS, pArg->value contains a pointer to a void* supplied
// by the client;  its presence, and the presence of a get-buffer has
// already been validated by ResolveArgIn()

ULONG           ResolveArgOut( PRWSDSC pArg, PRWSDSC pRtn, HWND hReply)

{
    ULONG       rc = 0;

do
{
    // this should never be called if we're only doing data conversion
    if (pArg->flags != DSC_ARG)
        ERRNBR( RWSSRV_ARGBADDSC)

    switch (GETP( pArg->type))
    {
      // this is primarily for freeing a somId used as an input arg;
      // zero-out its value to keep it from being reused by the client
      case GET_ASIS:
        if (pArg->value && (GETQ( pArg->type) == (UCHAR)COPY_SOMMEM))
        {
            SOMFree( (somId)pArg->value);
            pArg->value = 0;
        }
        break;

      // copy a string into our buffer & free the mem if requested
      case GET_PPSTR:
        rc = OutGetString( pArg, pRtn);
        break;

      // copy bytes into our buffer & free the mem if requested
      case GET_PPBUF:
        rc = OutGetBuffer( pArg, pRtn);
        break;

      // convert the returned object ptr or somID into
      // something the client can use
      case GET_CONV:
        if (GIVEP( pArg->type) == GIVE_PPVOID)
            rc = OutGetConvert( *(PULONG)pArg->value, pArg, hReply);
        else
            rc = OutGetConvert( pArg->value, pArg, hReply);
        break;

      default:
        rc = RWSSRV_ARGBADGET;
        break;

    } // switch (GETP)

} while (fFalse);

    pArg->rc = LOUSHORT(rc);

    return (rc);
}

/****************************************************************************/

// converts or copies data that was either the value returned by a
// procedure or was put into the value field by ResolveArgIn() during
// input processing for data conversion;  for returned value processing,
// pArg == pRtn, for conversion processing the two are always different

ULONG           ResolveRtn( PRWSDSC pArg, PRWSDSC pRtn, HWND hReply)

{
    ULONG       rc = 0;

do
{
    // this function is used both for returned values & data conversion
    if (pArg->flags != DSC_RTN && pArg->flags != DSC_CONV)
        ERRNBR( RWSSRV_RTNBADDSC)

    switch (GETP( pArg->type))
    {
      // nothing to do
      case GET_ASIS:
      case GET_VOID:
        break;

      // copy a string into our buffer & free the mem if requested
      case GET_PSTR:
      case GET_PPSTR:
        rc = OutGetString( pArg, pRtn);
        break;

      // copy bytes into our buffer & free the mem if requested
      case GET_PBUF:
      case GET_PPBUF:
        rc = OutGetBuffer( pArg, pRtn);
        break;

      // value is an icon handle
      case GET_COPYICON:
      case GET_COPYMINI:
        if (OutGetIcon( pArg->value, (HPOINTER*)pArg->pget, hReply,
                        (GETP( pArg->type) == GET_COPYMINI)) == FALSE)
            rc = RWSSRV_RTNCOPYICONFAILED;
        break;

      // convert the returned object ptr or somID
      // into something the client can use
      case GET_CONV:
        rc = OutGetConvert( pArg->value, pArg, hReply);
        break;

      default:
        rc = RWSSRV_RTNBADGET;
        break;

    } // switch (GETP)

} while (fFalse);

    pArg->rc = LOUSHORT(rc);

    return (rc);
}

/****************************************************************************/
/* Copy Output functions                                                    */
/****************************************************************************/

// copy a string into our buffer after confirming it will fit;
// if requested, free the buffer returned by the procedure, then
// zero-out its value to keep it from being reused by the client

ULONG           OutGetString( PRWSDSC pArg, PRWSDSC pRtn)

{
    ULONG       rc = 0;
    ULONG       getp;
    ULONG       getq;
    PBYTE       ptr;

do
{
    // determine if pArg->value is a ptr or a ptr to a ptr
    getp = GETP( pArg->type);

    ptr = (PBYTE)pArg->value;
    if (ptr == 0)
        ERRNBR( RWSSRV_OUTVALUEZERO)

    if (getp == GET_PPSTR)
    {
        ptr = *(PBYTE*)ptr;
        if (ptr == 0)
            ERRNBR( RWSSRV_OUTVALUEZERO)
    }

    // if the string fits, copy it
    if (strlen( ptr) < pArg->cbget)
        strcpy( (char*)pArg->pget, ptr);
    else
        rc = RWSSRV_OUTBUFTOOSMALL;

    // see if we're supposed to free whatever we're pointing at
    getq = (GETQ( pArg->type) & (BYTE)COPY_MEMMASK);

    // if this is object memory, assume that arg1
    // identifies the object which owns it
    if (getq == COPY_OBJMEM)
        _wpFreeMem( (SOMObject*)pRtn->pnext->value, ptr);
    else
    if (getq == COPY_SOMMEM)
        SOMFree( ptr);
    else
    // if neither of the above is true, exit
        break;

    // now that we've freed the mem, the ptr is invalid so zero it out
    if (getp == GET_PPSTR)
        *(PBYTE*)pArg->value = 0;
    else
        pArg->value = 0;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// copy bytes into our buffer;  if the client specified where the number of
// bytes to copy could be found, use that count;  otherwise, copy as many as
// will fit in our buffer;  if requested, free the buffer returned by the
// procedure, then clear its value to keep it from being reused by the client

ULONG           OutGetBuffer( PRWSDSC pArg, PRWSDSC pRtn)

{
    ULONG       rc = 0;
    ULONG       cnt;
    ULONG       getp;
    ULONG       getq;
    PBYTE       ptr;

do
{
    // determine if pArg->value is a ptr or a ptr to a ptr
    getp = GETP( pArg->type);

    ptr = (PBYTE)pArg->value;
    if (ptr == 0)
        ERRNBR( RWSSRV_OUTVALUEZERO)

    if (getp == GET_PPBUF)
    {
        ptr = *(PBYTE*)ptr;
        if (ptr == 0)
            ERRNBR( RWSSRV_OUTVALUEZERO)
    }

    // determine how many bytes we should copy
    getq = (GETQ( pArg->type) & (BYTE)COPY_MASK);

    // if the count can be found elsewhere, get it;
    // otherwise copy however many bytes will fit in the buffer
    if (getq >= COPY_CNTFIRST && getq <= COPY_CNTLAST)
        cnt = OutGetCnt( ptr, pRtn, getq);
    else
        cnt = pArg->cbget;

    // if there's something to copy & it will fit, do so
    if (cnt == 0)
        rc = RWSSRV_OUTCNTZERO;
    else
    if (cnt > pArg->cbget)
        rc = RWSSRV_OUTBUFTOOSMALL;
    else
        memcpy( pArg->pget, ptr, cnt);

    // see if we're supposed to free whatever we're pointing at
    getq = (GETQ( pArg->type) & (BYTE)COPY_MEMMASK);

    // if this is object memory, assume that arg1
    // identifies the object which owns it
    if (getq == COPY_OBJMEM)
        _wpFreeMem( (SOMObject*)pRtn->pnext->value, ptr);
    else
    if (getq == COPY_SOMMEM)
        SOMFree( ptr);
    else
    // if neither of the above is true, exit
        break;

    // now that we've freed the mem, the ptr is invalid so zero it out
    if (getp == GET_PPSTR)
        *(PBYTE*)pArg->value = 0;
    else
        pArg->value = 0;

} while (fFalse);

    return (rc);
}

/****************************************************************************/

// used when a function copies data into a buffer & identifies the count
// at the head of the buffer, in the return value, or in another arg

ULONG           OutGetCnt( PBYTE pBuf, PRWSDSC pRtn, ULONG ndx)

{
    ULONG       ulRtn = 0;
    ULONG       ctr;
    PRWSDSC     pArg;

do
{
    // the copy count is in a ULONG at the head of the buffer
    if (ndx == COPY_CNTULONG)
    {
        ulRtn = *(PULONG)pBuf;
        break;
    }

    // the copy count is in a USHORT at the head of the buffer
    if (ndx == COPY_CNTUSHORT)
    {
        ulRtn = *(PUSHORT)pBuf;
        break;
    }

    // this is an array of ULONGS whose last element is zero;
    // get a count of the elements, then calc its size
    if (ndx == COPY_CNTULZERO)
    {
        for (ctr=0; ((PULONG)pBuf)[ctr]; ctr++)
            {;}
        ulRtn = (ctr+1)*sizeof( ULONG);
        break;
    }

    // the copy count was the function's return value
    if (ndx == COPY_CNTRTN)
    {
        if (pRtn->type == RWSR_ASIS)
            ulRtn = pRtn->value;
        break;
    }

    // the copy count was returmed via a PULONG in one of the
    // first 9 arguments (COPY_CNTARG1 through COPY_CNTARG9)
    for (pArg=pRtn, ctr=0, ndx -= COPY_CNTRTN; ctr < ndx && pArg; ctr++)
        pArg = pArg->pnext;

    if (pArg && pArg->type == RWSI_PPVOID)
        ulRtn = *(PULONG)pArg->value;

} while (fFalse);

    return (ulRtn);
}

/****************************************************************************/

// convert an object or class pointer or somId into something the
// client can use;  because pArg->value may contain the either the
// value or a pointer to it, the caller dereferences it (if needed)
// so we don't have to figure it out

ULONG           OutGetConvert( ULONG ulValue, PRWSDSC pArg, HWND hReply)

{
#define pObj    ((SOMObject*)ulValue)

    ULONG       rc = RWSSRV_OUTGETCONVFAILED;
    BOOL        fRtn = FALSE;

do
{
    if (ulValue == 0)
        ERRNBR( RWSSRV_OUTVALUEZERO)

    switch (GETQ( pArg->type))
    {
        case CONV_OBJHNDL:
            *(PULONG)pArg->pget = _wpQueryHandle( pObj);
            if (*(PULONG)pArg->pget)
                fRtn = TRUE;
            break;

        case CONV_OBJPREC:
            *(PULONG)pArg->pget = (ULONG)_wpQueryCoreRecord( pObj);
            if (*(PULONG)pArg->pget)
                fRtn = TRUE;
            break;

        case CONV_OBJHWND:
            *(PULONG)pArg->pget = ObjToHwnd( pObj);
            if (*(PULONG)pArg->pget)
                fRtn = TRUE;
            break;

        case CONV_OBJNAME:
            fRtn = ObjToName( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        case CONV_OBJPATH:
            fRtn = ObjToPath( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        case CONV_OBJID:
            fRtn = ObjToObjID( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        case CONV_OBJTITLE:
            fRtn = ObjToTitle( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        case CONV_OBJFLDR:
            fRtn = ObjToFolder( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        case CONV_OBJFQTITLE:
            fRtn = ObjToFQTitle( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        case CONV_OBJICON:
        case CONV_OBJMINI:
            fRtn = ObjToIcon( pObj, (HPOINTER*)pArg->pget, hReply,
                              (GETQ( pArg->type) == CONV_OBJMINI));
            break;

        // pObj can be an object ptr or a class ptr
        case CONV_CLSOBJ:
            *(PULONG)pArg->pget = (ULONG)ClassFromObj( pObj);
            if (*(PULONG)pArg->pget)
                fRtn = TRUE;
            break;

        // pObj can be an object ptr or a class ptr
        case CONV_CLSICON:
        case CONV_CLSMINI:
            fRtn = ObjToIcon( ClassFromObj( pObj), (HPOINTER*)pArg->pget,
                              hReply, (GETQ( pArg->type) == CONV_CLSMINI));
            break;

        // pObj can be an object ptr or a class ptr
        case CONV_CLSNAME:
            fRtn = ClassToName( pObj, (char*)pArg->pget, pArg->cbget);
            break;

        // value is a somId
        case CONV_SOMID:
            fRtn = SomIDToString( (somId)ulValue, (char*)pArg->pget,
                                  pArg->cbget);
            break;

        // value is a somId that should be freed after conversion
        case CONV_SOMID+FREE_SOMMEM:
            fRtn = SomIDToString( (somId)ulValue, (char*)pArg->pget,
                                  pArg->cbget);
            SOMFree( (somId)ulValue);
            break;

        default:
            rc = RWSSRV_OUTBADGETCONV;
            break;
    }

    if (fRtn)
        rc = 0;

} while (fFalse);

    return (rc);

#undef pObj
}

/****************************************************************************/

// takes an icon handle, duplicates the icon, then makes it available to the
// client process;  in earlier attempts to use WinCreatePointerIndirect(),
// I found that it would only use hbmPointer and would ignore hbmMiniPointer;
// consequently, the client must specify which one it wants

BOOL            OutGetIcon( HPOINTER hIcon, HPOINTER * pBuf,
                            HWND hClient, BOOL fMini)

{
    BOOL        fRtn = FALSE;
    PID         pid;
    TID         tid;
    HPOINTER    hIconClient;
    POINTERINFO ptrInfo;

do
{
    if (hIcon == 0 ||
        WinQueryWindowProcess( hClient, &pid, &tid) == FALSE ||
        WinQueryPointerInfo( hIcon, &ptrInfo) == FALSE)
        break;

    // if the client asked for a mini & there's one present, use it;
    // otherwise use the fullsized version;  regardless of what we give
    // it, WinCreatePointerIndirect() will only create full-sized icons
    if (fMini && ptrInfo.hbmMiniPointer && ptrInfo.hbmMiniColor)
    {
        ptrInfo.hbmPointer = ptrInfo.hbmMiniPointer;
        ptrInfo.hbmColor   = ptrInfo.hbmMiniColor;
    }
    else
        if (ptrInfo.hbmPointer == 0 || ptrInfo.hbmColor == 0)
            break;

    ptrInfo.hbmMiniPointer = 0;
    ptrInfo.hbmMiniColor = 0;

    hIconClient = WinCreatePointerIndirect( HWND_DESKTOP, &ptrInfo);
    if (hIconClient == 0)
        break;

    // if we can't transfer ownership to the client, destory the icon
    if (WinSetPointerOwner( hIconClient, pid, TRUE) == FALSE)
    {
        WinDestroyPointer( hIconClient);
        break;
    }

    *(HPOINTER*)pBuf = hIconClient;
    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/
/* Data Conversion functions - input                                        */
/****************************************************************************/

// return the object identified by the handle

SOMObject *     ObjFromHandle( HOBJECT hobj)

{
    SOMObject * pObj = NULL;
    SOMClass  * anyClass;

do
{
// no handle available, exit
    if (hobj == NULLHANDLE)
        break;

// wpclsObjectFromHandle is overridden by each of the base classes to support
// its own type of handle;  consequently, we have to identify the handle type
// and then invoke the method on the corresponding class object

    if (HIUSHORT( hobj) == 1)
        anyClass = clsWPTransient;
    else
    if (HIUSHORT( hobj) == 2)
        anyClass = clsWPAbstract;
    else
    if (HIUSHORT( hobj) == 3)
        anyClass = clsWPFileSystem;
    else
        break;

    pObj = _wpclsObjectFromHandle( anyClass, hobj);

} while (fFalse);

    return (pObj);
}

/****************************************************************************/

// "name" is either an object ID or the fully qualified
// path of a filesystem object

SOMObject *     ObjFromPath( PSZ pszName)

{
    SOMObject * pObj = 0;

    if (pszName)
        if (*pszName == '<')
            pObj = ObjFromHandle( WinQueryObject( pszName));
        else
        if (*pszName != 0)
            pObj = _wpclsQueryObjectFromPath( clsWPFileSystem, pszName);

    return (pObj);
}

/****************************************************************************/

// a pMiniRecordcore can be gotten via d&d

SOMObject *     ObjFromCnrRec( ULONG ulItemID)

{
    SOMObject * pObj = NULL;

    if (ulItemID &&
        (pObj = (SOMObject*)OBJECT_FROM_PREC( ulItemID )) != NULL)
        if (somIsObj( pObj) == FALSE)
            pObj = NULL;

    return (pObj);
}

/****************************************************************************/

// this gets the object the WPS opened - which may not be what you expect;
// e.g. if you open cmd.exe then have it start e.exe, you'll get cmd.exe's
// object when you supply e.exe's hwnd

SOMObject *     ObjFromWindow( HWND hSrc)

{
    SOMObject * pObj = NULL;
    HWND        hOwner;

    if (hSrc)
    {
        while ((hOwner = WinQueryWindow( hSrc, QW_OWNER)) != NULLHANDLE)
            hSrc = hOwner;
        pObj = _wpclsQueryObjectFromFrame( clsWPDesktop, hSrc);
    }

    return (pObj);
}

/****************************************************************************/

// if the object is a shadow, this will get the original object;
// otherwise, it will return the object passed to it

SOMObject *     ObjFromShadow( SOMObject * pShadow)

{
    SOMObject * pObj;

    if (pShadow && _somIsA( pShadow, clsWPShadow))
        pObj = _wpQueryShadowedObject( pShadow, FALSE);
    else
        pObj = pShadow;

    return (pObj);
}

/****************************************************************************/

#define OFT_MAX     8

SOMObject *     ObjFromFQTitle( PSZ pszName, BOOL fShadow)

{
    BOOL        flag;
    ULONG       ctr;
    ULONG       cnt;
    HFIND       hfind = 0;
    SOMObject * pObj = 0;
    SOMObject * pFolder = 0;
    SOMObject * apObj[OFT_MAX];
    char *      ptr;
    char *      pSave;
    char        szObj[CCHMAXPATH];

do
{
    // if there's nothing to search for, exit
    if (pszName == 0 || *pszName == 0)
        break;

    // if this is an object ID, get its object ptr then exit
    if (*pszName == '<')
    {
        pObj = ObjFromHandle( WinQueryObject( pszName));
        break;
    }

    // look for the backslash that separates path from title;
    // since titles can contain backslashes, the caller is
    // required to escape them by doubling them (  la C);
    // flag signals the presence of an escape sequence
    for (ctr=strlen( pszName)-1, pSave=0, flag=FALSE ; ctr; ctr--)
    {
        if (pszName[ctr] != '\\')
            continue;

        if (--ctr == 0)
            break;

        if (pszName[ctr] != '\\')
        {
            pSave = &pszName[ctr+1];
            break;
        }

        flag = TRUE;
    }

    // if we didn't find a backslash or if it was the last character, exit
    if (pSave == 0 || pSave[1] == 0)
        break;

    // null-terminate the path, get the object it refers to,
    // then restore the backslash
    *pSave = 0;
    pFolder = _wpclsQueryObjectFromPath( clsWPFileSystem, pszName);
    *pSave++ = '\\';

    // exit if we couldn't get the folder object
    if (pFolder == 0)
        break;

    // if the title contains doubled backslashes, we have
    // to make an unescaped copy & point pSave at it
    if (flag)
    {
        for (ptr=szObj; *pSave; pSave++, ptr++)
        {
            *ptr = *pSave;
            if (*pSave == '\\')
                pSave++;
        }
        *ptr = 0;
        pSave = szObj;
    }

    // get up to 8 matching objects;  if none were found, exit
    cnt = OFT_MAX;
    if (_wpclsFindObjectFirst( clsWPAbstract, 0, &hfind, pSave, pFolder,
                               FALSE, 0, apObj, &cnt) == FALSE &&
        _wpclsQueryError( clsWPAbstract) != WPERR_BUFFER_OVERFLOW)
        break;

    // if multiple objects were found, we have to decide which one to return;
    // an abstract object is always preferred over a transient, and a file-
    // system object is only returned as a last resort;  for abstract objects,
    // fShadow determines whether shadows or non-shadows are preferred
    if (cnt > 1)
        for (ctr=0, flag=FALSE; ctr < cnt; ctr++)
        {
            if (_somIsA( apObj[ctr], clsWPAbstract))
            {
                if (_somIsA( apObj[ctr], clsWPShadow) == fShadow)
                {
                    pObj = apObj[ctr];
                    break;
                }
                if (flag == FALSE)
                {
                    pObj = apObj[ctr];
                    flag = TRUE;
                }
            }
            else
                if (pObj == 0 && _somIsA( apObj[ctr], clsWPTransient))
                    pObj = apObj[ctr];
        }

    // if only one object was found or none of the criteria above were met,
    // return the first object (if multiple objects were found, the one we
    // return is a filesystem object of some sort)
    if (pObj == 0)
        pObj = apObj[0];

    // unlock the objects we don't need
    for (ctr=0; ctr < cnt; ctr++)
        if (apObj[ctr] != pObj)
            _wpUnlockObject( apObj[ctr]);


} while (fFalse);

    if (hfind)
        _wpclsFindObjectEnd( clsWPAbstract, hfind);

    if (pFolder)
        _wpUnlockObject( pFolder);

    return (pObj);
}

/****************************************************************************/

// return the object's class object (if it isn't already a class object)

SOMClass *      ClassFromObj( SOMObject * pObj)

{
    SOMClass *  pCls;

    if (pObj == 0 || _somIsA( pObj, clsSOMClass))
        pCls = pObj;
    else
        pCls = _somGetClass( pObj);

    return (pCls);
}

/****************************************************************************/

// returns a class object;  if the class isn't already loaded,
// this will load it

SOMClass *      ClassFromName( PSZ pszClass)

{
    somId       classid;
    SOMClass *  anyClass;

    if (pszClass == 0 || *pszClass == 0)
        return 0;

    classid = somIdFromString( pszClass);
    anyClass = _somFindClass( SOMClassMgrObject, classid, 0, 0);
    SOMFree( classid);
    return (anyClass);
}

/****************************************************************************/
/* Data Conversion functions - output                                       */
/****************************************************************************/

// this attempts to locate the handle of a non-Settings view;
// if Settings is all that's available, it will return that hwnd

HWND            ObjToHwnd( SOMObject * pObj)

{
    HWND        hRtn = 0;
    PVIEWITEM   pv = 0;

    while ((pv = _wpFindViewItem( pObj, VIEW_ANY, pv)) != NULL)
    {
        hRtn = pv->handle;
        if (pv->view != OPEN_SETTINGS)
            break;
    }

    return (hRtn);
}

/****************************************************************************/

// returns an unqualified name for filesystem objects
// or an unqualified title for other objects

BOOL            ObjToName( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;

    if (_somIsA( pObj, clsWPFileSystem))
        fRtn = _wpQueryRealName( pObj, pBuf, &cbBuf, FALSE);
    else
        fRtn = ObjToTitle( pObj, pBuf, cbBuf);

    return (fRtn);
}

/****************************************************************************/

// returns a fully-qualified name for filesystem objects
// or a fully-qualified title for other objects

BOOL            ObjToPath( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;

    if (_somIsA( pObj, clsWPFileSystem))
        fRtn = _wpQueryRealName( pObj, pBuf, &cbBuf, TRUE);
    else
        fRtn = ObjToFQTitle( pObj, pBuf, cbBuf);

    return (fRtn);
}

/****************************************************************************/

// returns an object ID

BOOL            ObjToObjID( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;
    PSZ         ptr;

    ptr = _wpQueryObjectID( pObj);

    if (ptr && strlen( ptr) < cbBuf)
    {
        strcpy( pBuf, ptr);
        fRtn = TRUE;
    }

    return (fRtn);
}

/****************************************************************************/

// returns an object's title;  for files, this may not match its real name

BOOL            ObjToTitle( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;
    PSZ         ptr;

    ptr = _wpQueryTitle( pObj);

    if (ptr && strlen( ptr) < cbBuf)
    {
        strcpy( pBuf, ptr);
        fRtn = TRUE;
    }

    return (fRtn);
}

/****************************************************************************/

// returns a fully-qualified name of the folder containing an object

BOOL            ObjToFolder( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;
    SOMObject * pFS;

    pFS = _wpQueryFolder( pObj);

    if (pFS)
        fRtn = _wpQueryRealName( pFS, pBuf, &cbBuf, TRUE);

    return (fRtn);
}

/****************************************************************************/

// returns a fully-qualified title for any type of object

BOOL            ObjToFQTitle( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;
    ULONG       cbPath;
    SOMObject * pFS;
    PSZ         pszTitle;
    PSZ         ptr;

do
{
    pszTitle = _wpQueryTitle( pObj);
    if (pszTitle == 0)
        break;

    // get the folder containing the object
    pFS = _wpQueryFolder( pObj);

    // some objects may not be in a folder
    if (pFS == 0)
    {
        if (strlen( pszTitle) >= cbBuf)
            break;

        strcpy( pBuf, pszTitle);
        fRtn = TRUE;
        break;
    }

    cbPath = 0;
    if (_wpQueryRealName( pFS, NULL, &cbPath, TRUE) == FALSE)
        break;

    if (cbPath + strlen( pszTitle) + 2 > cbBuf)
        break;

    // construct a path
    if (_wpQueryRealName( pFS, pBuf, &cbBuf, TRUE) == FALSE)
        break;
    ptr = strchr( pBuf, 0);
    if (ptr[-1] != '\\')
        *ptr++ = '\\';
    strcpy( ptr, pszTitle);

    fRtn = TRUE;

} while (fFalse);

    return (fRtn);
}

/****************************************************************************/

// returns the class name for both class and instance objects

BOOL            ClassToName( SOMObject * pObj, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;
    PSZ         ptr;

    if (_somIsA( pObj, clsSOMClass))
        ptr = _somGetName( pObj);
    else
        ptr = _somGetClassName( pObj);

    if (ptr && strlen( ptr) < cbBuf)
    {
        strcpy( pBuf, ptr);
        fRtn = TRUE;
    }

    return (fRtn);
}

/****************************************************************************/

// translates a somId to the string it represents (somIds are used
// extensively by the SOM runtime but not by the WPS)

BOOL            SomIDToString( somId id, char * pBuf, ULONG cbBuf)

{
    BOOL        fRtn = FALSE;
    PSZ         ptr = 0;

    if (id)
        ptr = somStringFromId( id);

    if (ptr && strlen( ptr) < cbBuf)
    {
        strcpy( pBuf, ptr);
        fRtn = TRUE;
    }

    return (fRtn);
}

/****************************************************************************/

// gets a class or object's icon

BOOL            ObjToIcon( SOMObject * pObj, HPOINTER * pBuf,
                           HWND hClient, BOOL fMini)

{
    HPOINTER    hIcon;

    if (_somIsA( pObj, clsSOMClass))
        hIcon = _wpclsQueryIcon( pObj);
    else
        hIcon = _wpQueryIcon( pObj);

    return (OutGetIcon( hIcon, pBuf, hClient, fMini));
}

/****************************************************************************/
/****************************************************************************/

