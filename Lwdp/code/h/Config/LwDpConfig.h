/*! \file LwDpConfig.h
 *  \brief Base Head
 *  \author Guolisen, LwDp
 *  \date   2011.07.14
 */

#ifndef LW_DP_CONFIG_H_
#define LW_DP_CONFIG_H_


EXTERN_C_BEGIN
//Lib Config
#define CFG_CONFIGMGR_DIR_STR 	 __T("../../../../code/bin/Plugins/ConfigMgrSo.so")
#define CFG_PLUGINLOADER_DIR_STR __T("../../../../code/bin/Plugins/PluginLoaderSo.so")



//Platform Module Config

#define _LWDP_NO_SYSTEM_SIZE_T
//#define _STDIO_DEFINED



//Plugin Module Config
//Use logMgr Module
#ifndef USE_LOG_MANAGER_DEF
#define USE_LOG_MANAGER_DEF 1
#endif

//print file name and line num in log
#ifndef USE_FILENAME_LINENUM_LOG
#define USE_FILENAME_LINENUM_LOG 1
#endif





EXTERN_C_END

#endif




 









