
/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/* ==============================================================================
*             Texas Instruments OMAP (TM) Platform Software
*  (c) Copyright Texas Instruments, Incorporated.  All Rights Reserved.
*
*  Use of this software is controlled by the terms and conditions found
*  in the license agreement under which this software has been supplied.
* ============================================================================ */
/**
* @file OMX_JpegEncoder.c
*
* This file implements OMX Component for JPEG encoder that
* is fully compliant with the OMX specification 1.5.
*
* @path  $(CSLPATH)\src
*
* @rev  0.1
*/
/* -------------------------------------------------------------------------------- */
/* ================================================================================
*!
*! Revision History
*! ===================================
*!
*! 22-May-2006 mf: Revisions appear in reverse chronological order;
*! that is, newest first.  The date format is dd-Mon-yyyy.
* ================================================================================= */

/****************************************************************
*  INCLUDE FILES
****************************************************************/

/* ----- System and Platform Files ----------------------------*/
#ifdef UNDER_CE
#include <windows.h>
#include <oaf_osal.h>
#include <omx_core.h>
#else
#include <wchar.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <errno.h>
#include <pthread.h>
#endif

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbapi.h>


/*------- Program Header Files ----------------------------------------*/

#include "OMX_JpegEnc_Utils.h"
#include "OMX_JpegEnc_CustomCmd.h"

#ifdef RESOURCE_MANAGER_ENABLED
    #include <ResourceManagerProxyAPI.h>
#endif
/*----------------------Global-----------------------------------*/
OMX_STRING cJPEGencName = "OMX.TI.JPEG.encoder"; 
/*--------function prototypes ---------------------------------*/
static OMX_ERRORTYPE JPEGENC_SetCallbacks (OMX_HANDLETYPE hComp,
                                   OMX_CALLBACKTYPE* pCallBacks, 
                                   OMX_PTR pAppData);

static OMX_ERRORTYPE JPEGENC_GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING pComponentName, 
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion, 
                                          OMX_UUIDTYPE* pComponentUUID);

static OMX_ERRORTYPE JPEGENC_SendCommand (OMX_HANDLETYPE hComponent,
                                  OMX_COMMANDTYPE Cmd,
                                  OMX_U32 nParam,
                                  OMX_PTR pCmdData);

static OMX_ERRORTYPE JPEGENC_GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_INDEXTYPE nParamIndex,
                                   OMX_INOUT OMX_PTR ComponentParameterStructure);

static OMX_ERRORTYPE JPEGENC_SetParameter (OMX_HANDLETYPE hComp,
                                   OMX_INDEXTYPE nParamIndex,
                                   OMX_PTR ComponentParameterStructure);

static OMX_ERRORTYPE JPEGENC_GetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex, 
                                OMX_PTR ComponentConfigStructure);

static OMX_ERRORTYPE JPEGENC_SetConfig (OMX_HANDLETYPE hComp,
                                OMX_INDEXTYPE nConfigIndex, 
                                OMX_PTR ComponentConfigStructure);

static OMX_ERRORTYPE JPEGENC_EmptyThisBuffer (OMX_HANDLETYPE hComp, 
                                      OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE JPEGENC_FillThisBuffer (OMX_HANDLETYPE hComp, 
                                     OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE JPEGENC_GetState (OMX_HANDLETYPE hComp, 
                               OMX_STATETYPE* pState);

static OMX_ERRORTYPE JPEGENC_ComponentTunnelRequest(OMX_IN  OMX_HANDLETYPE hComp, 
                                            OMX_IN  OMX_U32 nPort,
                                            OMX_IN  OMX_HANDLETYPE hTunneledComp,
                                            OMX_IN  OMX_U32 nTunneledPort,
                                            OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);


static OMX_ERRORTYPE JPEGENC_VerifyTunnelConnection(JPEG_PORT_TYPE*pPort, 
                                            OMX_HANDLETYPE hTunneledComp,
                                            OMX_PARAM_PORTDEFINITIONTYPE* pPortDef);

static OMX_ERRORTYPE JPEGENC_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                               OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
                               OMX_IN OMX_U32 nPortIndex,
                               OMX_IN OMX_PTR pAppPrivate,
                               OMX_IN OMX_U32 nSizeBytes,
                               OMX_IN OMX_U8* pBuffer);

static OMX_ERRORTYPE JPEGENC_FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_IN  OMX_U32 nPortIndex,
                                OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE JPEGENC_ComponentDeInit(OMX_HANDLETYPE pHandle);

static OMX_ERRORTYPE JPEGENC_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                             OMX_INOUT OMX_BUFFERHEADERTYPE** pBuffer,
                             OMX_IN OMX_U32 nPortIndex,
                             OMX_IN OMX_PTR pAppPrivate,
                             OMX_IN OMX_U32 nSizeBytes);

static void JPEGENC_InitBufferFlagTrack(JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, OMX_U32 nPortIndex);

#ifdef KHRONOS_1_1
static OMX_ERRORTYPE ComponentRoleEnum(
                OMX_IN OMX_HANDLETYPE hComponent,
                OMX_OUT OMX_U8 *cRole,
                OMX_IN OMX_U32 nIndex);
#endif

static OMX_ERRORTYPE JPEGENC_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent, 
                                                OMX_IN OMX_STRING cParameterName, 
                                                OMX_OUT OMX_INDEXTYPE* pIndexType);


/*-------- Function Implementations ---------------------------------*/


static OMX_ERRORTYPE JPEGENC_FreeBuffer(OMX_IN  OMX_HANDLETYPE hComponent,
                                        OMX_IN  OMX_U32 nPortIndex,
                                        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    OMX_COMPONENTTYPE *pHandle                   = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef       = NULL;
    JPEGENC_BUFFER_PRIVATE* pBuffPrivate         = NULL;
    JPEG_PORT_TYPE* pCompPort                    = NULL;
    OMX_ERRORTYPE eError                         = OMX_ErrorNone;
    char* pTemp                                  = NULL;
    
    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pBuffer);
    
    JPEGENC_DPRINT("JPEGENC_FreeBuffer %p at port %d\n", pBuffer, nPortIndex);

    pHandle = (OMX_COMPONENTTYPE *) hComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *) pHandle->pComponentPrivate;
    pCompPort = pComponentPrivate->pCompPort[nPortIndex];

#ifdef __PERF_INSTRUMENTATION__
    /* never frees buffer */
    PERF_SendingFrame(pComponentPrivate->pPERF,
                      pBuffer->pBuffer,
                      pBuffer->nFilledLen,
                      PERF_ModuleMax);
#endif

    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    
    if ( nPortIndex == pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat->nPortIndex ) {
        pBuffPrivate = pBuffer->pInputPortPrivate;
    } 
    else if ( nPortIndex == pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat->nPortIndex  ) {
        pBuffPrivate = pBuffer->pOutputPortPrivate;
    } 
    else {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
    }

    if (pBuffPrivate->bAllocByComponent == OMX_TRUE) {
        if (pBuffer->pBuffer) {
           pTemp = (char*)pBuffer->pBuffer;
           pTemp -= 128;
           free(pTemp);
           pBuffer->pBuffer = NULL;
        }
    }
                       
    if (pBuffer) {
        free(pBuffer);
        pBuffer = NULL;
    }

    if ( pPortDef->bEnabled && 
        ((pComponentPrivate->nCurState == OMX_StateIdle && pComponentPrivate->nToState != OMX_StateLoaded) || 
         pComponentPrivate->nCurState == OMX_StateExecuting || 
         pComponentPrivate->nCurState == OMX_StatePause) ) {
        pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle, 
                                      pComponentPrivate->pHandle->pApplicationPrivate,
                                      OMX_EventError, 
                                      OMX_ErrorPortUnpopulated, 
                                      OMX_TI_ErrorMinor,
                                      "Port Unpopulated");
        }

    pCompPort->nBuffCount--;
        JPEGENC_DPRINT("JPEGE: bPopulated %d\n", pComponentPrivate->pCompPort[nPortIndex]->pPortDef->bPopulated);
    if (pCompPort->nBuffCount == 0) {
        pComponentPrivate->pCompPort[nPortIndex]->pPortDef->bPopulated = OMX_FALSE;

        pthread_mutex_lock(&pComponentPrivate->jpege_mutex_app);
        pthread_cond_signal(&pComponentPrivate->unpopulate_cond);
        pthread_mutex_unlock(&pComponentPrivate->jpege_mutex_app);

        JPEGENC_InitBufferFlagTrack(pComponentPrivate, nPortIndex);
    }

    if ((!pPortDef->bEnabled) &&
        (!pComponentPrivate->pCompPort[nPortIndex]->pPortDef->bPopulated)) {
        if ((nPortIndex == 0) && (!pComponentPrivate->bInportDisableIncomplete)) {
            pComponentPrivate->bInportDisableIncomplete = OMX_TRUE;
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                JPEGENC_INP_PORT,
                                                NULL);              
        }   
        if ((nPortIndex == 1) && (!pComponentPrivate->bOutportDisableIncomplete)) {
            pComponentPrivate->bOutportDisableIncomplete = OMX_TRUE;
            pComponentPrivate->cbInfo.EventHandler (pComponentPrivate->pHandle,
                                                pComponentPrivate->pHandle->pApplicationPrivate,
                                                OMX_EventCmdComplete,
                                                OMX_CommandPortDisable,
                                                JPEGENC_OUT_PORT,
                                                NULL);              
        }
    }

EXIT:
    JPEGENC_DPRINT("Exit from FreeBuffer\n");

    return eError;
}



OMX_ERRORTYPE JPEGENC_UseBuffer(OMX_IN OMX_HANDLETYPE hComponent,
            OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_IN OMX_U32 nPortIndex,
            OMX_IN OMX_PTR pAppPrivate,
            OMX_IN OMX_U32 nSizeBytes,
            OMX_IN OMX_U8* pBuffer)

{
    OMX_COMPONENTTYPE *pHandle                      = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate    = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef          = NULL;
    OMX_ERRORTYPE eError                            = OMX_ErrorNone;
    OMX_HANDLETYPE hTunnelComponent                 = NULL;
    OMX_U32 nBufferCount                            = -1;
    OMX_BUFFERHEADERTYPE *pBuffHeader               = NULL;
    int nInpIndex;
    int nOutIndex;
    

    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(ppBufferHdr);
    OMX_CHECK_PARAM(pBuffer);
    
    if (nPortIndex < 0 || nPortIndex > 1) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    hTunnelComponent = pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->hTunnelComponent;
    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    nBufferCount = pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;

#ifdef __PERF_INSTRUMENTATION__
    PERF_ReceivedFrame(pComponentPrivate->pPERF,
                       pBuffer,
                       nSizeBytes,
                       PERF_ModuleLLMM);
#endif

    pPortDef = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    

    nInpIndex = pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat->nPortIndex;
    nOutIndex = pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat->nPortIndex;

    if ( nPortIndex == nInpIndex ) {
        OMX_MALLOC_STRUCT(pBuffHeader, OMX_BUFFERHEADERTYPE);
        OMX_CONF_INIT_STRUCT(pBuffHeader, OMX_BUFFERHEADERTYPE);
        pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr = pBuffHeader;
        *ppBufferHdr = pBuffHeader;
    } 
    else if ( nPortIndex == nOutIndex ) {
        OMX_MALLOC_STRUCT(pBuffHeader, OMX_BUFFERHEADERTYPE);
        OMX_CONF_INIT_STRUCT(pBuffHeader, OMX_BUFFERHEADERTYPE);
        pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr = pBuffHeader;
        *ppBufferHdr = pBuffHeader;     
    } 
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    /* set relevant fields */
    (*ppBufferHdr)->nSize       = sizeof(OMX_BUFFERHEADERTYPE);
    (*ppBufferHdr)->nVersion    = pPortDef->nVersion;
    (*ppBufferHdr)->pBuffer     = pBuffer;
    (*ppBufferHdr)->nAllocLen   = nSizeBytes;
    (*ppBufferHdr)->pAppPrivate = pAppPrivate;
    pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->bAllocByComponent = OMX_FALSE;

    /* set direction dependent fields */
    if (hTunnelComponent != NULL) {

        if ( pPortDef->eDir == OMX_DirInput ) {
            (*ppBufferHdr)->nInputPortIndex   = nPortIndex;
            (*ppBufferHdr)->nOutputPortIndex  = pComponentPrivate->pCompPort[nPortIndex]->nTunnelPort;
            /* pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr = (*ppBufferHdr); */

        } 
        else {
            (*ppBufferHdr)->nOutputPortIndex   = nPortIndex;
            (*ppBufferHdr)->nInputPortIndex    = pComponentPrivate->pCompPort[nPortIndex]->nTunnelPort;
            /* pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->pBufferHdr = (*ppBufferHdr); */
        }
    }
    else {
        if (nPortIndex == JPEGENC_INP_PORT) {
            pBuffHeader->nInputPortIndex  = JPEGENC_INP_PORT;
            pBuffHeader->nOutputPortIndex = OMX_NOPORT;
        }
        else {
            pBuffHeader->nInputPortIndex  = OMX_NOPORT;
            pBuffHeader->nOutputPortIndex = JPEGENC_OUT_PORT;
        }
    }
    if (nPortIndex == JPEGENC_INP_PORT) {
        pBuffHeader->pInputPortPrivate = pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }
    else {
        pBuffHeader->pOutputPortPrivate = pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }

    if (pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->hTunnelComponent != NULL) {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->eBufferOwner = JPEGENC_BUFFER_TUNNEL_COMPONENT;
    }
    else {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->eBufferOwner = JPEGENC_BUFFER_CLIENT;
    }

    if ( nPortIndex == pComponentPrivate->pCompPort[nPortIndex]->pPortFormat->nPortIndex ) {
        pComponentPrivate->pCompPort[nPortIndex]->nBuffCount++;
        if (pComponentPrivate->pCompPort[nPortIndex]->nBuffCount == pPortDef->nBufferCountActual) {
            pPortDef->bPopulated = OMX_TRUE;

            pthread_mutex_lock(&pComponentPrivate->jpege_mutex_app);
            pthread_cond_signal(&pComponentPrivate->populate_cond);
            pthread_mutex_unlock(&pComponentPrivate->jpege_mutex_app);

            JPEGENC_InitBufferFlagTrack(pComponentPrivate, nPortIndex);
        }
    }

EXIT:
    return eError;  

}
/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentInit() Set the all the function pointers of component
  *
  * This method will update the component function pointer to the handle
  *
  * @param hComp         handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_ErrorInsufficientResources If the malloc fails
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComponent)
{
    OMX_COMPONENTTYPE *pHandle  = NULL;
    OMX_ERRORTYPE eError            = OMX_ErrorNone;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_U8 i = 0;
    /* Allocate memory for component's private data area */
    
    OMX_CHECK_PARAM(hComponent);
    
    JPEGENC_DPRINT("in jpeg-enc OMX_ComponentInit\n");

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    OMX_MALLOC_STRUCT (pHandle->pComponentPrivate, JPEGENC_COMPONENT_PRIVATE);
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;

#ifdef __PERF_INSTRUMENTATION__
        pComponentPrivate->pPERF = PERF_Create(PERF_FOURCC('J','P','E',' '),
                                               PERF_ModuleLLMM | PERF_ModuleImageEncode);
        pComponentPrivate->pPERFcomp = NULL;
#endif

    /**Assigning address of Component Structure point to place holder inside comp
    nentprivate structure
    */
    ((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pHandle = pHandle;

    pComponentPrivate->nCurState = OMX_StateLoaded;
   
    pHandle->SetCallbacks               = JPEGENC_SetCallbacks;
    pHandle->GetComponentVersion        = JPEGENC_GetComponentVersion;
    pHandle->SendCommand                = JPEGENC_SendCommand;
    pHandle->GetParameter               = JPEGENC_GetParameter;
    pHandle->SetParameter               = JPEGENC_SetParameter;
    pHandle->GetConfig                  = JPEGENC_GetConfig;
    pHandle->SetConfig                  = JPEGENC_SetConfig;
    pHandle->GetState                   = JPEGENC_GetState;
    pHandle->EmptyThisBuffer            = JPEGENC_EmptyThisBuffer;
    pHandle->FillThisBuffer             = JPEGENC_FillThisBuffer;
    pHandle->ComponentTunnelRequest     = JPEGENC_ComponentTunnelRequest;
    pHandle->ComponentDeInit            = JPEGENC_ComponentDeInit;
    pHandle->UseBuffer                  = JPEGENC_UseBuffer    ;
    pHandle->FreeBuffer                 = JPEGENC_FreeBuffer;
    pHandle->AllocateBuffer             = JPEGENC_AllocateBuffer;
    pHandle->GetExtensionIndex = JPEGENC_GetExtensionIndex;
#ifdef KHRONOS_1_1
    pHandle->ComponentRoleEnum          = ComponentRoleEnum;
#endif
    JPEGENC_DPRINT("Inside Component Init JPEG encoder\n");
    pComponentPrivate->ComponentVersion.s.nVersionMajor = 0x01;
    pComponentPrivate->ComponentVersion.s.nVersionMinor = 0x00;
    pComponentPrivate->ComponentVersion.s.nRevision = 0x00;
    pComponentPrivate->ComponentVersion.s.nStep = 0x00;
    OMX_MALLOC_STRUCT_EXTRA(pComponentPrivate->cComponentName, char, COMP_MAX_NAMESIZE);    
    strncpy(pComponentPrivate->cComponentName, cJPEGencName, COMP_MAX_NAMESIZE);
    
    /* Allocate memory for component data structures */
    OMX_MALLOC_STRUCT(pComponentPrivate->pPortParamType, OMX_PORT_PARAM_TYPE);  
    OMX_MALLOC_STRUCT(pComponentPrivate->pPortParamTypeAudio, OMX_PORT_PARAM_TYPE); 
    OMX_MALLOC_STRUCT(pComponentPrivate->pPortParamTypeVideo, OMX_PORT_PARAM_TYPE); 
    OMX_MALLOC_STRUCT(pComponentPrivate->pPortParamTypeOthers, OMX_PORT_PARAM_TYPE);    
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT], JPEG_PORT_TYPE); 
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT], JPEG_PORT_TYPE); 
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat, OMX_IMAGE_PARAM_PORTFORMATTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat, OMX_IMAGE_PARAM_PORTFORMATTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pParamBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pParamBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pQualityfactor, OMX_IMAGE_PARAM_QFACTORTYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCrop, OMX_CONFIG_RECTTYPE);    
    OMX_MALLOC_STRUCT(pComponentPrivate->pCustomLumaQuantTable, OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE);
    OMX_MALLOC_STRUCT(pComponentPrivate->pCustomChromaQuantTable, OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE);    
    OMX_MALLOC_STRUCT(pComponentPrivate->pHuffmanTable, JPEGENC_CUSTOM_HUFFMANTTABLETYPE);    
    for (i = 0;i<NUM_OF_BUFFERSJPEG;i++) {
        OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufferPrivate[i], JPEGENC_BUFFER_PRIVATE);
    }

    for (i = 0;i<NUM_OF_BUFFERSJPEG;i++) {
        pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufferPrivate[i]->pBufferHdr = NULL;
    }

    for (i = 0;i<NUM_OF_BUFFERSJPEG;i++) {
        OMX_MALLOC_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pBufferPrivate[i], JPEGENC_BUFFER_PRIVATE);
    }

    for (i = 0;i<NUM_OF_BUFFERSJPEG;i++) {
        pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pBufferPrivate[i]->pBufferHdr = NULL;
    }

    pComponentPrivate->nFlags               = 0;
    pComponentPrivate->isLCMLActive         = 0;
    pComponentPrivate->nCommentFlag         = 0;
    /* pComponentPrivate->nThumbnailFlag   = 0; */
    pComponentPrivate->pMarkData            = NULL;
    pComponentPrivate->hMarkTargetComponent = NULL;
    pComponentPrivate->pString_Comment      = NULL;
    pComponentPrivate->nDRI_Interval        = 0;
    pComponentPrivate->bSetHuffmanTable = OMX_FALSE;
    pComponentPrivate->bSetLumaQuantizationTable = OMX_FALSE;
    pComponentPrivate->bSetChromaQuantizationTable = OMX_FALSE;    
    
    pComponentPrivate->ThumbnailInfo.APP0_THUMB_INDEX = 0;
    pComponentPrivate->ThumbnailInfo.APP1_THUMB_INDEX = 0;
    pComponentPrivate->ThumbnailInfo.APP13_THUMB_INDEX = 0;

    pComponentPrivate->ThumbnailInfo.APP0_BUF.size = 0;
    pComponentPrivate->ThumbnailInfo.APP1_BUF.size = 0;
    pComponentPrivate->ThumbnailInfo.APP13_BUF.size = 0;

    pComponentPrivate->InParams.pInParams = NULL;
    pComponentPrivate->InParams.size = 0;   
    pComponentPrivate->bPreempted = OMX_FALSE;

    OMX_MALLOC_STRUCT_EXTRA(pComponentPrivate->pDynParams, IDMJPGE_TIGEM_DynamicParams, 256);
    if (!pComponentPrivate->pDynParams) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    eError = SetJpegEncInParams(pComponentPrivate);

#ifdef KHRONOS_1_1
    strcpy((char *)pComponentPrivate->componentRole.cRole, "image_encoder.jpeg");
#endif

    /* Set pPortParamType defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortParamType, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamType->nPorts = NUM_OF_PORTS;
    pComponentPrivate->pPortParamType->nStartPortNumber = 0;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortParamTypeAudio, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeAudio->nPorts = 0;
    pComponentPrivate->pPortParamTypeAudio->nStartPortNumber = -1;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortParamTypeVideo, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeVideo->nPorts = 0;
    pComponentPrivate->pPortParamTypeVideo->nStartPortNumber = -1;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPortParamTypeOthers, OMX_PORT_PARAM_TYPE);
    pComponentPrivate->pPortParamTypeOthers->nPorts = 0;
    pComponentPrivate->pPortParamTypeOthers->nStartPortNumber = -1;

    /*Set Quality factor structure */
/*    OMX_CONF_INIT_STRUCT(pComponentPrivate->pQualityfactor, OMX_IMAGE_PARAM_QFACTORTYPE);*/

    /* Set pInPortDef defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nPortIndex            = JPEGENC_INP_PORT;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->eDir                  = OMX_DirInput;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nBufferCountActual    = NUM_OF_BUFFERSJPEG;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nBufferCountMin       = 1;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nBufferSize           = 460800; /* 50688; */
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->bEnabled              = OMX_TRUE;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->bPopulated            = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->eDomain               = OMX_PortDomainImage;

    pComponentPrivate->isLCMLActive = 0;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.cMIMEType             = "JPEGENC";
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.pNativeRender         = NULL;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.nFrameWidth           = 640;  /* 176; */
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.nFrameHeight          = 480; /* 144; */
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.nStride               = -1;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.nSliceHeight          = -1;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.bFlagErrorConcealment = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.eCompressionFormat    =  OMX_IMAGE_CodingUnused; /* OMX_IMAGE_CodingJPEG; */
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->format.image.eColorFormat          =  OMX_COLOR_FormatYUV420PackedPlanar; /* OMX_COLOR_FormatCbYCrY; */
    

    /* Set pOutPortDef defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->nPortIndex         = JPEGENC_OUT_PORT;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->eDir               = OMX_DirOutput;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->nBufferCountMin    = 1;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->nBufferCountActual = NUM_OF_BUFFERSJPEG;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->nBufferSize        = 460800; /* 50688; */
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->bEnabled           = OMX_TRUE;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->bPopulated         = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->eDomain            = OMX_PortDomainImage;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.cMIMEType        = "JPEGENC";
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.pNativeRender    = NULL;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.nFrameWidth      = 640; /* 176; */
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.nFrameHeight     = 480; /* 144; */
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.nStride          = -1;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.nSliceHeight     = -1;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.bFlagErrorConcealment    = OMX_FALSE;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat       = OMX_IMAGE_CodingJPEG; /* OMX_IMAGE_CodingUnused; */
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eColorFormat             =  OMX_COLOR_FormatUnused; /* OMX_COLOR_FormatCbYCrY; */
    

   OMX_CONF_INIT_STRUCT(pComponentPrivate->pCustomLumaQuantTable, OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE);
    pComponentPrivate->pCustomLumaQuantTable->nPortIndex = JPEGENC_OUT_PORT;
    pComponentPrivate->pCustomLumaQuantTable->eQuantizationTable = OMX_IMAGE_QuantizationTableLuma;

   OMX_CONF_INIT_STRUCT(pComponentPrivate->pCustomChromaQuantTable, OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE);
    pComponentPrivate->pCustomChromaQuantTable->nPortIndex = JPEGENC_OUT_PORT;
    pComponentPrivate->pCustomChromaQuantTable->eQuantizationTable = OMX_IMAGE_QuantizationTableChroma;

   OMX_CONF_INIT_STRUCT(pComponentPrivate->pHuffmanTable, JPEGENC_CUSTOM_HUFFMANTTABLETYPE);
    pComponentPrivate->pHuffmanTable->nPortIndex = JPEGENC_OUT_PORT;
    
    /* Set pInPortFormat defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat, OMX_IMAGE_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat->nPortIndex         = JPEGENC_INP_PORT;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat->nIndex             = 0x0;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat->eCompressionFormat = OMX_IMAGE_CodingUnused; /* OMX_IMAGE_CodingJPEG; */
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat->eColorFormat       = OMX_COLOR_FormatYUV420PackedPlanar; /* OMX_COLOR_FormatCbYCrY; */

    /* Set pOutPortFormat defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat, OMX_IMAGE_PARAM_PORTFORMATTYPE);
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat->nPortIndex         = JPEGENC_OUT_PORT;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat->nIndex             = 0x0;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat->eCompressionFormat = OMX_IMAGE_CodingJPEG; /* OMX_IMAGE_CodingUnused; */
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat->eColorFormat       = OMX_COLOR_FormatUnused; /* OMX_COLOR_FormatCbYCrY; */

    /* Set pPriorityMgmt defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
    pComponentPrivate->pPriorityMgmt->nGroupPriority = -1;
    pComponentPrivate->pPriorityMgmt->nGroupID       = -1;

    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCrop, OMX_CONFIG_RECTTYPE);
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pQualityfactor, OMX_IMAGE_PARAM_QFACTORTYPE);
    pComponentPrivate->pQualityfactor->nPortIndex = 1;
    pComponentPrivate->pQualityfactor->nQFactor   = 100;
    
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pParamBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pParamBufSupplier->nPortIndex      = JPEGENC_INP_PORT;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pParamBufSupplier->eBufferSupplier = 0x0;

    /* Set pOutBufSupplier defaults */
    OMX_CONF_INIT_STRUCT(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pParamBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE);
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pParamBufSupplier->nPortIndex      = JPEGENC_OUT_PORT;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pParamBufSupplier->eBufferSupplier = 0x0;
    
    
    
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufSupplier= OMX_BufferSupplyOutput;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->hTunnelComponent = NULL;
    pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->nBuffCount       = 0;
    
        
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->hTunnelComponent = NULL;
    pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->nBuffCount       = 0;

    pthread_mutex_init(&pComponentPrivate->jpege_mutex, NULL);
    pthread_cond_init(&pComponentPrivate->stop_cond, NULL);
    pthread_cond_init(&pComponentPrivate->flush_cond, NULL);
    /* pthread_cond_init(&pComponentPrivate->control_cond, NULL); */

    pthread_mutex_init(&pComponentPrivate->jpege_mutex_app, NULL);
    pthread_cond_init(&pComponentPrivate->populate_cond, NULL);
    pthread_cond_init(&pComponentPrivate->unpopulate_cond, NULL);

#ifdef RESOURCE_MANAGER_ENABLED
    /* load the ResourceManagerProxy thread */
    eError = RMProxy_NewInitalizeEx(OMX_COMPONENTTYPE_IMAGE);
    if ( eError != OMX_ErrorNone ) {
        JPEGENC_DPRINT ("Error returned from loading ResourceManagerProxy thread\n");
        goto EXIT;
    }

#endif
    eError = JPEGEnc_Start_ComponentThread(pHandle);
    if (eError) {
        JPEGENC_DPRINT("Error while initializing thread\n");
        goto EXIT;
    }



JPEGENC_DPRINT("JPEG-ENC: actual buffer %d\n", pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nBufferCountActual);
    EXIT:
    if(eError != OMX_ErrorNone){
        FREE(pComponentPrivate->cComponentName);
        FREE(pComponentPrivate->pPortParamType);
        FREE(pComponentPrivate->pPortParamTypeAudio);
        FREE(pComponentPrivate->pPortParamTypeVideo);   
        FREE(pComponentPrivate->pPortParamTypeOthers);  
        FREE(pComponentPrivate->pCustomLumaQuantTable);
        FREE(pComponentPrivate->pCustomChromaQuantTable);        
        FREE(pComponentPrivate->pHuffmanTable);                
        FREE(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef);
        FREE(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef);
        FREE(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortFormat);
        FREE(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortFormat);
        FREE(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pParamBufSupplier);
        FREE(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pParamBufSupplier);
        FREE(pComponentPrivate->pPriorityMgmt);
        FREE(pComponentPrivate->pQualityfactor);
        FREE(pComponentPrivate->pDynParams);
        for (i = 0;i<NUM_OF_BUFFERSJPEG;i++) {
            FREE(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufferPrivate[i]);
            FREE(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pBufferPrivate[i]);
        }
#ifdef __PERF_INSTRUMENTATION__
        FREE(pComponentPrivate->pPERF);
#endif
        FREE(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]);
        FREE(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]);
        FREE(pComponentPrivate);
    }
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_SetCallbacks() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComponent    handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param pAppData      pointer to application Data
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_SetCallbacks (OMX_HANDLETYPE pComponent,
                                   OMX_CALLBACKTYPE* pCallBacks, 
                                   OMX_PTR pAppData)
{
    OMX_ERRORTYPE eError        = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle  = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;

    OMX_CHECK_PARAM(pComponent);
    OMX_CHECK_PARAM(pCallBacks);
    
    pHandle = (OMX_COMPONENTTYPE*)pComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    
    /*Copy the callbacks of the application to the component private  */
    memcpy (&(pComponentPrivate->cbInfo), pCallBacks, sizeof(OMX_CALLBACKTYPE));
    OMX_MEMCPY_CHECK(&(pComponentPrivate->cbInfo));

    /*Copy the application private data to component memory  */
    pHandle->pApplicationPrivate = pAppData;
    pComponentPrivate->nCurState = OMX_StateLoaded;

EXIT:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_GetComponentVersion() Sets application callbacks to the component
  *
  * This method will get the component Version.
  *
  * @param hComp         handle for this instance of the component
  * @param pComponentName    application callbacks
  * @param pComponentVersion
  * @param pSpecVersion
  * @param pComponentUUID
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_GetComponentVersion (OMX_HANDLETYPE hComp,
                                          OMX_STRING szComponentName, 
                                          OMX_VERSIONTYPE* pComponentVersion,
                                          OMX_VERSIONTYPE* pSpecVersion, 
                                          OMX_UUIDTYPE* pComponentUUID)
{
    OMX_ERRORTYPE          eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE    * pHandle           = NULL;
    JPEGENC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
    OMX_U8 *pTemp = NULL;
    JPEGENC_DPRINT ("Inside JPEGENC_GetComponentVersion function\n");
    if (!hComp || !szComponentName || !pComponentVersion || !pSpecVersion || !pComponentUUID) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pHandle = (OMX_COMPONENTTYPE*)hComp;
    if (!pHandle->pComponentPrivate) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;

    strncpy(szComponentName, pComponentPrivate->cComponentName, COMP_MAX_NAMESIZE);
    pTemp = memcpy(pComponentVersion, &(pComponentPrivate->ComponentVersion.s), sizeof(pComponentPrivate->ComponentVersion.s));
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
    pTemp = memcpy(pSpecVersion, &(pComponentPrivate->SpecVersion.s), sizeof(pComponentPrivate->SpecVersion.s));
    if(pTemp == NULL){
        eError = OMX_ErrorUndefined;
        goto EXIT;
    }
EXIT:
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_SendCommand() Send commands to the
  *
  * This method will update application callbacks
  * the application.
  *
  * @param phandle       handle for this instance of the component
  * @param Cmd           OMX Command usually OMX_CommandStateSet
  * @param nParam
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_SendCommand (
             OMX_HANDLETYPE hComponent,
             OMX_COMMANDTYPE Cmd,
             OMX_U32 nParam,
             OMX_PTR pCmdData
             )
{
    OMX_ERRORTYPE                eError         = OMX_ErrorNone;
    OMX_COMPONENTTYPE            *pHandle       = NULL;
    JPEGENC_COMPONENT_PRIVATE    *pCompPrivate  = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDefIn    = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDefOut   = NULL;
    OMX_MARKTYPE *pMarkType = NULL; 
    OMX_COMMANDTYPE eCmd         = -1;
    int nRet                = 0;

    OMX_CHECK_PARAM(hComponent);
    
    pHandle      = (OMX_COMPONENTTYPE *)hComponent;
    pCompPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDefIn   = pCompPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef;
    pPortDefOut  = pCompPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef;
   
    JPEGENC_DPRINT("Print nParam = %d\n", (int)nParam);

    if ( pCompPrivate->nCurState == OMX_StateInvalid ) {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch ( Cmd ) {
    case OMX_CommandStateSet:
        eCmd = SetState;
        pCompPrivate->nToState = nParam;
        break;
    case OMX_CommandFlush:
        if ((nParam != 0) && (nParam != 1) && (nParam != -1)) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        eCmd = Flush;
        break;
    case OMX_CommandPortDisable:
        if (nParam == JPEGENC_INP_PORT  || nParam == -1) {
            pPortDefIn->bEnabled = OMX_FALSE;
            pCompPrivate->bInportDisableIncomplete = OMX_FALSE;
        }
        if (nParam == JPEGENC_OUT_PORT  || nParam == -1) {
            pPortDefOut->bEnabled = OMX_FALSE;
            pCompPrivate->bOutportDisableIncomplete = OMX_FALSE;
        }
        if ((nParam != JPEGENC_INP_PORT) && (nParam != JPEGENC_OUT_PORT) && (nParam != -1))
        {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }   

        if (!pPortDefIn->bPopulated) {
            pCompPrivate->bInportDisableIncomplete = OMX_TRUE;
            pCompPrivate->cbInfo.EventHandler (pCompPrivate->pHandle,
            pCompPrivate->pHandle->pApplicationPrivate,
            OMX_EventCmdComplete,
            OMX_CommandPortDisable,
            JPEGENC_INP_PORT,
            NULL);              
        }

        if (!pPortDefOut->bPopulated) {
            pCompPrivate->bOutportDisableIncomplete = OMX_TRUE;
            pCompPrivate->cbInfo.EventHandler (pCompPrivate->pHandle,
                            pCompPrivate->pHandle->pApplicationPrivate,
                            OMX_EventCmdComplete,
                            OMX_CommandPortDisable,
                            JPEGENC_OUT_PORT,
                            NULL);          
        }
        eCmd = OMX_CommandPortDisable; 
        break; 

    case OMX_CommandPortEnable:
        if (nParam == JPEGENC_INP_PORT  || nParam == -1) {
            pPortDefIn->bEnabled = OMX_TRUE;
        }
        if (nParam == JPEGENC_OUT_PORT  || nParam == -1) {
            pPortDefOut->bEnabled = OMX_TRUE;
        }
        if ((nParam != JPEGENC_INP_PORT) && (nParam != JPEGENC_OUT_PORT) && (nParam != -1)) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        eCmd = OMX_CommandPortEnable;
        break;

    case OMX_CommandMarkBuffer:
        if ((nParam != JPEGENC_OUT_PORT) && (nParam != JPEGENC_INP_PORT)) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        pMarkType = (OMX_MARKTYPE *)pCmdData;
        pCompPrivate->pMarkData = pMarkType->pMarkData;
        pCompPrivate->hMarkTargetComponent = pMarkType->hMarkTargetComponent;
        pCompPrivate->nMarkPort = nParam;
        JPEGENC_DPRINT("IN SendCommand, port %d mark %x\n", pCompPrivate->nMarkPort, pCompPrivate->pMarkData);
        goto EXIT;
        break;
    default:
        break;

    }

    nRet = write(pCompPrivate->nCmdPipe[1], &eCmd, sizeof(eCmd));
    if (nRet == -1) {
        JPEGENC_DPRINT("%d :: Error while writing to the pipe.\n", __LINE__);
        eError = OMX_ErrorUndefined;
    }

#ifdef __PERF_INSTRUMENTATION__
        PERF_SendingCommand(pCompPrivate->pPERF,
                            Cmd,
                            (Cmd == OMX_CommandMarkBuffer) ? (OMX_U32) pCmdData : nParam,
                            PERF_ModuleComponent);
#endif
        
   
    nRet = write(pCompPrivate->nCmdDataPipe[1], &nParam, sizeof(nParam));
    if (nRet == -1) {
        JPEGENC_DPRINT("%d :: Error while writing to the pipe.\n", __LINE__);
        eError = OMX_ErrorUndefined;
    }
    JPEGENC_DPRINT("Writing into Cmd Pipe\n");
    
    EXIT:
    
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_GetParameter() Gets Parameters from the Component
  *
  * This method will update parameters from the component to the app.
  *
  * @param hComp         handle for this instance of the component
  * @param nParamIndex   Component Index Port
  * @param ComponentParameterStructure Component Parameter Structure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
                                   OMX_IN OMX_INDEXTYPE nParamIndex,
                                   OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    OMX_COMPONENTTYPE         *pComp             = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_ERRORTYPE             eError             = OMX_ErrorNone;
    JPEG_PORT_TYPE            *pInpPortType      = NULL;
    JPEG_PORT_TYPE            *pOutPortType      = NULL;

    JPEGENC_DPRINT("Entering function\n");
    
    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pComponentParameterStructure);
    
    pComp = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE*)pComp->pComponentPrivate;

    if ( pComponentPrivate->nCurState == OMX_StateInvalid ) {
        eError = OMX_ErrorInvalidState;
        goto EXIT;
    }
    pInpPortType = pComponentPrivate->pCompPort[JPEGENC_INP_PORT];
    pOutPortType = pComponentPrivate->pCompPort[JPEGENC_OUT_PORT];

    switch ( nParamIndex ) {
    case OMX_IndexParamImageInit:
        memcpy(pComponentParameterStructure, pComponentPrivate->pPortParamType, sizeof(OMX_PORT_PARAM_TYPE));
        OMX_MEMCPY_CHECK(pComponentParameterStructure);
        break;
    case OMX_IndexParamAudioInit:
        memcpy(pComponentParameterStructure, pComponentPrivate->pPortParamTypeAudio, sizeof(OMX_PORT_PARAM_TYPE));
        OMX_MEMCPY_CHECK(pComponentParameterStructure);
        break;
    case OMX_IndexParamVideoInit:
        memcpy(pComponentParameterStructure, pComponentPrivate->pPortParamTypeVideo, sizeof(OMX_PORT_PARAM_TYPE));
        OMX_MEMCPY_CHECK(pComponentParameterStructure);
        break;
    case OMX_IndexParamOtherInit:
        memcpy(pComponentParameterStructure, pComponentPrivate->pPortParamTypeOthers, sizeof(OMX_PORT_PARAM_TYPE));
        OMX_MEMCPY_CHECK(pComponentParameterStructure);
        break;
    case OMX_IndexParamPortDefinition:
    {
       OMX_PARAM_PORTDEFINITIONTYPE *pParamPortDef  = NULL;

       pParamPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
       JPEGENC_DPRINT("index is %d (%d) (%d)\n", 
            pParamPortDef->nPortIndex,
            pInpPortType->pPortDef->nPortIndex,
            pOutPortType->pPortDef->nPortIndex);
        if (pParamPortDef->nPortIndex == pInpPortType->pPortDef->nPortIndex)
        {
            JPEGENC_DPRINT("nBufferCountActual %d (0) \n", pInpPortType->pPortDef->nBufferCountActual);
            /* pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->bPopulated = OMX_FALSE; */
            memcpy(pComponentParameterStructure, pInpPortType->pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            OMX_MEMCPY_CHECK(pComponentParameterStructure);
        } else if (pParamPortDef->nPortIndex == pOutPortType->pPortDef->nPortIndex)
        {
            JPEGENC_DPRINT("nBufferCountActual %d (1)\n", pOutPortType->pPortDef->nBufferCountActual);
            /* pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->bPopulated = OMX_FALSE; */
            memcpy(pComponentParameterStructure, pOutPortType->pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            OMX_MEMCPY_CHECK(pComponentParameterStructure);
        } else {
            eError = OMX_ErrorBadPortIndex;
        }
    }
        break;

    case OMX_IndexParamImagePortFormat:
        {
        OMX_IMAGE_PARAM_PORTFORMATTYPE *pParamImagePortFormat = NULL;

        pParamImagePortFormat = (OMX_IMAGE_PARAM_PORTFORMATTYPE *)pComponentParameterStructure;
        if ( pParamImagePortFormat->nPortIndex == pInpPortType->pPortFormat->nPortIndex )
        {
            if (pParamImagePortFormat->nIndex > pInpPortType->pPortFormat->nIndex ) {
                eError = OMX_ErrorNoMore;
            } else {
                memcpy(pComponentParameterStructure, pInpPortType->pPortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentParameterStructure);
            }
        } else if (pParamImagePortFormat->nPortIndex == pOutPortType->pPortFormat->nPortIndex )
        {
            if ( pParamImagePortFormat->nIndex > pOutPortType->pPortFormat->nIndex ) {
                eError = OMX_ErrorNoMore;
            } else {
                memcpy(pComponentParameterStructure, pOutPortType->pPortFormat, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
                OMX_MEMCPY_CHECK(pComponentParameterStructure);
            }
        } else {
            eError = OMX_ErrorBadPortIndex;
        }
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        memcpy(pComponentParameterStructure, pComponentPrivate->pPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
        OMX_MEMCPY_CHECK(pComponentParameterStructure);
        break;

    case OMX_IndexParamCompBufferSupplier:
        {
        OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pComponentParameterStructure;

        if ( pBuffSupplierParam->nPortIndex == pOutPortType->pPortDef->nPortIndex)
        {
            pBuffSupplierParam->eBufferSupplier = pOutPortType->pParamBufSupplier->eBufferSupplier;
        } else if ( pBuffSupplierParam->nPortIndex == pInpPortType->pPortDef->nPortIndex )
        {
            pBuffSupplierParam->eBufferSupplier = pInpPortType->pParamBufSupplier->eBufferSupplier;
        } else
        {
            eError = OMX_ErrorBadPortIndex; 
            break;
        }
        }
        break;
    case OMX_IndexParamQuantizationTable:
        {
        OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *pQuantTable = (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *)pComponentParameterStructure;
        if (pQuantTable->eQuantizationTable == OMX_IMAGE_QuantizationTableLuma) {
            memcpy(pQuantTable, pComponentPrivate->pCustomLumaQuantTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
        } 
        else if (pQuantTable->eQuantizationTable == OMX_IMAGE_QuantizationTableChroma) {
            memcpy(pQuantTable, pComponentPrivate->pCustomChromaQuantTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
        }
        else { /* wrong eQuantizationTable, return error */
           eError = OMX_ErrorBadParameter;
        }
        break;
        }    
    case OMX_IndexCustomHuffmanTable:
        {
        JPEGENC_CUSTOM_HUFFMANTTABLETYPE *pHuffmanTable = (JPEGENC_CUSTOM_HUFFMANTTABLETYPE *)pComponentParameterStructure;
        memcpy(pHuffmanTable, pComponentPrivate->pHuffmanTable, sizeof(JPEGENC_CUSTOM_HUFFMANTTABLETYPE));
        break;
        }
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    JPEGENC_DPRINT("Exit function eError = %x\n", eError);
    return eError;

}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_SetParameter() Gets the parameters sent by the Application and sets it to the component.
  *
  * This method will update component parameters .
  *
  *
  * @param hComp         handle for this instance of the component
  * @param nParamIndex    Component Params Index Port
  * @param ComponentParameterStructure Component Parameter Structure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_SetParameter (OMX_HANDLETYPE hComponent, 
                                           OMX_INDEXTYPE nParamIndex,
                                           OMX_PTR pCompParam)
{
    OMX_COMPONENTTYPE         * pHandle           = NULL;
    JPEGENC_COMPONENT_PRIVATE * pComponentPrivate = NULL;
    OMX_ERRORTYPE               eError            = OMX_ErrorNone;
    JPEG_PORT_TYPE            *pInpPortType       = NULL;
    JPEG_PORT_TYPE            *pOutPortType       = NULL;
#ifdef KHRONOS_1_1
    OMX_PARAM_COMPONENTROLETYPE  *pRole;
#endif

    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pCompParam);
    
    pHandle= (OMX_COMPONENTTYPE*)hComponent;
    pComponentPrivate = pHandle->pComponentPrivate; 
    
    if ( pComponentPrivate->nCurState != OMX_StateLoaded ) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    pInpPortType = pComponentPrivate->pCompPort[JPEGENC_INP_PORT];
    pOutPortType = pComponentPrivate->pCompPort[JPEGENC_OUT_PORT];

    switch ( nParamIndex ) {
    case OMX_IndexParamImagePortFormat:
    {
        OMX_IMAGE_PARAM_PORTFORMATTYPE* pComponentParam = (OMX_IMAGE_PARAM_PORTFORMATTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pInpPortType->pPortFormat->nPortIndex )
        {
            memcpy(pInpPortType->pPortFormat, pComponentParam, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
            OMX_MEMCPY_CHECK(pInpPortType->pPortFormat);
        } 
        else if ( pComponentParam->nPortIndex == pOutPortType->pPortFormat->nPortIndex ) {
            memcpy(pOutPortType->pPortFormat, pComponentParam, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
            OMX_MEMCPY_CHECK(pOutPortType->pPortFormat);
        }
        else {
             eError = OMX_ErrorBadPortIndex;
        }
        break;
    }
    case OMX_IndexParamImageInit:
    {
        memcpy(pComponentPrivate->pPortParamType, (OMX_PORT_PARAM_TYPE*)pCompParam, sizeof(OMX_PORT_PARAM_TYPE));
        OMX_MEMCPY_CHECK(pComponentPrivate->pPortParamType);
        break;
    }
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE* pComponentParam = (OMX_PARAM_PORTDEFINITIONTYPE *)pCompParam;
        if ( pComponentParam->nPortIndex == pInpPortType->pPortDef->nPortIndex ) {
            memcpy(pInpPortType->pPortDef, pComponentParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            OMX_MEMCPY_CHECK(pInpPortType->pPortDef);
        } 
        else if ( pComponentParam->nPortIndex == pOutPortType->pPortDef->nPortIndex ) {
            memcpy(pOutPortType->pPortDef, pComponentParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
            OMX_MEMCPY_CHECK(pOutPortType->pPortDef);
        } 
        else {
            eError = OMX_ErrorBadPortIndex;
        }
        break;
    }
    case OMX_IndexParamPriorityMgmt:
    {
        memcpy(pComponentPrivate->pPriorityMgmt, (OMX_PRIORITYMGMTTYPE*)pCompParam, sizeof(OMX_PRIORITYMGMTTYPE));
        OMX_MEMCPY_CHECK(pComponentPrivate->pPriorityMgmt);
        break;
    }
    case OMX_IndexParamQFactor:
    {           
        memcpy(pComponentPrivate->pQualityfactor, (OMX_IMAGE_PARAM_QFACTORTYPE*)pCompParam, sizeof(OMX_IMAGE_PARAM_QFACTORTYPE));
        OMX_MEMCPY_CHECK(pComponentPrivate->pQualityfactor);
        break;
    }
    case OMX_IndexParamCompBufferSupplier:
    {
        OMX_PARAM_BUFFERSUPPLIERTYPE *pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE *)pCompParam;

        if ( pBuffSupplierParam->nPortIndex == 1 /*pOutPortType->pBuffSupplierParam->nPortIndex */) {
            /* Copy parameters to input port buffer supplier type */
            pOutPortType->pBufSupplier = pBuffSupplierParam->eBufferSupplier;
        } 
        else if ( pBuffSupplierParam->nPortIndex == 0 /* pInpPortType->pBuffSupplierParam->nPortIndex */ ) {
            pInpPortType->pBufSupplier = pBuffSupplierParam->eBufferSupplier;
        } 
        else {
            eError = OMX_ErrorBadPortIndex;
            break;
        }
        break;
    }
#ifdef KHRONOS_1_1
    case OMX_IndexParamStandardComponentRole:
        if (pCompParam) {
            pRole = (OMX_PARAM_COMPONENTROLETYPE *)pCompParam;
            memcpy(&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        } else {
            eError = OMX_ErrorBadParameter;
        }
        break;
#endif
   case OMX_IndexParamQuantizationTable:
    {
        OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *pQuantTable = (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *)pCompParam;
        if (pQuantTable->eQuantizationTable == OMX_IMAGE_QuantizationTableLuma) {
            memcpy(pComponentPrivate->pCustomLumaQuantTable, pQuantTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
            pComponentPrivate->bSetLumaQuantizationTable = OMX_TRUE;
            eError = SetJpegEncInParams(pComponentPrivate);
        } 
        else if (pQuantTable->eQuantizationTable == OMX_IMAGE_QuantizationTableChroma) {
            memcpy(pComponentPrivate->pCustomChromaQuantTable, pQuantTable, sizeof(OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE));
            pComponentPrivate->bSetChromaQuantizationTable = OMX_TRUE;   
            eError = SetJpegEncInParams(pComponentPrivate);            
        }
        else { /* wrong eQuantizationTable, return error */
           eError = OMX_ErrorBadParameter;
        }
        break;
    }
   case OMX_IndexCustomHuffmanTable:
    {
        JPEGENC_CUSTOM_HUFFMANTTABLETYPE *pHuffmanTable = (JPEGENC_CUSTOM_HUFFMANTTABLETYPE *)pCompParam;
        if (pHuffmanTable->nPortIndex == pOutPortType->pPortDef->nPortIndex) {
            memcpy(pComponentPrivate->pHuffmanTable, pHuffmanTable, sizeof(JPEGENC_CUSTOM_HUFFMANTTABLETYPE));
            pComponentPrivate->bSetHuffmanTable = OMX_TRUE;
            eError = SetJpegEncInParams(pComponentPrivate);            
        } else { /* wrong nPortIndex, return error */
           eError = OMX_ErrorBadPortIndex;
        }
        break;
    }
        break;  default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }
    
    EXIT:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_GetConfig() Gets Configuration from the Component
  *
  * This method will update component config structure .(NOT IMPLEMENTED)
  *
  * @param hComp         handle for this instance of the component
  * @param nConfigIndex   Component Config Index Port
  * @param ComponentConfigStructure Component Config Structure
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_GetConfig (OMX_HANDLETYPE hComp, 
                                        OMX_INDEXTYPE nConfigIndex,
                                        OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    JPEGENC_DPRINT ("Inside JPEGENC_GetConfig function\n");
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_SetConfig() Gets Configuration structure to the component.
  *
  * This method will update application callbacks
  * the application.
  *
  * @param pComp         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE JPEGENC_SetConfig (OMX_HANDLETYPE hComp, 
                                        OMX_INDEXTYPE nConfigIndex,
                                        OMX_PTR ComponentConfigStructure)
{
    OMX_ERRORTYPE      eError   = OMX_ErrorNone;
    OMX_COMPONENTTYPE* pHandle  = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate;
    
    OMX_CHECK_PARAM(hComp);

    pHandle = (OMX_COMPONENTTYPE *)hComp;

    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    switch ( nConfigIndex ) {
    case OMX_IndexCustomCommentFlag :
    {
        int *nComment = (int*)ComponentConfigStructure;
        if ( nComment == NULL ) {
            eError = OMX_ErrorBadParameter;
            goto EXIT;
        }
        ((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->nCommentFlag = *nComment;

        
        break;
    }
    case OMX_IndexCustomThumbnailAPP1_INDEX:
    {
        
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingEXIF){
            pComponentPrivate->ThumbnailInfo.APP1_THUMB_INDEX = * (int *)ComponentConfigStructure;
            if (pComponentPrivate->ThumbnailInfo.APP1_THUMB_INDEX) {
                pComponentPrivate->ThumbnailInfo.APP1_THUMB_W = 44;
                pComponentPrivate->ThumbnailInfo.APP1_THUMB_H = 36;
            }
            eError = SetJpegEncInParams(pComponentPrivate);
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;
    }
    case OMX_IndexCustomThumbnailAPP1_BUF:
    {
        APP_INFO *pAppInfo;
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingEXIF){
            pAppInfo = (APP_INFO *) ComponentConfigStructure;
            if(pComponentPrivate->ThumbnailInfo.APP1_BUF.app != NULL){
                free(pComponentPrivate->ThumbnailInfo.APP1_BUF.app);
                pComponentPrivate->ThumbnailInfo.APP1_BUF.app = NULL;
                pComponentPrivate->ThumbnailInfo.APP1_BUF.size = 0;
            }
            OMX_MALLOC_STRUCT_EXTRA(pComponentPrivate->ThumbnailInfo.APP1_BUF.app, OMX_U8, (pAppInfo->size-1));
            memcpy(pComponentPrivate->ThumbnailInfo.APP1_BUF.app, pAppInfo->app, pAppInfo->size);
            OMX_MEMCPY_CHECK(pComponentPrivate->ThumbnailInfo.APP1_BUF.app);
            pComponentPrivate->ThumbnailInfo.APP1_BUF.size = pAppInfo->size;

            eError = SetJpegEncInParams(pComponentPrivate);
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;
    }
    case OMX_IndexCustomThumbnailAPP1_W:
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingEXIF ){
            if (pComponentPrivate->ThumbnailInfo.APP1_THUMB_INDEX) {
                pComponentPrivate->ThumbnailInfo.APP1_THUMB_W = * (int *)ComponentConfigStructure;
                eError = SetJpegEncInParams(pComponentPrivate);
            }
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;
    case OMX_IndexCustomThumbnailAPP1_H:
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingEXIF){
            if (pComponentPrivate->ThumbnailInfo.APP1_THUMB_INDEX) {
                pComponentPrivate->ThumbnailInfo.APP1_THUMB_H = * (int *)ComponentConfigStructure;
                eError = SetJpegEncInParams(pComponentPrivate);
            }
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;

    case OMX_IndexCustomThumbnailAPP0_INDEX:
    {
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingJPEG){
            pComponentPrivate->ThumbnailInfo.APP0_THUMB_INDEX = * (int *)ComponentConfigStructure;
            if (pComponentPrivate->ThumbnailInfo.APP0_THUMB_INDEX) {
                pComponentPrivate->ThumbnailInfo.APP0_THUMB_W = 44;
                pComponentPrivate->ThumbnailInfo.APP0_THUMB_H = 32;
            }
            eError = SetJpegEncInParams(pComponentPrivate);
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;
    }
    case OMX_IndexCustomThumbnailAPP0_BUF:
    {
        APP_INFO *pAppInfo;
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingJPEG ){
            pAppInfo = (APP_INFO *) ComponentConfigStructure;
            if(pComponentPrivate->ThumbnailInfo.APP0_BUF.app != NULL){
                free(pComponentPrivate->ThumbnailInfo.APP0_BUF.app);
                pComponentPrivate->ThumbnailInfo.APP0_BUF.app = NULL;
                pComponentPrivate->ThumbnailInfo.APP0_BUF.size = 0;
            }
            OMX_MALLOC_STRUCT_EXTRA(pComponentPrivate->ThumbnailInfo.APP0_BUF.app, OMX_U8, (pAppInfo->size-1));
            memcpy(pComponentPrivate->ThumbnailInfo.APP0_BUF.app, pAppInfo->app, pAppInfo->size);
            OMX_MEMCPY_CHECK(pComponentPrivate->ThumbnailInfo.APP0_BUF.app);
            pComponentPrivate->ThumbnailInfo.APP0_BUF.size = pAppInfo->size;

            SetJpegEncInParams(pComponentPrivate);
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }

        break;
    }
    case OMX_IndexCustomThumbnailAPP0_W:
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingJPEG ){
            if (pComponentPrivate->ThumbnailInfo.APP0_THUMB_INDEX) {
                pComponentPrivate->ThumbnailInfo.APP0_THUMB_W = * (int *)ComponentConfigStructure;
                eError = SetJpegEncInParams(pComponentPrivate);
            }
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;
    case OMX_IndexCustomThumbnailAPP0_H:
        if(pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->format.image.eCompressionFormat == OMX_IMAGE_CodingJPEG ){
            if (pComponentPrivate->ThumbnailInfo.APP0_THUMB_INDEX) {
                pComponentPrivate->ThumbnailInfo.APP0_THUMB_H = * (int *)ComponentConfigStructure;
                eError = SetJpegEncInParams(pComponentPrivate);
            }
        }
        else{
            eError = OMX_ErrorUnsupportedIndex;
        }
        break;

    case OMX_IndexCustomThumbnailAPP13_INDEX:
    {
        pComponentPrivate->ThumbnailInfo.APP13_THUMB_INDEX = * (int *)ComponentConfigStructure;
        if (pComponentPrivate->ThumbnailInfo.APP13_THUMB_INDEX) {
            pComponentPrivate->ThumbnailInfo.APP13_THUMB_W = 44;
            pComponentPrivate->ThumbnailInfo.APP13_THUMB_H = 36;
        }
        eError = SetJpegEncInParams(pComponentPrivate);

        break;
    }
    case OMX_IndexCustomThumbnailAPP13_BUF:
    {
        APP_INFO *pAppInfo;

        pAppInfo = (APP_INFO *) ComponentConfigStructure;
        if(pComponentPrivate->ThumbnailInfo.APP13_BUF.app != NULL){
            free(pComponentPrivate->ThumbnailInfo.APP13_BUF.app);
            pComponentPrivate->ThumbnailInfo.APP13_BUF.app = NULL;
            pComponentPrivate->ThumbnailInfo.APP13_BUF.size = 0;
        }
        OMX_MALLOC_STRUCT_EXTRA(pComponentPrivate->ThumbnailInfo.APP13_BUF.app, OMX_U8, (pAppInfo->size-1));
        memcpy(pComponentPrivate->ThumbnailInfo.APP13_BUF.app, pAppInfo->app, pAppInfo->size);
        OMX_MEMCPY_CHECK(pComponentPrivate->ThumbnailInfo.APP13_BUF.app);
        pComponentPrivate->ThumbnailInfo.APP13_BUF.size = pAppInfo->size;
        
        eError = SetJpegEncInParams(pComponentPrivate);

        break;
    }
    case OMX_IndexCustomThumbnailAPP13_W:
        
        if (pComponentPrivate->ThumbnailInfo.APP13_THUMB_INDEX) {
            pComponentPrivate->ThumbnailInfo.APP13_THUMB_W = * (int *)ComponentConfigStructure;
            eError = SetJpegEncInParams(pComponentPrivate);
        }
        
        break;
    case OMX_IndexCustomThumbnailAPP13_H:
        
        if (pComponentPrivate->ThumbnailInfo.APP13_THUMB_INDEX) {
            pComponentPrivate->ThumbnailInfo.APP13_THUMB_H = * (int *)ComponentConfigStructure;
            eError = SetJpegEncInParams(pComponentPrivate);
        }
        
        break;
    case OMX_IndexCustomDRI:
        pComponentPrivate->nDRI_Interval = *(OMX_U8 *)ComponentConfigStructure;
        break;
    case OMX_IndexCustomCommentString : 
    {
        if(((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pString_Comment == NULL){
            OMX_MALLOC_STRUCT_EXTRA(((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pString_Comment ,
                                                             OMX_U8, 255);
        }
        strncpy((char *)((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pString_Comment, (char *)ComponentConfigStructure, 255);
        eError = SetJpegEncInParams(pComponentPrivate);
        break;
    }
    case OMX_IndexCustomInputFrameWidth:
    {
        OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
        int *nWidth= (int*)ComponentConfigStructure;

        pPortDefIn = ((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pCompPort[JPEGENC_INP_PORT]->pPortDef;
        JPEGENC_DPRINT("INIT nFrameHeight = %d\n", pPortDefIn->format.image.nFrameHeight);
        JPEGENC_DPRINT("INIT nFrameWidth = %d\n", pPortDefIn->format.image.nFrameWidth);
        pPortDefIn->format.image.nFrameWidth = *nWidth;
        JPEGENC_DPRINT("nFrameWidth = %d\n", pPortDefIn->format.image.nFrameWidth);
#if 0
        eError = SendDynamicParam(pComponentPrivate);
            if (eError != OMX_ErrorNone ) {
                JPEGENC_DPRINT("SETSTATUS failed...  %x\n", eError);
                goto EXIT;
        }
#endif
        break;
    }
    case OMX_IndexCustomInputFrameHeight:
    {
        OMX_PARAM_PORTDEFINITIONTYPE* pPortDefIn = NULL;
        int *nHeight = (int*)ComponentConfigStructure;

        pPortDefIn = ((JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate)->pCompPort[JPEGENC_INP_PORT]->pPortDef;
        pPortDefIn->format.image.nFrameHeight = *nHeight;
        JPEGENC_DPRINT("nFrameHeight = %d\n", pPortDefIn->format.image.nFrameHeight);
#if 0
        eError = SendDynamicParam(pComponentPrivate);
            if (eError != OMX_ErrorNone ) {
                JPEGENC_DPRINT("SETSTATUS failed...  %x\n", eError);
                goto EXIT;
        }
#endif
        break;
    }
    case OMX_IndexCustomQFactor:
    {           
        memcpy(pComponentPrivate->pQualityfactor, (OMX_IMAGE_PARAM_QFACTORTYPE*)ComponentConfigStructure, sizeof(OMX_IMAGE_PARAM_QFACTORTYPE));
        OMX_MEMCPY_CHECK(pComponentPrivate->pQualityfactor);
        break;
    }
    case OMX_IndexConfigCommonInputCrop :
    {
        OMX_CONFIG_RECTTYPE *crop = (OMX_CONFIG_RECTTYPE*)ComponentConfigStructure;
        if ((crop->nTop != 0) || (crop->nLeft != 0))
        {
            eError = OMX_ErrorBadParameter;
        }
        else{
            pComponentPrivate->pCrop->nWidth = crop->nWidth;
            pComponentPrivate->pCrop->nHeight = crop->nHeight;        
        }
        break;
    }
    
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }

    if (pComponentPrivate->nCurState == OMX_StateExecuting || pComponentPrivate->nCurState == OMX_StatePause) {
        eError = SendDynamicParam(pComponentPrivate);
            if (eError != OMX_ErrorNone ) {
                JPEGENC_DPRINT("SETSTATUS failed...  %x\n", eError);
                goto EXIT;
        }
    }

EXIT:
    JPEGENC_DPRINT ("Inside JPEGENC_SetConfig function\n");
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_GetState() Gets OMX Component State
  *
  * This method will return OMX Component State
  *
  * @param pComponent   handle for this instance of the component
  * @param pState       State type
  *
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_GetState (OMX_HANDLETYPE pComponent, OMX_STATETYPE* pState)
{
    OMX_ERRORTYPE eError        = OMX_ErrorNone;
    OMX_ERRORTYPE error         = OMX_ErrorUndefined;
    OMX_COMPONENTTYPE *pHandle  = NULL;
    OMX_CHECK_PARAM(pComponent);
    
    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    
    JPEGENC_DPRINT ("Inside JPEGENC_GetState function\n");
    if( !pState )
    {
        error = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if ( pHandle && pHandle->pComponentPrivate ) {
        *pState =  ((JPEGENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate)->nCurState;
    }       
    else 
    {
        *pState = OMX_StateLoaded;
    }
    error = OMX_ErrorNone;
    
EXIT:
    return error;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_EmptyThisBuffer() Send Input Buffers
  *
  * The application uses this macro to send the input buffers filled with data to the input port of the component
  *
  * @param pComponent    handle for this instance of the component
  * @param nPortIndex    PortIndex
  * @param pBuffHead     Pointer to data filled
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  *     OMX_ErrorInvalidState    Called in a invalid state
  *     OMX_ErrorHardware        Error in Writing to the Data pipe
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_EmptyThisBuffer (OMX_HANDLETYPE pComponent,
                                              OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE              eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE         *pHandle           = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    JPEGENC_BUFFER_PRIVATE    *pBuffPrivate;
    OMX_HANDLETYPE             hTunnelComponent;
    int i;
    int ret;

    OMX_CHECK_PARAM(pComponent);

    JPEGENC_DPRINT("inside JPEGENC_EmptyThisBuffer\n");

    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    hTunnelComponent = pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->hTunnelComponent;
    pBuffPrivate = pBuffHead->pInputPortPrivate;

    if ( pComponentPrivate->nCurState != OMX_StateExecuting && 
        pComponentPrivate->nCurState != OMX_StatePause && 
        pComponentPrivate->nCurState != OMX_StateIdle) {
        eError= OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    
    if ( pBuffHead == NULL ) {
        eError = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if ( pBuffHead->nSize != sizeof(OMX_BUFFERHEADERTYPE) ) {
        eError = OMX_ErrorBadParameter;
        JPEGENC_DPRINT("JPEG-ENC: buffer header size is not correct\n");
        goto EXIT;
    }

    if (!pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->bEnabled) {
        eError = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if ( (pBuffHead->nVersion.s.nVersionMajor != 0x1) || 
         (pBuffHead->nVersion.s.nVersionMinor != 0x0) || 
         (pBuffHead->nVersion.s.nRevision != 0x0) || 
         (pBuffHead->nVersion.s.nStep != 0x0) ) {
        
        eError= OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffHead->nInputPortIndex != 0x0) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
    }

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,
                           pBuffHead->pBuffer,
                           pBuffHead->nFilledLen,
                           PERF_ModuleHLMM);
#endif

    JPEGENC_DPRINT("pBuffHead->nAllocLen = %d\n",(int)pBuffHead->nAllocLen);
    JPEGENC_DPRINT("pBuffHead->nFilledLen = %d\n",(int)pBuffHead->nFilledLen);

    pComponentPrivate->nInPortIn ++;

    JPEGENC_DPRINT("EmptyThisBuffer nInPortIn %d\n", pComponentPrivate->nInPortIn);

    if (pBuffHead->nFlags == OMX_BUFFERFLAG_EOS) {
        JPEGENC_DPRINT("END OF STREAM DETECTED\n");
        pComponentPrivate->nFlags = OMX_BUFFERFLAG_EOS;
         /* pBuffHead->nFlags = 0; */
        JPEGENC_DPRINT("record EOS flag on buffer ID %d\n", pComponentPrivate->nInPortIn);
        for (i = 0; i < pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nBufferCountActual; i ++) {
            if (pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->sBufferFlagTrack[i].buffer_id == 0xFFFFFFFF) 
            {
                 JPEGENC_DPRINT("record buffer id in array %d\n", i);
                 pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->sBufferFlagTrack[i].flag = pBuffHead->nFlags;
                 pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->sBufferFlagTrack[i].buffer_id = 
                 pComponentPrivate->nInPortIn;
                 break;
             }
         }
     }

     /* mark the first buffer from input port after receiving mark buffer command */
     if (pComponentPrivate->nMarkPort == JPEGENC_INP_PORT) {
         if (pComponentPrivate->pMarkData) {
               JPEGENC_DPRINT("get mark buffer command, mark buffer %p\n", pBuffHead);
               pBuffHead->pMarkData = pComponentPrivate->pMarkData;
               pBuffHead->hMarkTargetComponent = pComponentPrivate->hMarkTargetComponent;
               pComponentPrivate->pMarkData = NULL;
               pComponentPrivate->hMarkTargetComponent = NULL;
         }
     }

     /* if a buffer from input port is marked, record this port # in the buffer queue */
     if (pBuffHead->pMarkData) {
         for (i = 0; i < pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->nBufferCountActual; i ++) {
             if (pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->sBufferMarkTrack[i].buffer_id == 0xFFFFFFFF)
             {
                 JPEGENC_BUFFERMARK_TRACK *pMarkTrack;
                 pMarkTrack = &(pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->sBufferMarkTrack[i]);
                 pMarkTrack->buffer_id = pComponentPrivate->nInPortIn;
                 pMarkTrack->pMarkData = pBuffHead->pMarkData;
                 pMarkTrack->hMarkTargetComponent = pBuffHead->hMarkTargetComponent;
                 JPEGENC_DPRINT("mark buffer at ID %u\n", pComponentPrivate->nInPortIn);
                 break;
             }
         }
     }

#if 0
    eError = SendDynamicParam(pComponentPrivate);
    if (eError != OMX_ErrorNone ) {
            JPEGENC_DPRINT("SETSTATUS failed...  %x\n", eError);
            goto EXIT;
    }
#endif

     pBuffPrivate->eBufferOwner = JPEGENC_BUFFER_COMPONENT_IN;
     pBuffPrivate->bReadFromPipe = OMX_FALSE;

     JPEGENC_DPRINT("buffer summary (get filled input buffer) %d %d %d %d (%p)\n",
                    pComponentPrivate->nInPortIn,
                    pComponentPrivate->nInPortOut,
                    pComponentPrivate->nOutPortIn,
                    pComponentPrivate->nOutPortOut, pBuffHead);       

     /*Writing the component buffer corresponding to input buffer to infill_q  */
     JPEGENC_DPRINT("EmptyThisBuffer: write to the queue %p \n", pBuffHead);

     ret = write (pComponentPrivate->filled_inpBuf_Q[1], &(pBuffHead),sizeof(pBuffHead));
    
     if ( ret == -1 ) {
         JPEGENC_DPRINT ("Error in Writing to the Data pipe\n");
         eError = OMX_ErrorHardware;
         goto EXIT;
     }

EXIT:
    return eError;
}


/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_FillThisBuffer() Empty an output Buffer
  *
  * The component uses this to request the application to
  * empty an output buffer which contains the decoded data..
  * @param pComponent         handle for this instance of the component
  * @param pCallBacks    application callbacks
  * @param ptr
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
static OMX_ERRORTYPE JPEGENC_FillThisBuffer (OMX_HANDLETYPE pComponent,
                                     OMX_BUFFERHEADERTYPE* pBuffHead)
{
    OMX_ERRORTYPE eError        = OMX_ErrorNone;
    OMX_ERRORTYPE err           = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle = NULL;
    JPEGENC_BUFFER_PRIVATE* pBuffPrivate = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;


    
    OMX_CHECK_PARAM(pComponent);
    OMX_CHECK_PARAM(pBuffHead);

    pHandle = (OMX_COMPONENTTYPE *)pComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pBuffPrivate = pBuffHead->pOutputPortPrivate;

    JPEGENC_DPRINT("Inside JPEGENC_FillThisBuffer function (%p) (state %d -> %d)\n", pBuffHead, pComponentPrivate->nCurState, pComponentPrivate->nToState);

    if ( pComponentPrivate->nCurState != OMX_StateExecuting && 
        pComponentPrivate->nCurState != OMX_StatePause && 
        pComponentPrivate->nCurState != OMX_StateIdle) {
        eError= OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }

    if ( pBuffHead->nSize != sizeof(OMX_BUFFERHEADERTYPE) ) {
        eError = OMX_ErrorBadParameter;
        JPEGENC_DPRINT("JPEG-ENC: buffer header size is not correct\n");
        goto EXIT;
    }

    if (!pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pPortDef->bEnabled) {
            eError = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
    }

    if ( (pBuffHead->nVersion.s.nVersionMajor != 0x1) || 
         (pBuffHead->nVersion.s.nVersionMinor != 0x0) || 
         (pBuffHead->nVersion.s.nRevision != 0x0) || 
         (pBuffHead->nVersion.s.nStep != 0x0) ) {
        
        eError= OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    if (pBuffHead->nOutputPortIndex != 0x1) {
            eError = OMX_ErrorBadPortIndex;
            goto EXIT;
    }

    pBuffHead->nFilledLen = 0;
#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,
                           pBuffHead->pBuffer,
                           0,
                           PERF_ModuleHLMM);
#endif

    pBuffPrivate->eBufferOwner = JPEGENC_BUFFER_COMPONENT_IN;
    pBuffPrivate->bReadFromPipe = OMX_FALSE;


    pComponentPrivate->nOutPortIn ++;

    JPEGENC_DPRINT("FillThisBuffer nOutPortIn %d\n", pComponentPrivate->nOutPortIn);

    JPEGENC_DPRINT("buffer summary (get empty output buffer) %d %d %d %d\n",
                    pComponentPrivate->nInPortIn,
                    pComponentPrivate->nInPortOut,
                    pComponentPrivate->nOutPortIn,
                    pComponentPrivate->nOutPortOut);       

    JPEGENC_DPRINT("Inside JPEGENC_FillThisBuffer function writing to pipe\n");
    /*  writing the component structure corresponding to output buffer    */
     JPEGENC_DPRINT("FillThisBuffer: write to the queue %p \n", pBuffHead);
    err = write (pComponentPrivate->free_outBuf_Q[1], &(pBuffHead), sizeof (pBuffHead));
    if (err == -1) {
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    JPEGENC_DPRINT("Error is %x\n",eError);
    EXIT:
    return eError;

}


/*-------------------------------------------------------------------*/
/**
  * OMX_ComponentDeinit() Deinitialize Component
  *
  * This method will clean all resources in the component (NOT IMPLEMENTED)
  *
  * @param pHandle        handle for this instance of the component
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/

static OMX_ERRORTYPE JPEGENC_ComponentDeInit(OMX_HANDLETYPE hComponent)
{
	OMX_ERRORTYPE eError        = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle  = NULL;

	OMX_CHECK_PARAM(hComponent);
	pHandle = (OMX_COMPONENTTYPE *)hComponent;

	JPEGEnc_Free_ComponentResources(pHandle->pComponentPrivate);

#ifdef RESOURCE_MANAGER_ENABLED
	eError = RMProxy_NewSendCommand(pHandle,  RMProxy_FreeResource, OMX_JPEG_Encoder_COMPONENT, 0, 3456, NULL);
	if (eError != OMX_ErrorNone) {
		JPEGENC_DPRINT("Cannot Free RMProxy Resources\n");
	}

	eError = RMProxy_DeinitalizeEx(OMX_COMPONENTTYPE_IMAGE);
	if ( eError != OMX_ErrorNone )  {
		JPEGENC_DPRINT ("Error returned from destroy ResourceManagerProxy thread\n");
	}

#endif
    
EXIT:
	return eError;
}



/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_VerifyTunnelConnection() 
  *
  * 
  * 
  *
  * @param         
  * @param    
  * @param 
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
/* TODO: Component Name */

OMX_ERRORTYPE JPEGENC_VerifyTunnelConnection (JPEG_PORT_TYPE *pPort, 
                                              OMX_HANDLETYPE hTunneledComp, 
                                              OMX_PARAM_PORTDEFINITIONTYPE* pPortDef) 
{
   OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
   OMX_ERRORTYPE eError = OMX_ErrorNone;
   /*sPortDef.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;*/

   JPEGENC_DPRINT("Inside JPEG JPEGENC_VerifyTunnelConnection..\n");
   sPortDef.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
   sPortDef.nVersion.s.nVersionMajor = 0x1;
   sPortDef.nVersion.s.nVersionMinor = 0x0;
   sPortDef.nPortIndex = pPort->nTunnelPort;
   
   eError = OMX_GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &sPortDef);
   if (eError != OMX_ErrorNone) {
       JPEGENC_DPRINT("error 1\n");
       return eError;
   }

   switch(pPort->pPortDef->eDomain) {
       case OMX_PortDomainOther:
           if (sPortDef.format.other.eFormat!= pPortDef->format.other.eFormat) {
                pPort->hTunnelComponent = 0; 
                pPort->nTunnelPort      = 0;
                return OMX_ErrorPortsNotCompatible;
            }
            break;
        case OMX_PortDomainAudio:
            if (sPortDef.format.audio.eEncoding != pPortDef->format.audio.eEncoding) {
                pPort->hTunnelComponent = 0; 
                pPort->nTunnelPort      = 0;
                return OMX_ErrorPortsNotCompatible;
            }
            break;
        case OMX_PortDomainVideo:
            if (sPortDef.format.video.eCompressionFormat != pPortDef->format.video.eCompressionFormat) {
                pPort->hTunnelComponent = 0; 
                pPort->nTunnelPort      = 0;
                return OMX_ErrorPortsNotCompatible;
            }
            break;
        case OMX_PortDomainImage:
            if (sPortDef.format.image.eCompressionFormat != pPortDef->format.image.eCompressionFormat) {
                pPort->hTunnelComponent = 0; 
                pPort->nTunnelPort      = 0;
                return OMX_ErrorPortsNotCompatible;
            }
            break;
        default: 
            pPort->hTunnelComponent = 0;
            pPort->nTunnelPort      = 0;
            return OMX_ErrorPortsNotCompatible; /* Our current port is not set up correctly */
   }
    return eError;
}

/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_ComponentTunnelRequest() Sets application callbacks to the component
  *
  * This method will update application callbacks
  * the application.
  *
  * @param hComp         handle for this instance of the component
  * @param nPortInput
  * @param hTunneledComp
  * @param nTunneledPort
  * @param eDir
  * @param pCallbacks
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  *
static OMX_ERRORTYPE JPEGENC_ComponentTunnelRequest (OMX_HANDLETYPE hComp,
                                             OMX_U32 nPortInput, 
                                             OMX_HANDLETYPE hTunneledComp,
                                             OMX_U32 nTunneledPort, 
                                             OMX_DIRTYPE eDir, 
                                             OMX_CALLBACKTYPE* pCallbacks)

-------------------------------------------------------------------*/

OMX_ERRORTYPE JPEGENC_ComponentTunnelRequest(OMX_IN  OMX_HANDLETYPE hComponent, 
                                     OMX_IN  OMX_U32 nPort,
                                     OMX_IN  OMX_HANDLETYPE hTunneledComp,
                                     OMX_IN  OMX_U32 nTunneledPort,
                                     OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    OMX_ERRORTYPE eError            = OMX_ErrorNone;
    OMX_COMPONENTTYPE *pHandle  = NULL;
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
    OMX_PARAM_BUFFERSUPPLIERTYPE sBufferSupplier;
    JPEG_PORT_TYPE *pPortType           = NULL;

    OMX_CHECK_PARAM(hComponent);
    

    pHandle = (OMX_COMPONENTTYPE *)hComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
    pPortType = pComponentPrivate->pCompPort[nPort];
    
    JPEGENC_DPRINT("nPort = %d nTunneledPort = %d\n",(int)nPort, (int)nTunneledPort);
 
    if (pTunnelSetup == NULL || hTunneledComp == 0) 
    {
        /* cancel previous tunnel */
        pPortType->hTunnelComponent = 0;
        pPortType->nTunnelPort = 0;
        pPortType->pBufSupplier = OMX_BufferSupplyUnspecified;
        goto EXIT;
    }

    if (pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->eDir != OMX_DirInput && 
        pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef->eDir != OMX_DirOutput) {
        return OMX_ErrorBadParameter;
    }

     /* Check if the other component is developed by TI */
     if(IsTIOMXComponent(hTunneledComp) != OMX_TRUE) 
    {
       eError = OMX_ErrorTunnelingUnsupported;
       goto EXIT;
    }

    pPortType->hTunnelComponent = hTunneledComp;
    pPortType->nTunnelPort = nTunneledPort;
    JPEGENC_DPRINT("PP comp = %x, tunneled comp = %x\n",(int)hComponent, (int)pPortType->hTunnelComponent);

    if (pPortType->pPortDef->eDir == OMX_DirOutput) {
        /* Component is the output (source of data) */
        pTunnelSetup->eSupplier = pPortType->pBufSupplier;
    }
    else {
/*      Component is the input (sink of data) */
        eError = JPEGENC_VerifyTunnelConnection(pPortType, 
                                               hTunneledComp, 
                                               pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pPortDef); 
        if (OMX_ErrorNone != eError) {
            JPEGENC_DPRINT("########### Error !! PP JPEGENC_VerifyTunnelConnection inside JPEGfailed\n");
            /* Invalid connection formats. Return eError */
            return OMX_ErrorPortsNotCompatible;
        }
           
        /* If specified obey output port's preferences. Otherwise choose output */
        pPortType->pBufSupplier = pTunnelSetup->eSupplier;
        if (OMX_BufferSupplyUnspecified == pPortType->pBufSupplier) {
            pPortType->pBufSupplier = pTunnelSetup->eSupplier = OMX_BufferSupplyOutput;
        }

        /* Tell the output port who the supplier is */
        sBufferSupplier.nSize = sizeof(sBufferSupplier);
        sBufferSupplier.nVersion.s.nVersionMajor = 0x1;
        sBufferSupplier.nVersion.s.nVersionMinor = 0x0;
        sBufferSupplier.nPortIndex = nTunneledPort;
        sBufferSupplier.eBufferSupplier = pPortType->pBufSupplier;
        eError = OMX_SetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);

        eError = OMX_GetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &sBufferSupplier);
        if (sBufferSupplier.eBufferSupplier != pPortType->pBufSupplier) {
            JPEGENC_DPRINT("- JPEGENC_SetParameter: OMX_IndexParamCompBufferSupplier failed to change setting\n" );
            return OMX_ErrorUndefined;
        }
    }
    
EXIT:
    return eError;
}



/*-------------------------------------------------------------------*/
/**
  *  JPEGENC_AllocateBuffer()
  *
  *
  *
  *
  * @param
  * @param
  * @param
  *
  * @retval OMX_NoError              Success, ready to roll
  *         OMX_Error_BadParameter   The input parameter pointer is null
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE JPEGENC_AllocateBuffer(OMX_IN OMX_HANDLETYPE hComponent,
                             OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffHead,
                             OMX_IN OMX_U32 nPortIndex,
                             OMX_IN OMX_PTR pAppPrivate,
                             OMX_IN OMX_U32 nSizeBytes)
{
    OMX_COMPONENTTYPE            *pHandle           = NULL;
    JPEGENC_COMPONENT_PRIVATE    *pComponentPrivate = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef          = NULL;
    OMX_BUFFERHEADERTYPE         *pBufferHdr        = NULL;
    OMX_U32 nBufferCount = -1;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_U8* pTemp = NULL;
    
    pHandle           = (OMX_COMPONENTTYPE *) hComponent;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
    pPortDef          = pComponentPrivate->pCompPort[nPortIndex]->pPortDef;
    nBufferCount      = pComponentPrivate->pCompPort[nPortIndex]->nBuffCount;
  
    if (nBufferCount >= pPortDef->nBufferCountActual) {
        eError = OMX_ErrorInsufficientResources;
        JPEGENC_DPRINT(" try to allocate more buffers that the port supports\n");
        goto EXIT;     
    }

    if(nPortIndex == JPEGENC_INP_PORT) {
        OMX_MALLOC_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE);
        JPEGENC_DPRINT("Allocate Buffer Input pBufferPrivate = %p\n",pBufferHdr);

        OMX_CONF_INIT_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE);
        pBufferHdr->nOutputPortIndex = OMX_NOPORT;  
        pBufferHdr->nInputPortIndex = nPortIndex;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion = pPortDef->nVersion;
        pBufferHdr->pAppPrivate = pAppPrivate;
        pBufferHdr->nAllocLen = nSizeBytes;
        pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufferPrivate[nBufferCount]->bAllocByComponent = OMX_TRUE;
        pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr = pBufferHdr;
        *ppBuffHead = pBufferHdr;
    }
    else if(nPortIndex == JPEGENC_OUT_PORT) {
        OMX_MALLOC_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE);
        JPEGENC_DPRINT("Allocate Buffer Output pBufferPrivate[0] = %p\n", pBufferHdr);
        OMX_CONF_INIT_STRUCT(pBufferHdr, OMX_BUFFERHEADERTYPE); 
        pBufferHdr->nInputPortIndex = OMX_NOPORT;  
        pBufferHdr->nOutputPortIndex = nPortIndex;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion = pPortDef->nVersion;
        pBufferHdr->pAppPrivate = pAppPrivate;
        pBufferHdr->nAllocLen = nSizeBytes;
        pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pBufferPrivate[nBufferCount]->bAllocByComponent = OMX_TRUE;   
        pComponentPrivate->pCompPort[JPEGENC_OUT_PORT]->pBufferPrivate[nBufferCount]->pBufferHdr = pBufferHdr; 
        *ppBuffHead = pBufferHdr;
    }
    else {
        eError = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    OMX_MALLOC_STRUCT_EXTRA(pBufferHdr->pBuffer, OMX_U8, (nSizeBytes+255));

#ifdef __PERF_INSTRUMENTATION__
        PERF_ReceivedFrame(pComponentPrivate->pPERF,
                           pBufferHdr->pBuffer,
                           pBufferHdr->nFilledLen,
                           PERF_ModuleMemory);
#endif
  
    if (!pBufferHdr->pBuffer) {
        JPEGENC_DPRINT("Error: Malloc failed\n");
        eError = OMX_ErrorInsufficientResources;
        goto EXIT;
    }


    pTemp = (OMX_U8 *)(pBufferHdr->pBuffer);
    pTemp += 128;
    pBufferHdr->pBuffer = pTemp;
    JPEGENC_DPRINT("Allocate Buffer Input pBufferPrivate[0]-pBuffer = %p\n",pBufferHdr->pBuffer);
    

    if (nPortIndex == JPEGENC_INP_PORT) {
        pBufferHdr->pInputPortPrivate  = pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }
    else {
        pBufferHdr->pOutputPortPrivate = pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount];
    }

    /** The following 6 lines need to be changed **/
    if (pComponentPrivate->pCompPort[JPEGENC_INP_PORT]->hTunnelComponent != NULL) {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->eBufferOwner = JPEGENC_BUFFER_TUNNEL_COMPONENT;
    }
    else {
        pComponentPrivate->pCompPort[nPortIndex]->pBufferPrivate[nBufferCount]->eBufferOwner = JPEGENC_BUFFER_CLIENT;
    }
    
  
   pComponentPrivate->pCompPort[nPortIndex]->nBuffCount++;
   JPEGENC_DPRINT("JPEG-ENC: actual %d ask %d\n", 
        pComponentPrivate->pCompPort[nPortIndex]->nBuffCount, 
        pPortDef->nBufferCountActual);
    
   if (pComponentPrivate->pCompPort[nPortIndex]->nBuffCount == pPortDef->nBufferCountActual) {
       pPortDef->bPopulated = OMX_TRUE;

       pthread_mutex_lock(&pComponentPrivate->jpege_mutex_app);
       pthread_cond_signal(&pComponentPrivate->populate_cond);
       pthread_mutex_unlock(&pComponentPrivate->jpege_mutex_app);

       JPEGENC_InitBufferFlagTrack(pComponentPrivate, nPortIndex);
       JPEGENC_DPRINT(" Port [%d] Populated!\n", nPortIndex);
   }
    
EXIT:
    JPEGENC_DPRINT("Exiting pHandle = %d\n", (int)pHandle);
    return eError;
}


static void JPEGENC_InitBufferFlagTrack(
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate, 
    OMX_U32 nPortIndex)

{
    JPEG_PORT_TYPE *pPortType = NULL;
    int i;
        
     pPortType = pComponentPrivate->pCompPort[nPortIndex];

    /* assume  pPortType->pPortDef->nBufferCountActual <= NUM_OF_BUFFERSJPEG */
    for (i = 0; i < pPortType->pPortDef->nBufferCountActual; i ++) {
        pPortType->sBufferFlagTrack[i].flag = 0;
        pPortType->sBufferFlagTrack[i].buffer_id = 0xFFFFFFFF;
        pPortType->sBufferMarkTrack[i].buffer_id = 0xFFFFFFFF;
        pPortType->sBufferMarkTrack[i].pMarkData = NULL;
    }
}

#ifdef KHRONOS_1_1

static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
                OMX_OUT OMX_U8 *cRole,
                OMX_IN OMX_U32 nIndex)
{
    JPEGENC_COMPONENT_PRIVATE *pComponentPrivate;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    pComponentPrivate = (JPEGENC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);

    if(nIndex == 0){
      /* memcpy(cRole, &(pComponentPrivate->componentRole.cRole), sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE - 1); */
      strncpy((char *)cRole, (char *)pComponentPrivate->componentRole.cRole, sizeof(OMX_U8) * OMX_MAX_STRINGNAME_SIZE - 1); 
    }
    else {
      eError = OMX_ErrorNoMore;
        }

    return eError;
};
#endif



/*-------------------------------------------------------------------*/
/**
  * JPEGENC_GetExtensionIndex() 
  *
  * Return the index corresponding to the string.
  *
  * @retval OMX_ErrorNone                    Successful operation.
  *             OMX_ErrorUnsupportedIndex  If no index corresponding to the string is found
  **/
/*-------------------------------------------------------------------*/
OMX_ERRORTYPE JPEGENC_GetExtensionIndex(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    OMX_U16 nIndex;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;
    JPEGENC_CUSTOM_PARAM_DEFINITION sJpegEncCustomParams[] = {
    {"OMX.TI.JPEG.encoder.Config.HuffmanTable", OMX_IndexCustomHuffmanTable},    
    {"OMX.TI.JPEG.encoder.Config.CommentFlag", OMX_IndexCustomCommentFlag},
    {"OMX.TI.JPEG.encoder.Config.CommentString", OMX_IndexCustomCommentString},
    {"OMX.TI.JPEG.encoder.Config.InputFrameWidth", OMX_IndexCustomInputFrameWidth},
    {"OMX.TI.JPEG.encoder.Config.InputFrameHeight", OMX_IndexCustomInputFrameHeight},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP0_INDEX", OMX_IndexCustomThumbnailAPP0_INDEX},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP0_W", OMX_IndexCustomThumbnailAPP0_W},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP0_H", OMX_IndexCustomThumbnailAPP0_H},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP0_BUF", OMX_IndexCustomThumbnailAPP0_BUF},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP1_INDEX", OMX_IndexCustomThumbnailAPP1_INDEX},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP1_W", OMX_IndexCustomThumbnailAPP1_W},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP1_H", OMX_IndexCustomThumbnailAPP1_H},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP1_BUF", OMX_IndexCustomThumbnailAPP1_BUF},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP13_INDEX", OMX_IndexCustomThumbnailAPP13_INDEX},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP13_W", OMX_IndexCustomThumbnailAPP13_W},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP13_H", OMX_IndexCustomThumbnailAPP13_H},
    {"OMX.TI.JPEG.encoder.Config.ThumbnailAPP13_BUF", OMX_IndexCustomThumbnailAPP13_BUF},
    {"OMX.TI.JPEG.encoder.Config.QFactor", OMX_IndexCustomQFactor},
    {"OMX.TI.JPEG.encoder.Config.DRI", OMX_IndexCustomDRI},
    {"",0x0}
    };

    /* Check parameter validity */    
    OMX_CHECK_PARAM(hComponent);
    OMX_CHECK_PARAM(pIndexType);
    *pIndexType = OMX_IndexMax;

    for (nIndex = 0; strlen((const char*)sJpegEncCustomParams[nIndex].cCustomParamName); nIndex++){
        if (!strcmp((const char*)cParameterName, (const char*)(&(sJpegEncCustomParams[nIndex].cCustomParamName)))){
            *pIndexType = sJpegEncCustomParams[nIndex].nCustomParamIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

    if(*pIndexType == OMX_IndexMax){
         eError = OMX_ErrorUnsupportedIndex;
    }

EXIT:
    return eError;
}



