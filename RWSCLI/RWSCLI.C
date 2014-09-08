/****************************************************************************/
/* RWS - beta version 0.80                                                  */
/****************************************************************************/

// RWSCLI.C
// Remote Workplace Server - Client core functions

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
 * The Original Code is Remote Workplace Server - Client component.
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

#include "RWSCLI.H"

/****************************************************************************/

ULONG   _System RwsCall( PRWSHDR* ppHdr, ULONG callType, ULONG callValue,
                         ULONG rtnType,  ULONG rtnSize,  ULONG argCnt, ...);
ULONG   _System RwsCallIndirect( PRWSBLD pBld);

ULONG   _System RwsCallAsync( HWND hwnd, PRWSHDR* ppHdr,
                              ULONG callType, ULONG callValue,
                              ULONG rtnType,  ULONG rtnSize,
                              ULONG argCnt, ...);
ULONG   _System RwsCallIndirectAsync( HWND hwnd, PRWSBLD pBld);

ULONG   _System RwsBuild( PRWSHDR* ppHdr, ULONG callType, ULONG callValue,
                          ULONG rtnType,  ULONG rtnSize,  ULONG argCnt, ...);
ULONG   _System RwsBuildIndirect( PRWSBLD pBld);

ULONG           SetProc(  ULONG type,             ULONG value, PRWSDSC* ppBuf);
ULONG           SetRtn(   ULONG type, ULONG size,              PRWSDSC* ppBuf);
ULONG           SetArg(   ULONG type, ULONG size, ULONG value,
                          PRWSDSC* ppBuf, PRWSDSC pPrev);
ULONG           SizeProc( ULONG type,             ULONG value, PULONG   pcbArg);
ULONG           SizeRtn(  ULONG type, ULONG size,              PULONG   pcbArg);
ULONG           SizeArg(  ULONG type, ULONG size, ULONG value, PULONG   pcbArg);

/****************************************************************************/
/* Interfaces for the Build and Build/Dispatch functions                    */
/****************************************************************************/

// build & dispatch a server request from arguments pushed onto the stack

ULONG   _System RwsCall( PRWSHDR* ppHdr, ULONG callType, ULONG callValue,
                         ULONG rtnType,  ULONG rtnSize,  ULONG argCnt, ...)

{
    ULONG       rc = 0;

    rc = RwsBuildIndirect( (PRWSBLD)&ppHdr);
    if (rc == 0)
        rc = RwsDispatch( *ppHdr);

    return (rc);
}

/****************************************************************************/

// build & dispatch a server request from arguments in an RWSBLD struct

ULONG   _System RwsCallIndirect( PRWSBLD pBld)

{
    ULONG       rc = 0;

    rc = RwsBuildIndirect( pBld);
    if (rc == 0)
        rc = RwsDispatch( *pBld->ppHdr);

    return (rc);
}

/****************************************************************************/

// build a server request from arguments pushed onto the stack
// and dispatch it asynchronously

ULONG   _System RwsCallAsync( HWND hwnd, PRWSHDR* ppHdr,
                              ULONG callType, ULONG callValue,
                              ULONG rtnType,  ULONG rtnSize,
                              ULONG argCnt, ...)

{
    ULONG       rc = 0;

    rc = RwsBuildIndirect( (PRWSBLD)&ppHdr);
    if (rc == 0)
        rc = RwsDispatchAsync( hwnd, *ppHdr);

    return (rc);
}

/****************************************************************************/

// build a server request from arguments in an RWSBLD struct
// and dispatch it asynchronously

ULONG   _System RwsCallIndirectAsync( HWND hwnd, PRWSBLD pBld)

{
    ULONG       rc = 0;

    rc = RwsBuildIndirect( pBld);
    if (rc == 0)
        rc = RwsDispatchAsync( hwnd, *pBld->ppHdr);

    return (rc);
}

/****************************************************************************/

// build a server request from arguments pushed onto the stack

ULONG   _System RwsBuild( PRWSHDR* ppHdr, ULONG callType, ULONG callValue,
                          ULONG rtnType,  ULONG rtnSize,  ULONG argCnt, ...)

{
    return (RwsBuildIndirect( (PRWSBLD)&ppHdr));
}

/****************************************************************************/
/* Primary Build function                                                   */
/****************************************************************************/

// validates the user's input, calculates the request block's size,
// then constructs the request block that will be passed to the server;
// if successful, returns a ptr to the request block in *pBld->ppHdr

ULONG   _System RwsBuildIndirect( PRWSBLD pBld)

{
    ULONG       rc = 0;
    ULONG       ctr;
    ULONG       flags;
    ULONG       size = sizeof( RWSHDR);
    PRWSARG     pArg;
    PRWSHDR     pHdr = 0;
    PRWSDSC     pNext;
    PRWSDSC     pPrev;
    PRWSDSC     pFinal;

do
{
    // calculate the size of the request block;  the Size* functions
    // perform all input validation so the Set* functions won't have
    // to do any error checking
    ERRRTN( SizeProc( pBld->callType, pBld->callValue, &size))
    ERRRTN( SizeRtn( pBld->rtnType, pBld->rtnSize, &size))
    if (pBld->argCnt)
    {
        for (ctr=0, pArg=CALCARGPTR( pBld); ctr < pBld->argCnt; ctr++, pArg++)
            ERRRTN( SizeArg( pArg->type, pArg->size, pArg->value, &size))

        if (rc)
            break;
    }
    else
        // these types of calls all require at least one arg
        if (GIVEP( pBld->callType) == PROC_MNAM ||
            GIVEP( pBld->callType) == PROC_MPFN ||
            GIVEP( pBld->callType) == PROC_CONV)
            ERRNBR( RWSCLI_MISSINGARGS)

    // allocate the request block & init the header
    ERRRTN( RwsGetMem( &pHdr, size))
    pHdr->Cnt = LOUSHORT(pBld->argCnt);

    // the first RWSDSC struct immediately follows the header
    pNext = CALCPROCPTR( pHdr);

    // set the procedure descriptor
    ERRRTN( SetProc( pBld->callType, pBld->callValue, &pNext))

    pFinal = pNext;

    // set the return descriptor
    ERRRTN( SetRtn( pBld->rtnType, pBld->rtnSize, &pNext))

    // if there are any arguments, construct their descriptors
    if (pBld->argCnt)
    {
        if (GIVEP( pBld->callType) == (UCHAR)PROC_CONV)
            flags = DSC_CONV;
        else
            flags = DSC_ARG;

        pFinal = 0;
        for (ctr=0, pArg=CALCARGPTR( pBld); ctr < pBld->argCnt; ctr++, pArg++)
        {
            pPrev = pFinal;
            pFinal = pNext;
            pNext->flags = flags;
            ERRRTN( SetArg( pArg->type, pArg->size, pArg->value, &pNext, pPrev))
        }
    }
    if (rc)
        break;

    // the last arg in the chain must not point at anything
    pFinal->pnext = 0;

} while (fFalse);

    // if there was an error, free pHdr
    if (rc)
    {
        RwsFreeMem( pHdr);
        pHdr = 0;
    }
    *pBld->ppHdr = pHdr;

    return (rc);
}

/****************************************************************************/
/* Functions to Construct Server Request                                    */
/****************************************************************************/

// construct the procedure descriptor

ULONG       SetProc( ULONG type, ULONG value, PRWSDSC* ppBuf)

{
    ULONG       rc = 0;
    PRWSDSC     pdsc = *ppBuf;
    PBYTE       pnext = CALCGIVEPTR( pdsc);

do
{
    pdsc->flags = DSC_PROC;
    pdsc->type = type;

    switch (GIVEP( type))
    {
      // method identified by name
      // value will be resolved by server
      case PROC_MNAM:
        pdsc->cbgive = (USHORT)(strlen( (char*)value) + 1);
        strcpy( pnext, (char*)value);
        pnext += pdsc->cbgive;
        break;

      // SOM function identified by ordinal
      // value will be resolved by server
      case PROC_KORD:
        pdsc->cbgive = (USHORT)sizeof(ULONG);
        *((PULONG)pnext) = value;
        pnext += pdsc->cbgive;
        break;

      // method or function for which the user supplied the PFN
      case PROC_MPFN:
      case PROC_KPFN:
        pdsc->value = value;
        break;

      // data conversion
      case PROC_CONV:
        break;

      // RWS command
      // value will be interpreted by server
      case PROC_CMD:
        pdsc->value = value;
        break;

      // anything else is an error
      default:
        ERRNBR( RWSCLI_SETPROC)
    }

} while (fFalse);

    if (rc == 0)
    {
        // align the next descriptor on a dword, save it
        // in the descriptor's pnext & return it in *ppBuf
        pnext = (BYTE*)(((ULONG)pnext + 3) & (ULONG)~3);
        pdsc->pnext = (PRWSDSC)pnext;
        *ppBuf = (PRWSDSC)pnext;
    }

    return (rc);
}

/****************************************************************************/

// construct the descriptor for procedure's return value

ULONG       SetRtn( ULONG type, ULONG size, PRWSDSC* ppBuf)

{
    ULONG       rc = 0;
    PRWSDSC     pdsc = *ppBuf;
    PBYTE       pnext = CALCGIVEPTR( pdsc);

do
{
    pdsc->flags = DSC_RTN;
    pdsc->type = type;

    switch (GETP( type))
    {
      // returns a dword that requires no processing
      // or the function returns void
      case GET_ASIS:
      case GET_VOID:
        break;

      // returns ptr to string or ptr to ptr to string
      case GET_PSTR:
      case GET_PPSTR:
        if (size == 0)
            size = RWSSTRINGSIZE;
        pdsc->pget = pnext;
        pdsc->cbget = (USHORT)size;
        pnext += size;
        break;

      // returns ptr to a fixed-length buffer or ptr to ptr to buffer
      case GET_PBUF:
      case GET_PPBUF:
        pdsc->pget = pnext;
        pdsc->cbget = (USHORT)size;
        pnext += size;
        break;

      // returns an icon that must be duplicated
      // then given to the client process
      case GET_COPYICON:
      case GET_COPYMINI:
        pdsc->pget = pnext;
        pdsc->cbget = sizeof( ULONG);
        pnext += sizeof( ULONG);
        break;

      // returns an object or class ptr or a somId that has
      // to be converted into something the client can use
      case GET_CONV:
        switch (GETQ( type))
        {
          // conversion yields a dword
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_OBJICON:
          case CONV_OBJMINI:
          case CONV_CLSICON:
          case CONV_CLSMINI:
            pdsc->pget = pnext;
            pdsc->cbget = sizeof( ULONG);
            pnext += sizeof( ULONG);
            break;

          // conversion yields a string
          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_OBJNAME:
          case CONV_OBJTITLE:
          case CONV_OBJID:
          case CONV_OBJFLDR:
          case CONV_CLSNAME:
          case CONV_SOMID:
          case CONV_SOMID+FREE_SOMMEM:
            if (size == 0)
                size = RWSSTRINGSIZE;
            pdsc->pget = pnext;
            pdsc->cbget = (USHORT)size;
            pnext += size;
            break;

          // no other conversion is permitted
          default:
            ERRNBR( RWSCLI_SETRTN)
        }
        break;

      // no other return type is permitted
      default:
        ERRNBR( RWSCLI_SETRTN)
    }

} while (fFalse);

    if (rc == 0)
    {
        // align the next descriptor on a dword, save it
        // in the descriptor's pnext & return it in *ppBuf
        pnext = (BYTE*)(((ULONG)pnext + 3) & (ULONG)~3);
        pdsc->pnext = (PRWSDSC)pnext;
        *ppBuf = (PRWSDSC)pnext;
    }

    return (rc);
}

/****************************************************************************/

// construct the descriptor for procedure arguments

ULONG           SetArg( ULONG type, ULONG size, ULONG value,
                        PRWSDSC* ppBuf, PRWSDSC pPrev)

{
    ULONG       rc = 0;
    PRWSDSC     pdsc = *ppBuf;
    PBYTE       pnext = CALCGIVEPTR( pdsc);

do
{
    pdsc->type = type;

    // data that will be given to the procedure
    // (possibly after conversion)
    switch (GIVEP( type))
    {
      // a dword that requires no processing
      case GIVE_ASIS:
        pdsc->value = value;
        break;

      // ptr to a string
      // input string is required
      case GIVE_PSTR:
        pdsc->cbgive = (USHORT)(strlen( (char*)value) + 1);
        pdsc->value = (ULONG)pnext;
        strcpy( pnext, (char*)value);
        pnext += pdsc->cbgive;
        break;

      // ptr to a fixed-length buffer
      // input data is optional
      case GIVE_PBUF:
        pdsc->cbgive = (USHORT)size;
        pdsc->value = (ULONG)pnext;
        if (value)
            memcpy( pnext, (char*)value, pdsc->cbgive);
        pnext += pdsc->cbgive;
        break;

      // ptr to a dword
      // input data is optional
      case GIVE_PPVOID:
        pdsc->cbgive = (USHORT)sizeof(ULONG);
        pdsc->value = (ULONG)pnext;
        *((PULONG)pnext) = value;
        pnext += pdsc->cbgive;
        break;

      // ptr to a string that the server will copy into
      // memory allocated by either SOM or a WPS object
      // input string is required
      case GIVE_COPYSTR:
        pdsc->cbgive = (USHORT)(strlen( (char*)value) + 1);
        strcpy( pnext, (char*)value);
        pnext += pdsc->cbgive;
        break;

      // ptr to a buffer that the server will copy into
      // memory allocated by either SOM or a WPS object
      // input data is optional
      case GIVE_COPYBUF:
        pdsc->cbgive = (USHORT)size;
        if (value)
            memcpy( pnext, (char*)value, pdsc->cbgive);
        pnext += pdsc->cbgive;
        break;

      // data that is accessible to the client which must be converted
      // into an object or class ptr or a somID by the server
      // input data is required except for CONV_SOMCLSMGR
      case GIVE_CONV:
        switch (GIVEQ( type))
        {
          // input data is a dword
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_SHDHNDL:
          case CONV_SHDPREC:
          case CONV_CLSOBJ:
          case CONV_CLSOBJHNDL:
          case CONV_CLSOBJHWND:
          case CONV_CLSOBJPREC:
          case CONV_CLSSHDHNDL:
          case CONV_CLSSHDPREC:
            pdsc->cbgive = (USHORT)sizeof(ULONG);
            *((PULONG)pnext) = value;
            pnext += pdsc->cbgive;
            break;

          // input data is a string
          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_SHDFQTITLE:
          case CONV_CLSNAME:
          case CONV_CLSOBJPATH:
          case CONV_CLSOBJFQTITLE:
          case CONV_CLSSHDFQTITLE:
          case CONV_SOMID:
            pdsc->cbgive = (USHORT)(strlen( (char*)value) + 1);
            strcpy( pnext, (char*)value);
            pnext += pdsc->cbgive;
            break;

          // SOM Class Manager Object - input is ignored
          // a ULONG is allocated to facilitate reuse of the server block
          case CONV_SOMCLSMGR:
            pdsc->cbgive = (USHORT)sizeof(ULONG);
            pnext += pdsc->cbgive;
            break;

          // provide a ptr to the previous arg so its value can be copied
          // this is one of the few items that aren't validated by SizeArg()
          case CONV_PREVIOUS:
            if (pPrev == 0)
                ERRNBR( RWSCLI_ARGBADGIVECONV)
            pdsc->cbgive = (USHORT)sizeof(ULONG);
            *((PRWSDSC*)pnext) = pPrev;
            pnext += pdsc->cbgive;
            break;

          // no other input conversion is permitted
          default:
            ERRNBR( RWSCLI_SETARG)
        }
        break;

      // no other input type is permitted
      default:
        ERRNBR( RWSCLI_SETARG)
    }

    // if there was an error, exit
    if (rc)
        break;

    // align the get buffer on a dword
    pnext = (BYTE*)(((ULONG)pnext + 3) & (ULONG)~3);

    // data that will be returned by the procedure in
    // an in/out argument (typically, in a GIVE_PPVOID)
    switch (GETP( type))
    {
      // a dword that requires no processing
      case GET_ASIS:
        break;

      // ptr to ptr to string
      case GET_PPSTR:
      case GET_PSTR:
        if (size == 0)
            size = RWSSTRINGSIZE;
        pdsc->pget = pnext;
        pdsc->cbget = (USHORT)size;
        pnext += size;
        break;

      // ptr to ptr to fixed-length buffer
      case GET_PPBUF:
      case GET_PBUF:
        pdsc->pget = pnext;
        pdsc->cbget = (USHORT)size;
        pnext += size;
        break;

      // an object or class ptr or a somId that has to
      // be converted into something the client can use
      case GET_CONV:
        switch (GETQ( type))
        {
          // conversion yields a dword
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_OBJICON:
          case CONV_OBJMINI:
          case CONV_CLSOBJ:
          case CONV_CLSICON:
          case CONV_CLSMINI:
            pdsc->pget = pnext;
            pdsc->cbget = sizeof( ULONG);
            pnext += sizeof( ULONG);
            break;

          // conversion yields a string
          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_OBJNAME:
          case CONV_OBJTITLE:
          case CONV_OBJID:
          case CONV_OBJFLDR:
          case CONV_CLSNAME:
          case CONV_SOMID:
          case CONV_SOMID+FREE_SOMMEM:
            if (size == 0)
                size = RWSSTRINGSIZE;
            pdsc->pget = pnext;
            pdsc->cbget = (USHORT)size;
            pnext += size;
            break;

          // no other output conversion is permitted
          default:
            ERRNBR( RWSCLI_SETARG)
        }
        break;

      // no other output type is permitted
      default:
        ERRNBR( RWSCLI_SETARG)
    }

} while (fFalse);

    if (rc == 0)
    {
        // align the next descriptor on a dword, save it
        // in the descriptor's pnext & return it in *ppBuf
        pnext = (BYTE*)(((ULONG)pnext + 3) & (ULONG)~3);
        pdsc->pnext = (PRWSDSC)pnext;
        *ppBuf = (PRWSDSC)pnext;
    }

    return (rc);
}

/****************************************************************************/
/* Input Validation & Sizing functions                                      */
/****************************************************************************/

// validates procedure info & calculates size needed to hold it;
// in all cases:  no buffer can exceed 64k-1,  all types except
// PROC_CONV require a value

ULONG       SizeProc( ULONG type, ULONG value, PULONG pcbArg)

{
    ULONG       rc = 0;
    ULONG       cnt = sizeof( RWSDSC);
    ULONG       size = 0;

do
{
    switch (GIVEP( type))
    {
      // method identified by name
      // value will be resolved by server
      case PROC_MNAM:
        if (value == 0 || *(char*)value == 0)
            ERRNBR( RWSCLI_PROCNOVALUE)
        if (GIVEQ( type) > PRTN_LASTGIVEQ)
            ERRNBR( RWSCLI_PROCBADGIVEQ)
        size = strlen( (char*)value) + 1;
        if (size > 65535)
            ERRNBR( RWSCLI_PROCBADSIZE)
        cnt += size;
        break;

      // value will be resolved by server
      // SOM function identified by ordinal
      case PROC_KORD:
        if (value == 0)
            ERRNBR( RWSCLI_PROCNOVALUE)
        if (GIVEQ( type) > PRTN_LASTGIVEQ)
            ERRNBR( RWSCLI_PROCBADGIVEQ)
        cnt += sizeof( ULONG);
        break;

      // method or function for which the user supplied the PFN
      case PROC_MPFN:
      case PROC_KPFN:
        if (value == 0)
            ERRNBR( RWSCLI_PROCNOVALUE)
        if (GIVEQ( type) > PRTN_LASTGIVEQ)
            ERRNBR( RWSCLI_PROCBADGIVEQ)
        break;

      // data conversion
      case PROC_CONV:
        if (GIVEQ( type) != PRTN_NA)
            ERRNBR( RWSCLI_PROCBADGIVEQ)
        break;

      // RWS command
      // value will be interpreted by server
      case PROC_CMD:
        if (value < RWSCMD_FIRST || value > RWSCMD_LAST)
            ERRNBR( RWSCLI_PROCBADCMD)
        if (GIVEQ( type) != PRTN_NA)
            ERRNBR( RWSCLI_PROCBADGIVEQ)
        break;

      // anything else is an error
      default:
        ERRNBR( RWSCLI_PROCBADTYPE)
    }

} while (fFalse);

    if (rc == 0)
    {
        // the next descriptor will be aligned on a dword
        cnt = ((ULONG)(cnt + 3) & (ULONG)~3);
        *pcbArg += cnt;
    }

    return (rc);
}

/****************************************************************************/

// validates returned value info & calculates size needed to hold it;
// in all cases:  no buffer can exceed 64k-1 & only specific subtypes
// ("Q" values) are permitted for each major type ("P" value)

ULONG       SizeRtn( ULONG type, ULONG size, PULONG pcbArg)

{
    ULONG       rc = 0;
    ULONG       cnt = sizeof( RWSDSC);
    ULONG       getq = GETQ( type);

do
{
    // the only GIVE type permitted is VOID
    if (GIVEP( type) != GIVE_VOID)
        ERRNBR( RWSCLI_RTNBADGIVE)

    switch (GETP( type))
    {
      // returns a dword that requires no processing
      // or the function returns void
      case GET_ASIS:
      case GET_VOID:
        if (getq)
            rc = RWSCLI_RTNBADGETQ;
        break;

      // returns ptr to string or ptr to ptr to string
      case GET_PSTR:
      case GET_PPSTR:
        if (getq && getq <= COPY_CNTRTN)
            ERRNBR( RWSCLI_RTNBADGETQ)
        if (size > 65535)
            ERRNBR( RWSCLI_RTNBADSIZE)
        if (size == 0)
            size = RWSSTRINGSIZE;
        cnt += size;
        break;

      // returns ptr to a fixed-length buffer or ptr to ptr to buffer
      case GET_PBUF:
      case GET_PPBUF:
        if (getq && getq <= COPY_CNTRTN)
            ERRNBR( RWSCLI_RTNBADGETQ)
        if (size == 0 || size > 65535)
            ERRNBR( RWSCLI_RTNBADSIZE)
        cnt += size;
        break;

      // returns an icon that must be duplicated
      // then given to the client process
      case GET_COPYICON:
      case GET_COPYMINI:
        if (getq)
            ERRNBR( RWSCLI_RTNBADGETQ)
        cnt += sizeof( ULONG);
        break;

      // returns an object or class ptr or a somId that has
      // to be converted into something the client can use
      case GET_CONV:
        switch (getq)
        {
          // conversion yields a dword
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_OBJICON:
          case CONV_OBJMINI:
          case CONV_CLSICON:
          case CONV_CLSMINI:
            cnt += sizeof( ULONG);
            break;

          // conversion yields a string
          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_OBJNAME:
          case CONV_OBJTITLE:
          case CONV_OBJID:
          case CONV_OBJFLDR:
          case CONV_CLSNAME:
          case CONV_SOMID:
          case CONV_SOMID+FREE_SOMMEM:
            if (size > 65535)
                ERRNBR( RWSCLI_RTNBADSIZE)
            if (size == 0)
                size = RWSSTRINGSIZE;
            cnt += size;
            break;

          // no other conversion is permitted
          default:
            ERRNBR( RWSCLI_RTNBADGETCONV)
        }
        break;

      // no other return type is permitted
      default:
        ERRNBR( RWSCLI_RTNBADGET)
    }

} while (fFalse);

    if (rc == 0)
    {
        // the next descriptor will be aligned on a dword
        cnt = ((ULONG)(cnt + 3) & (ULONG)~3);
        *pcbArg += cnt;
    }

    return (rc);
}

/****************************************************************************/

// validates IN & IN/OUT arguments & calculates size needed to hold them;
// in all cases:  no buffer can exceed 64k-1 & only specific subtypes
// ("Q" values) are permitted for each major type ("P" value)

ULONG       SizeArg( ULONG type, ULONG size, ULONG value, PULONG pcbArg)

{
    ULONG       rc = 0;
    ULONG       q = GIVEQ( type);
    ULONG       cnt = sizeof( RWSDSC);

do
{
    // data that will be given to the procedure
    // (possibly after conversion)
    switch (GIVEP( type))
    {
      // a dword that requires no processing
      case GIVE_ASIS:
        if (q)
            rc = RWSCLI_ARGBADGIVEQ;
        break;

      // ptr to a string
      // input string is required
      case GIVE_PSTR:
        if (q)
            ERRNBR( RWSCLI_ARGBADGIVEQ)
        if (value == 0)
            ERRNBR( RWSCLI_ARGNOVALUE)
        size = strlen( (char*)value) + 1;
        if (size > 65535)
            ERRNBR( RWSCLI_ARGBADSIZE)
        cnt += size;
        break;

      // ptr to a string that the server will copy into
      // memory allocated by either SOM or a WPS object
      case GIVE_COPYSTR:
        if (q != COPY_OBJMEM && q != COPY_SOMMEM)
            ERRNBR( RWSCLI_ARGBADGIVEQ)
        if (value == 0)
            ERRNBR( RWSCLI_ARGNOVALUE)
        size = strlen( (char*)value) + 1;
        if (size > 65535)
            ERRNBR( RWSCLI_ARGBADSIZE)
        cnt += size;
        break;

      // ptr to a fixed-length buffer
      // input data is optional
      case GIVE_PBUF:
        if (q)
            ERRNBR( RWSCLI_ARGBADGIVEQ)
        if (size == 0 || size > 65535)
            ERRNBR( RWSCLI_ARGBADSIZE)
        cnt += size;
        break;

      // ptr to a buffer that the server will copy into
      // memory allocated by either SOM or a WPS object
      case GIVE_COPYBUF:
        if (q != COPY_OBJMEM && q != COPY_SOMMEM)
            ERRNBR( RWSCLI_ARGBADGIVEQ)
        if (size == 0 || size > 65535)
            ERRNBR( RWSCLI_ARGBADSIZE)
        cnt += size;
        break;

      // ptr to a dword
      // input data is optional
      case GIVE_PPVOID:
        if (q)
            ERRNBR( RWSCLI_ARGBADGIVEQ)
        cnt += sizeof(ULONG);
        break;

      // data that is accessible to the client which must be converted
      // into an object or class ptr or a somID by the server
      // input data is required except for CONV_SOMCLSMGR
      case GIVE_CONV:
        switch (q)
        {
          // input data is a dword
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_SHDHNDL:
          case CONV_SHDPREC:
          case CONV_CLSOBJ:
          case CONV_CLSOBJHNDL:
          case CONV_CLSOBJHWND:
          case CONV_CLSOBJPREC:
          case CONV_CLSSHDHNDL:
          case CONV_CLSSHDPREC:
            if (value == 0)
                ERRNBR( RWSCLI_ARGNOVALUE)
            cnt += sizeof(ULONG);
            break;

          // input data is a string
          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_SHDFQTITLE:
          case CONV_CLSNAME:
          case CONV_CLSOBJPATH:
          case CONV_CLSOBJFQTITLE:
          case CONV_CLSSHDFQTITLE:
          case CONV_SOMID:
            if (value == 0)
                ERRNBR( RWSCLI_ARGNOVALUE)
            size = strlen( (char*)value) + 1;
            if (size > 65535)
                ERRNBR( RWSCLI_ARGBADSIZE)
            cnt += size;
            break;

          // copy previous arg's value - input is ignored;  this will be
          // validated by SetArg() & filled with a ptr to the previous arg
          case CONV_PREVIOUS:
            cnt += sizeof(ULONG);
            break;

          // SOM Class Manager Object - input is ignored
          // a ULONG is allocated to facilitate reuse of the server block
          case CONV_SOMCLSMGR:
            cnt += sizeof(ULONG);
            break;

          // no other input conversion is permitted
          default:
            ERRNBR( RWSCLI_ARGBADGIVECONV)
        }
        break;

      // no other input type is permitted
      default:
        ERRNBR( RWSCLI_ARGBADGIVE)
    }

    // if there was an error, exit
    if (rc)
        break;

    // the get buffer will be aligned on a dword, so account for that
    cnt = ((ULONG)(cnt + 3) & (ULONG)~3);

    q = GETQ( type);

    // data that will be returned by the procedure in
    // an in/out argument (typically, in a GIVE_PPVOID)
    switch (GETP( type))
    {
      // a dword that requires no processing
      case GET_ASIS:
        if (q && q != FREE_SOMMEM)
            rc = RWSCLI_ARGBADGETQ;
        break;

      // ptr to ptr to string
      case GET_PPSTR:
        if (q)
            ERRNBR( RWSCLI_ARGBADGETQ)
        if (size > 65535)
            ERRNBR( RWSCLI_ARGBADGETSIZE)
        if (size == 0)
            size = RWSSTRINGSIZE;
        cnt += size;
        break;

      // ptr to string (used only by RWSC_ADDR_PSTR)
      case GET_PSTR:
        if (GIVEP( type) != GIVE_ASIS)
            ERRNBR( RWSCLI_ARGBADGET)
        if (value == 0)
            ERRNBR( RWSCLI_ARGNOVALUE)
        if (q)
            ERRNBR( RWSCLI_ARGBADGETQ)
        if (size > 65535)
            ERRNBR( RWSCLI_ARGBADGETSIZE)
        if (size == 0)
            size = RWSSTRINGSIZE;
        cnt += size;
        break;

      // ptr to ptr to fixed-length buffer
      case GET_PPBUF:
        if (q && q < COPY_CNTFIRST)
            ERRNBR( RWSCLI_ARGBADGETQ)
        if (size == 0 || size > 65535)
            ERRNBR( RWSCLI_ARGBADGETSIZE)
        cnt += size;
        break;

      // ptr to fixed-length buffer (used only by RWSC_ADDR_PBUF)
      case GET_PBUF:
        if (GIVEP( type) != GIVE_ASIS)
            ERRNBR( RWSCLI_ARGBADGET)
        if (value == 0)
            ERRNBR( RWSCLI_ARGNOVALUE)
        if (q && (q < COPY_CNTULONG || q > COPY_CNTULZERO))
            ERRNBR( RWSCLI_ARGBADGETQ)
        if (size == 0 || size > 65535)
            ERRNBR( RWSCLI_ARGBADGETSIZE)
        cnt += size;
        break;

      // an object or class ptr or a somId that has to
      // be converted into something the client can use
      case GET_CONV:
        switch (q)
        {
          // conversion yields a dword
          case CONV_OBJHNDL:
          case CONV_OBJHWND:
          case CONV_OBJPREC:
          case CONV_OBJICON:
          case CONV_OBJMINI:
          case CONV_CLSOBJ:
          case CONV_CLSICON:
          case CONV_CLSMINI:
            cnt += sizeof( ULONG);
            break;

          // conversion yields a string
          case CONV_OBJPATH:
          case CONV_OBJFQTITLE:
          case CONV_OBJNAME:
          case CONV_OBJTITLE:
          case CONV_OBJID:
          case CONV_OBJFLDR:
          case CONV_CLSNAME:
          case CONV_SOMID:
          case CONV_SOMID+FREE_SOMMEM:
            if (size > 65535)
                ERRNBR( RWSCLI_ARGBADGETSIZE)
            if (size == 0)
                size = RWSSTRINGSIZE;
            cnt += size;
            break;

          // no other output conversion is permitted
          default:
            ERRNBR( RWSCLI_ARGBADGETCONV)
        }
        break;

      // no other output type is permitted
      default:
        ERRNBR( RWSCLI_ARGBADGET)
    }

} while (fFalse);

    if (rc == 0)
    {
        // the next descriptor will be aligned on a dword
        cnt = ((ULONG)(cnt + 3) & (ULONG)~3);
        *pcbArg += cnt;
    }

    return (rc);
}

/****************************************************************************/
/****************************************************************************/

