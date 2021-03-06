/*! \file CommonUtilMgrModule.cpp
 *  \brief CommonUtilMgr Module 
 *  \author Guolisen, LwDp
 *  \date   2012.10.17
 */
 
#define LWDP_MODULE_IMPL
#include <LwDp.h>
#include <PluginInc.h>
#include <iostream>

#include <Interface/ConfigMgr/Ix_ConfigMgr.h>
//#include <Interface/ScriptMgr/Ix_ScriptMgr.h>
#include "Cx_CommonUtilMgr.h"

LWDP_NAMESPACE_BEGIN;

XBEGIN_DEFINE_CLASS()
    XDEFINE_CLASSMAP_ENTRY_Singleton(MODID_CommonUtilMgr, CLSID_CommonUtilMgr, Cx_CommonUtilMgr)
XEND_DEFINE_CLASS_SYS();


DEF_MODULE_INFO_BEGIN(CommonUtilMgr, LWDP_TOP_DOMAIN, PlugLevel::LEVEL0, 0);
	DEC_INIT_FUN();
	DEC_UNINIT_FUN();
DEF_MODULE_INFO_END(CommonUtilMgr);


// Optional function to initialize this plugin when loaded by the plugin manager.
DEF_INIT_FUN(CommonUtilMgr)
{
	printf("CommonUtilMgr InitializePlugin\n");
	GET_OBJECT_RET(CommonUtilMgr, iCx_CommonUtilMgr, -1)
	RINOK(iCx_CommonUtilMgr->Init());
	
    return LWDP_OK;
}

// Optional function to free this plugin when unloaded by the plugin manager.
DEF_UNINIT_FUN(CommonUtilMgr)
{
	printf("CommonUtilMgr UninitializePlugin\n");
    return LWDP_OK;
}

LWDP_NAMESPACE_END;

