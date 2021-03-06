#include <LwDp.h>
#include <PluginInc.h>
#include <sstream>

#include <Interface/ConfigMgr/Ix_ConfigMgr.h>
#include <Interface/LogMgr/Ix_LogMgr.h>
#include <Interface/DbMgr/Ix_DbMgr.h>

#include "../Interface/ZmqBackend/Ix_ZmqBackend.h"
#include "../Interface/ACDevice/Ix_ACDevice.h"

#include "../Common/ExternalInterface.h"
#include "../Common/InternalInterface.h"
#include "../Common/ACInterface.h"

#include "ACDeviceDef.h"
#include "Cx_ACDevice.h"

#include <time.h>

std::string Cx_ACDevice::mInitSql   = std::string("");
std::string Cx_ACDevice::mConfigSql = std::string("");
std::string Cx_ACDevice::mCardSql   = std::string("");

XPropertyTable Cx_ACDevice::gateInfoTable;

uint32_     processPageSize  = LW_ACDEVICE_BULK_PAGESIZE_DEFAULT;
uint32_     gThreadCount     = LW_ACDEVICE_THREAD_NUM_DEFAULT;


typedef std::vector<std::string> RET_LIST;

struct WORKTHREAD_PARAM
{
	uint32_ startPos;
	uint32_ length;
	TS_ZMQ_SERVER_MSG* msg;
	Cx_ACDevice* object;
	DBHandle dbHandle;
};

void* work_thread(void* arg)
{	
	if(!arg)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
		           	   "ACD Work Thread Parameter is NULL");
		return NULL;
	}

	WORKTHREAD_PARAM* ptmpDomain = (WORKTHREAD_PARAM*)arg;	
	WORKTHREAD_PARAM Domain;
	Domain.startPos = ptmpDomain->startPos;
	Domain.length   = ptmpDomain->length;
	Domain.msg      = ptmpDomain->msg;
	ASSERT_CHECK(Domain.msg != 0);
	Domain.object   = ptmpDomain->object;
	ASSERT_CHECK(Domain.object != 0);
	Domain.dbHandle = ptmpDomain->dbHandle;
	DBHandle workDbHandle = Domain.dbHandle;
	ASSERT_CHECK(workDbHandle != 0);
	free(arg);
	ptmpDomain = NULL;


	TS_ZMQ_SERVER_MSG* zmqMsg = (TS_ZMQ_SERVER_MSG*)Domain.msg;
	TS_DEVICE_BULK_DATA_REQ_BODY* msgBody = (TS_DEVICE_BULK_DATA_REQ_BODY*)zmqMsg->customMsgBody;
	Cx_ACDevice* acdObj = Domain.object;

	int32_ inum = Domain.length;
	int32_ pageSize = (processPageSize > Domain.length)? Domain.length: processPageSize;
	int32_ reTimes  = (inum / pageSize);
		   reTimes += (inum%pageSize)>0?1:0;

	uint32_ blockNum  = 0;
	RET_LIST* errList = new RET_LIST;
	ASSERT_CHECK(errList != 0);
	uint32_ newLen = sizeof(TS_ZMQ_SERVER_MSG) + 
					 sizeof(TS_DEVICE_CARD_DATA_REQ_BODY);
	uint8_* fakeMsg = new uint8_[newLen];
	ASSERT_CHECK(fakeMsg != 0);
	memset(fakeMsg, 0, newLen);
	TS_DEVICE_CARD_DATA_REQ_BODY* tmpBody = NULL;
	tmpBody = (TS_DEVICE_CARD_DATA_REQ_BODY*)msgBody->cardDataEntry;

	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::NOTICE, 
					"Worker TotleNum: %d reTimes: %d pageSize:%d start: %d", inum, reTimes, pageSize, Domain.startPos);
	uint32_ nowSize = inum;
	for(blockNum = 0; blockNum<reTimes; blockNum++)
	{
		int32_ start = blockNum * pageSize + Domain.startPos;

		if(nowSize < pageSize)
			pageSize = nowSize;
		
		for(uint32_ i=start; i<start+pageSize; ++i)
		{
			memset(fakeMsg, 0, newLen);
			LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::INFO,
						   "[Received] REQ:%x REQCODE: %x, cardId: %s sceneryId: %s cardType: %x actionId: %x checkinTime: %x", 
					       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
					       zmqMsg->msgCode, 
					       std::string((char_*)tmpBody[i].cardId, sizeof(tmpBody[i].cardId)).c_str(),
						   std::string((char_*)tmpBody[i].sceneryId, sizeof(tmpBody[i].sceneryId)).c_str(),
					       tmpBody[i].cardType, tmpBody[i].actionId, tmpBody[i].checkinTime);

			TS_ZMQ_SERVER_MSG* fakeCardMsg = (TS_ZMQ_SERVER_MSG*)fakeMsg;
			memcpy(fakeCardMsg->deviceId, zmqMsg->deviceId, sizeof(fakeCardMsg->deviceId));
			fakeCardMsg->msgCode  = zmqMsg->msgCode;
			memcpy(fakeCardMsg->customMsgBody, &tmpBody[i], sizeof(TS_DEVICE_CARD_DATA_REQ_BODY));

			Data_Ptr tmpData;
			tmpData.reset();
			uint32_  len = 0;
			if(acdObj->DeviceCardDataMsgProcess(workDbHandle, fakeMsg, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_DEVICE_CARD_DATA_REQ_BODY),
										        tmpData, len) != LWDP_OK)
			{
				errList->push_back(std::string((char_*)tmpBody[i].cardId, sizeof(tmpBody[i].cardId)));
			}
	
		}

		nowSize -= pageSize;
	}

	DELETE_MULTIPLE(fakeMsg);
	GET_OBJECT_RET(DbMgr, iDbMgr, NULL);
	iDbMgr->Close(workDbHandle);

	if(errList->empty())
	{
		DELETE_SINGLE(errList);
		return NULL;
	}
	else
	{
		return errList;
	}

	return NULL;
}

Cx_ACDevice::Cx_ACDevice()
{
	printf("ACDevice Create!\n");
}

Cx_ACDevice::~Cx_ACDevice()
{
	printf("ACDevice Delete!\n");
}

LWRESULT Cx_ACDevice::Init()
{
	GET_OBJECT_RET(ConfigMgr, iConfigMgr, LWDP_GET_OBJECT_ERROR);
	Cx_ACDevice::mInitSql = std::string(LW_ACDEVICE_SELECT_DEVICE_DEFAULT);
	XPropertys propSelectForInit;
	iConfigMgr->GetModulePropEntry(LW_ACDEVICE_MODULE_NAME, LW_ACDEVICE_SELECT_FOR_INIT, propSelectForInit);
	if(!propSelectForInit[0].propertyText.empty())
	{
		Cx_ACDevice::mInitSql = propSelectForInit[0].propertyText;
	}
	else
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
					   "Can't Find <InitSql> In Config File, Default(%s)", 
					   Cx_ACDevice::mInitSql.c_str());
	}

	RINOK(iConfigMgr->GetModuleTable(LW_ACDEVICE_MODULE_NAME, LW_ACDEVICE_GATE_INFO_TABLE, 
									 Cx_ACDevice::gateInfoTable));

	Cx_ACDevice::mConfigSql = std::string(LW_ACDEVICE_CONFIG_DEVICE_DEFAULT);
	XPropertys propConfigSql;
	iConfigMgr->GetModulePropEntry(LW_ACDEVICE_MODULE_NAME, LW_ACDEVICE_CONFIG_SQL_STR, propConfigSql);
	if(!propConfigSql[0].propertyText.empty())
	{
		Cx_ACDevice::mConfigSql = propConfigSql[0].propertyText;
	}
	else
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
					   "Can't Find <ConfigSql> In Config File, Default(%s)", 
					   Cx_ACDevice::mConfigSql.c_str());
	}

	Cx_ACDevice::mCardSql = std::string(LW_ACDEVICE_INSERT_CARD_DATA_DEFAULT);
	XPropertys propCardSql;
	iConfigMgr->GetModulePropEntry(LW_ACDEVICE_MODULE_NAME, LW_ACDEVICE_CARD_SQL_STR, propCardSql);
	if(!propCardSql[0].propertyText.empty())
	{
		Cx_ACDevice::mCardSql = propCardSql[0].propertyText;
	}
	else
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
					   "Can't Find <CardSql> In Config File, Default(%s)", 
					   Cx_ACDevice::mCardSql.c_str());
	}
	
	XPropertys propThreadNum;
	iConfigMgr->GetModulePropEntry(LW_ACDEVICE_MODULE_NAME, LW_ACDEVICE_BULK_THREAD_NUM_NAME, propThreadNum);
	if(!propThreadNum[0].propertyText.empty())
	{
		gThreadCount = atol(propThreadNum[0].propertyText.c_str());
	}
	else
	{
		LWDP_LOG_PRINT("CT", LWDP_LOG_MGR::WARNING, 
					   "Can't Find <BulkThreadNum> In Config File, Default(%d)", 
					   gThreadCount);
	}	
	
	XPropertys propDomainSize;
	iConfigMgr->GetModulePropEntry(LW_ACDEVICE_MODULE_NAME, LW_ACDEVICE_BULK_PAGESIZE_NAME, propDomainSize);
	if(!propDomainSize[0].propertyText.empty())
	{
		processPageSize = atol(propDomainSize[0].propertyText.c_str());
	}
	else
	{
		LWDP_LOG_PRINT("CT", LWDP_LOG_MGR::WARNING, 
					   "Can't Find <BulkPageSize> In Config File, Default(%s)", 
					   processPageSize);
	}	

	GET_OBJECT_RET(ZmqBackend, iZmqBackend, LWDP_GET_OBJECT_ERROR);
	MsgDelegate regFun = MakeDelegate(this, &Cx_ACDevice::DeviceInitMsgProcess);
	RINOK(iZmqBackend->RegisteZmqMsg(TS_DEVICE_INIT_REQ_MSG, regFun));

	regFun = MakeDelegate(this, &Cx_ACDevice::DeviceConfigMsgProcess);
	RINOK(iZmqBackend->RegisteZmqMsg(TS_DEVICE_CONFIG_REQ_MSG, regFun));

	regFun = MakeDelegate(this, &Cx_ACDevice::DeviceHBMsgProcess);
	RINOK(iZmqBackend->RegisteZmqMsg(TS_DEVICE_HEART_BEAT_REQ_MSG, regFun));

	regFun = MakeDelegate(this, &Cx_ACDevice::DeviceCardDataMsgProcess);
	RINOK(iZmqBackend->RegisteZmqMsg(TS_DEVICE_CARD_DATA_REQ_MSG, regFun));

	regFun = MakeDelegate(this, &Cx_ACDevice::DeviceBulkDataMsgProcess);
	RINOK(iZmqBackend->RegisteZmqMsg(TS_DEVICE_BULK_DATA_REQ_MSG, regFun));
	
	return LWDP_OK;
}

LWRESULT MsgProcess(const uint8_* ret_msg, uint32_ ret_msg_len, 
					                 Data_Ptr& send_msg, uint32_& send_msg_len)
{
	//returnMsg.deviceId = zMsg->deviceId;
	//returnMsg.msgCode  = TS_SERVER_UNKNOW_MSG;

	uint8_* tmpstr = (uint8_*)new char[strlen("Hello ACD CallBack Client!") + 1];
	strcpy((char*)tmpstr, "Hello ACD CallBack Client!");
	Data_Ptr tmpData(tmpstr);
	send_msg = tmpData;
	send_msg_len = strlen("Hello ACD CallBack Client!") + 1;

	return LWDP_OK;
}


/*
enum TS_INIT_MSG_RESAULT_ENUM
{	
	TS_SERVER_CHECK_OK_RECONFIG = 1, //��֤�ɹ��������������?
	TS_SERVER_ID_ERROR,     //�豸IDδ֪����
	TS_SERVER_TYPE_ERROR,   //�豸���ʹ���
	TS_SERVER_UNKNOW_ERROR //δ֪���󣬾�������Ϣ
};
typedef struct stru_device_init_req_body
{	
	uint32_ deviceType;  //�豸����
	uint8_  sceneryId[8];  //����ID
	uint32_ checkResult; //�豸�Լ���
}TS_DEVICE_INIT_REQ_BODY;

typedef struct stru_server_init_rsp_body
{	
	uint32_  msgResult; //��Ϣ���ͽ��?
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ�
	uint32_  appendDataLength; //������ݳ���?
	uint8_*  appendData;
}TS_SERVER_INIT_RSP_BODY;
typedef struct stru_zmq_server_msg
{
	uint32_ deviceId;
	uint32_ msgCode;
	uint8_  customMsgBody[0];  //��Ϣ��
}TS_ZMQ_SERVER_MSG;
*/
LWRESULT Cx_ACDevice::DeviceInitMsgProcess(DBHandle db_handle, const uint8_* ret_msg, uint32_ ret_msg_len, 
								      Data_Ptr& send_msg, uint32_& send_msg_len)
{
	if(!ret_msg)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
					   "Cx_ACDevice::DeviceInitMsgProcess Param ret_msg is NULL");
		return LWDP_PARAMETER_ERROR;
	}
	
	TS_ZMQ_SERVER_MSG* zmqMsg = (TS_ZMQ_SERVER_MSG*)ret_msg;
	TS_DEVICE_INIT_REQ_BODY* msgBody = (TS_DEVICE_INIT_REQ_BODY*)zmqMsg->customMsgBody;
	if(!msgBody)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
				       "Msg(%d) Body Error", zmqMsg->msgCode);
		
		uint8_* errMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY)] ;
		ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, errMsg, 
						 "Cx_ACDevice::DeviceInitMsgProcess alloc memory error");

		memset(errMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY));
		TS_ZMQ_SERVER_MSG* errStru = (TS_ZMQ_SERVER_MSG*)errMsg;
		memcpy(errStru->deviceId, zmqMsg->deviceId, sizeof(errStru->deviceId));
		errStru->msgCode  = TS_SERVER_MSG_BODY_ERR;
		TS_SERVER_ERROR_BODY* errBody = (TS_SERVER_ERROR_BODY*)errStru->customMsgBody;
		errBody->errMsgCode = zmqMsg->msgCode;
		memcpy(errBody->errData, "Body Empty", 11);

		Data_Ptr tmpData(errMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY);
		return LWDP_OK;
	}

	//check
	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::INFO,
				   "[Received] REQ:%s REQCODE: %x, checkResult: %x deviceType: %x sceneryId: %s", 
			       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
			       zmqMsg->msgCode, msgBody->checkResult, msgBody->deviceType, 
				   std::string((char_*)msgBody->sceneryId, sizeof(msgBody->sceneryId)).c_str());

	char_ buffer[2048] = {0};
	Api_snprintf(buffer, 2047, Cx_ACDevice::mInitSql.c_str(), 
		         std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str());


	GET_OBJECT_RET(DbMgr, iDbMgr, LWDP_GET_OBJECT_ERROR);
	Cx_Interface<Ix_DbQuery> gateQuery;
	LWRESULT queryRes = 0;
	//iDbMgr->Ping();
	if((queryRes = iDbMgr->QuerySQL(db_handle, buffer, gateQuery)) != LWDP_OK)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
				       "Msg(%d) DB Select Table Error", zmqMsg->msgCode);
		
		uint8_* errMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY)] ;
		ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, errMsg, 
						 "Cx_ACDevice::DeviceInitMsgProcess alloc memory error");

		memset(errMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY));
		TS_ZMQ_SERVER_MSG* errStru = (TS_ZMQ_SERVER_MSG*)errMsg;
		memcpy(errStru->deviceId, zmqMsg->deviceId, sizeof(errStru->deviceId));
		errStru->msgCode  = TS_SERVER_DB_ERR;
		TS_SERVER_ERROR_BODY* errBody = (TS_SERVER_ERROR_BODY*)errStru->customMsgBody;
		errBody->errMsgCode = zmqMsg->msgCode;
		memcpy(errBody->errData, "Db Error", 9);

		Data_Ptr tmpData(errMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY);

		return LWDP_OK;
	}

	if(gateQuery->NumRow() == 0)
	{
		uint8_* returnMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + 
									   sizeof(TS_SERVER_INIT_RSP_BODY)];
		ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnMsg, "Malloc Error");

		memset(returnMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + 
							 sizeof(TS_SERVER_INIT_RSP_BODY));
		TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnMsg;
		memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
		retStru->msgCode  = TS_SERVER_INIT_RSP_MSG; //
		TS_SERVER_INIT_RSP_BODY* retBody = (TS_SERVER_INIT_RSP_BODY*)retStru->customMsgBody;
		retBody->msgResult = TS_SERVER_ID_ERROR;
		memcpy(retBody->msgResultData, "Can't Find ID", 14);

		Data_Ptr tmpData(returnMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + 
					   sizeof(TS_SERVER_INIT_RSP_BODY);
		
		return LWDP_OK;
	}

	uint32_ retCode = TS_SERVER_OK;
	char_*  retStr  = "Check OK!";
	std::string scidCol   = Cx_ACDevice::gateInfoTable[LW_ACDEVICE_GATE_INFO_SCENERY_ID_COL].propertyText;
	std::string scidValue = gateQuery->GetStringField(scidCol, "");
	ASSERT_CHECK_HALT(LWDP_PLUGIN_LOG, !scidValue.empty(), "scidValue Empty");
	std::string clientScId = std::string((char_ *)msgBody->sceneryId, sizeof(msgBody->sceneryId));
	if(scidValue != clientScId)
	{
		//retCode = TS_SERVER_CHECK_OK_RECONFIG;
		//retStr = "sceneryId reconfig";
	}

	std::string positionCol   = Cx_ACDevice::gateInfoTable[LW_ACDEVICE_GATE_INFO_POSITION_COL].propertyText;
	std::string positionValue = gateQuery->GetStringField(positionCol, "");
	ASSERT_CHECK_HALT(LWDP_PLUGIN_LOG, !positionValue.empty(), "positionValue Empty");
	uint32_ posInt = atol(positionValue.c_str());
	if(posInt != msgBody->position)
	{
		retCode = TS_SERVER_CHECK_OK_RECONFIG;
		retStr  = "position reconfig";
	}

	std::string typeCol   = Cx_ACDevice::gateInfoTable[LW_ACDEVICE_GATE_INFO_TYPE_COL].propertyText;
	std::string typeValue = gateQuery->GetStringField(typeCol, "");
	ASSERT_CHECK_HALT(LWDP_PLUGIN_LOG, !typeValue.empty(), "typeValue Empty");
	uint32_ typeInt = atol(typeValue.c_str());
	if(typeInt != msgBody->deviceType)
	{
		retCode = TS_SERVER_CHECK_OK_RECONFIG;
		retStr  = "gate type reconfig";
	}
	
	uint8_* returnMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + 
								   sizeof(TS_SERVER_INIT_RSP_BODY)];
	ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnMsg, "Malloc Error");

	memset(returnMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + 
						 sizeof(TS_SERVER_INIT_RSP_BODY));
	TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnMsg;
	memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
	retStru->msgCode  = TS_SERVER_INIT_RSP_MSG; //
	TS_SERVER_INIT_RSP_BODY* retBody = (TS_SERVER_INIT_RSP_BODY*)retStru->customMsgBody;
	retBody->msgResult = retCode;
	memcpy(retBody->msgResultData, retStr, strlen(retStr));

	Data_Ptr tmpData(returnMsg);
	send_msg = tmpData;
	send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + 
				   sizeof(TS_SERVER_INIT_RSP_BODY);
	
	return LWDP_OK;
}

/*
enum TS_SERVER_CONFIG_MSG_RESAULT_ENUM
{	
	TS_SERVER_CONFIG_ERROR = 1, //ʧ��
};

typedef struct stru_dev_config_body
{	
	uint32_  msgResult; //��Ϣ���ͽ��?
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ�
	uint32_  deviceId;  //�豸����
	uint32_  deviceType;  //�豸����
	uint8_   sceneryId[8];  //����ID
	uint8_   sceneryDomainId[8];  //����ID
	uint32_  sceneryPostion;  //ƫ��
}TS_DEV_CONFIG_BODY;
*/
LWRESULT Cx_ACDevice::DeviceConfigMsgProcess(DBHandle db_handle,const uint8_* ret_msg, uint32_ ret_msg_len, 
								      Data_Ptr& send_msg, uint32_& send_msg_len)
{
	if(!ret_msg)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
					   "Cx_ACDevice::DeviceInitMsgProcess Param ret_msg is NULL");
		return LWDP_PARAMETER_ERROR;
	}
	
	uint8_* returnMsg = NULL;
	Data_Ptr tmpData;
	TS_ZMQ_SERVER_MSG* zmqMsg = (TS_ZMQ_SERVER_MSG*)ret_msg;

	//check
	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::INFO,
				   "[Received] REQ:%s REQCODE: %x", 
			       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
			       zmqMsg->msgCode);

	char_ buffer[2048] = {0};
	Api_snprintf(buffer, 2047, Cx_ACDevice::mConfigSql.c_str(), 
				 std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str());

	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::DEBUG,
				   "Config Select Db Str(%s)", buffer);

	GET_OBJECT_RET(DbMgr, iDbMgr, LWDP_GET_OBJECT_ERROR);
	Cx_Interface<Ix_DbQuery> gateQuery;
	LWRESULT queryRes = 0;
	uint32_ retCode = 0;
	char_*  retMsg  = "NO ERROR!";
	//iDbMgr->Ping();
	if((queryRes = iDbMgr->QuerySQL(db_handle, buffer, gateQuery)) != LWDP_OK)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
				       "Msg(%d) DB Select(%s) Table Error", 
				       zmqMsg->msgCode, buffer);
		
		retCode = TS_SERVER_DB_ERR;
		retMsg  = "Db Error";
		goto ERR_RET;
	}

	if(gateQuery->NumRow() == 0)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
				       "Msg(%d) Can't Find id(%d) Select(%s) Table Error", 
				       zmqMsg->msgCode, 
				       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
				       buffer);
		
		retCode = TS_SERVER_ID_ERROR;
		retMsg  = "Can't Find ID";
		
		goto ERR_RET;
	}

	returnMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) +
							   sizeof(TS_DEV_CONFIG_RSP_BODY)];

	{
		ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnMsg, "Malloc Error");

		memset(returnMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + 
							 sizeof(TS_DEV_CONFIG_RSP_BODY));
		TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnMsg;
		memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
		retStru->msgCode  = TS_SERVER_CONFIG_RSP_MSG; //
		TS_DEV_CONFIG_RSP_BODY* retBody = (TS_DEV_CONFIG_RSP_BODY*)retStru->customMsgBody;
		retBody->msgResult = TS_SERVER_OK;
		memcpy(retBody->msgResultData, "OK!", 4);

		//scenery
		std::string scidCol   = Cx_ACDevice::gateInfoTable[LW_ACDEVICE_GATE_INFO_SCENERY_ID_COL].propertyText;
		std::string scidValue = gateQuery->GetStringField(scidCol, "");
		ASSERT_CHECK_HALT(LWDP_PLUGIN_LOG, !scidValue.empty(), "scidValue Empty");
		memcpy(retBody->sceneryId, scidValue.c_str(), sizeof(retBody->sceneryId));

		//position
		std::string positionCol   = Cx_ACDevice::gateInfoTable[LW_ACDEVICE_GATE_INFO_POSITION_COL].propertyText;
		std::string positionValue = gateQuery->GetStringField(positionCol, "");
		ASSERT_CHECK_HALT(LWDP_PLUGIN_LOG, !positionValue.empty(), "positionValue Empty");
		retBody->sceneryPostion = atol(positionValue.c_str());

		//type
		std::string typeCol   = Cx_ACDevice::gateInfoTable[LW_ACDEVICE_GATE_INFO_TYPE_COL].propertyText;
		std::string typeValue = gateQuery->GetStringField(typeCol, "");
		ASSERT_CHECK_HALT(LWDP_PLUGIN_LOG, !typeValue.empty(), "typeValue Empty");
		retBody->deviceType = atol(typeValue.c_str());

		//deviceId
		memcpy(retBody->deviceId, retStru->deviceId, sizeof(retBody->deviceId));
		//memcpy(retBody->sceneryDomainId, "1234567", 8);
	}

	tmpData.reset(returnMsg);
	send_msg = tmpData;
	send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + 
				   sizeof(TS_DEV_CONFIG_RSP_BODY);

	return LWDP_OK;
	
ERR_RET:
	uint8_* returnErrMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + 
								      sizeof(TS_DEV_CONFIG_RSP_BODY)];
	ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnErrMsg, "Malloc Error");

	memset(returnErrMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + 
					        sizeof(TS_DEV_CONFIG_RSP_BODY));
	TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnErrMsg;
	memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
	retStru->msgCode  = TS_SERVER_CONFIG_RSP_MSG; //
	TS_DEV_CONFIG_RSP_BODY* retBody = (TS_DEV_CONFIG_RSP_BODY*)retStru->customMsgBody;
	retBody->msgResult = retCode;
	memcpy(retBody->msgResultData, retMsg, strlen(retMsg));

	Data_Ptr tmpErrData(returnErrMsg);
	send_msg = tmpErrData;
	send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + 
				   sizeof(TS_DEV_CONFIG_RSP_BODY);
	return LWDP_OK;
}

/*
enum TS_SERVER_HB_MSG_RESAULT_ENUM
{	
	TS_SERVER_HB_ERROR = 1, //ʧ��
};

typedef struct stru_dev_status_req_body
{	
	uint32_ statusCode;  //״̬��
}TS_DEV_STATUS_REQ_BODY;

typedef struct stru_dev_status_rsp_body
{	
	uint32_  msgResult; //��Ϣ���ͽ��?
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ�
	uint32_  oprationCode;  //������
	uint32_  appendDataLength; //������ݳ���?
	uint8_   appendData[1];
}TS_DEV_STATUS_RSP_BODY;

*/
LWRESULT Cx_ACDevice::DeviceHBMsgProcess(DBHandle db_handle,const uint8_* ret_msg, uint32_ ret_msg_len, 
								      Data_Ptr& send_msg, uint32_& send_msg_len)
{
	if(!ret_msg)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
					   "Cx_ACDevice::DeviceInitMsgProcess Param ret_msg is NULL");
		return LWDP_PARAMETER_ERROR;
	}
	
	TS_ZMQ_SERVER_MSG* zmqMsg = (TS_ZMQ_SERVER_MSG*)ret_msg;
	TS_DEV_STATUS_REQ_BODY* msgBody = (TS_DEV_STATUS_REQ_BODY*)zmqMsg->customMsgBody;
	if(!msgBody)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
				       "Msg(%d) Body Error", zmqMsg->msgCode);
		
		uint8_* errMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY)] ;
		memset(errMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY));
		TS_ZMQ_SERVER_MSG* errStru = (TS_ZMQ_SERVER_MSG*)errMsg;
		memcpy(errStru->deviceId, zmqMsg->deviceId, sizeof(errStru->deviceId));
		errStru->msgCode  = TS_SERVER_MSG_BODY_ERR;
		TS_SERVER_ERROR_BODY* errBody = (TS_SERVER_ERROR_BODY*)errStru->customMsgBody;
		errBody->errMsgCode = zmqMsg->msgCode;
		memcpy(errBody->errData, "Body Empty", 11);

		Data_Ptr tmpData(errMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY);
		return LWDP_OK;
	}

	//check
	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::INFO,
				   "[Received] REQ:%s REQCODE: %x, statusCode: %x", 
			       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
			       zmqMsg->msgCode, msgBody->statusCode);


	uint8_* returnMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + 
								   sizeof(TS_DEV_STATUS_RSP_BODY)];
	ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnMsg, "Malloc Error");

	memset(returnMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + 
					     sizeof(TS_DEV_STATUS_RSP_BODY));
	TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnMsg;
	memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
	retStru->msgCode  = TS_SERVER_HEART_BEAT_RSP_MSG; //
	TS_DEV_STATUS_RSP_BODY* retBody = (TS_DEV_STATUS_RSP_BODY*)retStru->customMsgBody;
	retBody->msgResult = TS_SERVER_OK;
	memcpy(retBody->msgResultData, "No Error!", 10);

	Data_Ptr tmpData(returnMsg);
	send_msg = tmpData;
	send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + 
				   sizeof(TS_DEV_STATUS_RSP_BODY);
	
	return LWDP_OK;
}

/*
enum TS_CARD_DATA_MSG_RESAULT_ENUM
{	
	TS_SERVER_WRITE_DATA_ERROR = 1, //д��ݿ�ʧ��?
	TS_SERVER_UNKONW_ERROR,     //�豸IDδ֪����
};

typedef struct stru_dev_card_data_msg
{
	uint8_   cardId[8];  
    uint8_   sceneryId[8];
	uint32_  cardType;   //������
	uint32_  actionId;   //��ΪID
	time_t   checkinTime;
}TS_DEVICE_CARD_DATA_REQ_BODY;

typedef struct stru_dev_card_data_rsp_msg
{	
	uint32_  msgResult; //��Ϣ���ͽ��?
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ�
} TS_DEVICE_CARD_DATA_RSP_BODY;


*/

LWRESULT Cx_ACDevice::DeviceCardDataMsgProcess(DBHandle db_handle,const uint8_* ret_msg, uint32_ ret_msg_len, 
								      Data_Ptr& send_msg, uint32_& send_msg_len)
{
	if(!ret_msg)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
					   "Cx_ACDevice::DeviceInitMsgProcess Param ret_msg is NULL");
		return LWDP_PARAMETER_ERROR;
	}

	LWRESULT funStatus = 0;
	TS_ZMQ_SERVER_MSG* zmqMsg = (TS_ZMQ_SERVER_MSG*)ret_msg;
	TS_DEVICE_CARD_DATA_REQ_BODY* msgBody = (TS_DEVICE_CARD_DATA_REQ_BODY*)zmqMsg->customMsgBody;
	if(!msgBody)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
				       "Msg(%d) Body Error", zmqMsg->msgCode);
		
		uint8_* errMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY)] ;
		memset(errMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY));
		TS_ZMQ_SERVER_MSG* errStru = (TS_ZMQ_SERVER_MSG*)errMsg;
		memcpy(errStru->deviceId, zmqMsg->deviceId, sizeof(errStru->deviceId));
		errStru->msgCode  = TS_SERVER_MSG_BODY_ERR;
		TS_SERVER_ERROR_BODY* errBody = (TS_SERVER_ERROR_BODY*)errStru->customMsgBody;
		errBody->errMsgCode = zmqMsg->msgCode;
		memcpy(errBody->errData, "Body Empty", 11);

		Data_Ptr tmpData(errMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY);
		return LWDP_ERROR;
	}

	//check
	std::string carIdStr = std::string((char_*)msgBody->cardId, sizeof(msgBody->cardId));
	//IntArrayToStr(msgBody->cardId, TS_CARD_ID_STRU_LEN, carIdStr);
	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::INFO,
				   "[Received] REQ:%s REQCODE: %x, cardId: %s sceneryId: %s cardType: %x actionId: %x checkinTime: %x", 
			       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
			       zmqMsg->msgCode, carIdStr.c_str(), 
			       std::string((char_*)msgBody->sceneryId, sizeof(msgBody->sceneryId)).c_str(), 
			       msgBody->cardType, msgBody->actionId, msgBody->checkinTime);

	time_t timep = msgBody->checkinTime;
	struct tm checkTime = {0};
	localtime_r(&timep, &checkTime); 
	
	char_ bufTime[1024] = {0};
	strftime(bufTime, 1024, "%Y-%m-%d %H:%M:%S", &checkTime);

	char_ buffer[3072] = {0};
	Api_snprintf(buffer, 3071, Cx_ACDevice::mCardSql.c_str(),  carIdStr.c_str(), 
															   std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)-1).c_str(),
				  											   std::string((char_*)msgBody->sceneryId, sizeof(msgBody->sceneryId)-1).c_str(),
				  								               msgBody->cardType,
				  								               msgBody->actionId,
				  								               bufTime);
				  								               //msgBody->checkinTime);



	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::DEBUG,
				   "Select Db Str(%s)", buffer);
	GET_OBJECT_RET(DbMgr, iDbMgr, LWDP_GET_OBJECT_ERROR);
	//iDbMgr->Ping();
	Cx_Interface<Ix_DbQuery> gateQuery;
	int32_ queryRes = 0;
	uint32_ retCode = 0;
	char_*  retMsg  = "NO ERROR!";
	//iDbMgr->Ping();
	if((queryRes = iDbMgr->ExecSQL(db_handle, buffer)) != 1)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
				       "Msg(%d) DB Insert(%s) Table Error", 
				       zmqMsg->msgCode, buffer);

		retCode = TS_SERVER_DB_ERR;
		retMsg  = "Insert Error";
		funStatus = LWDP_ERROR;
	}


	uint8_* returnMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + 
								   sizeof(TS_DEVICE_CARD_DATA_RSP_BODY)];
	ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnMsg, "Malloc Error");

	memset(returnMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + 
						 sizeof(TS_DEVICE_CARD_DATA_RSP_BODY));
	TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnMsg;
	memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
	retStru->msgCode  = TS_SERVER_CARD_DATA_RSP_MSG; //
	TS_DEVICE_CARD_DATA_RSP_BODY* retBody = (TS_DEVICE_CARD_DATA_RSP_BODY*)retStru->customMsgBody;
	retBody->msgResult = retCode;
	memcpy(retBody->msgResultData, retMsg, strlen(retMsg));

	Data_Ptr tmpData(returnMsg);
	send_msg = tmpData;
	send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + 
				   sizeof(TS_DEVICE_CARD_DATA_RSP_BODY);
	
	return funStatus;
}



/*
enum TS_BULK_DATA_MSG_RESAULT_ENUM
{	
	TS_SERVER_BULK_WRITE_DATA_ERROR = 1, //д��ݿ�ʧ��?
	TS_SERVER_BULK_UNKONW_ERROR,     //�豸IDδ֪����
};

typedef struct stru_dev_bulk_data_msg
{  
	uint32_  cardDataCount;   //�������Ŀ����?
	uint8_*  cardDataEntry;   //��ΪID
}TS_DEVICE_BULK_DATA_REQ_BODY;

typedef struct stru_dev_bulk_data_rsp_msg
{	
	uint32_  msgResult; //��Ϣ���ͽ��?
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ�
	uint32_  errorEntryNum;
	uint8_   errCardId[1]; //errorEntryNum * 8
} TS_DEVICE_BULK_DATA_RSP_BODY;
*/

LWRESULT Cx_ACDevice::DeviceBulkDataMsgProcess(DBHandle db_handle,const uint8_* ret_msg, uint32_ ret_msg_len, 
								      Data_Ptr& send_msg, uint32_& send_msg_len)
{
	if(!ret_msg)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
					   "Cx_ACDevice::DeviceInitMsgProcess Param ret_msg is NULL");
		return LWDP_PARAMETER_ERROR;
	}
	
	TS_ZMQ_SERVER_MSG* zmqMsg = (TS_ZMQ_SERVER_MSG*)ret_msg;
	TS_DEVICE_BULK_DATA_REQ_BODY* msgBody = (TS_DEVICE_BULK_DATA_REQ_BODY*)zmqMsg->customMsgBody;
	if(!msgBody)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::WARNING, 
				       "Msg(%d) Body Error", zmqMsg->msgCode);
		
		uint8_* errMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY)] ;
		memset(errMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY));
		TS_ZMQ_SERVER_MSG* errStru = (TS_ZMQ_SERVER_MSG*)errMsg;
		memcpy(errStru->deviceId, zmqMsg->deviceId, sizeof(errStru->deviceId));
		errStru->msgCode  = TS_SERVER_MSG_BODY_ERR;
		TS_SERVER_ERROR_BODY* errBody = (TS_SERVER_ERROR_BODY*)errStru->customMsgBody;
		errBody->errMsgCode = zmqMsg->msgCode;
		memcpy(errBody->errData, "Body Empty", 11);

		Data_Ptr tmpData(errMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY);
		return LWDP_OK;
	}

	//check	
	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::NOTICE,
				   "[Received] REQ:%x REQCODE: %x, cardDataCount: %x", 
			       std::string((char_*)zmqMsg->deviceId, sizeof(zmqMsg->deviceId)).c_str(), 
			       zmqMsg->msgCode, msgBody->cardDataCount);

	uint32_ tmpLen = sizeof(TS_ZMQ_SERVER_MSG) + 
		             sizeof(TS_DEVICE_BULK_DATA_REQ_BODY) +
				     sizeof(TS_DEVICE_CARD_DATA_REQ_BODY) * msgBody->cardDataCount;
	if(ret_msg_len < tmpLen)
	{
		LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR,
					   "Request Data is too Small,ret_msg_len(%d) Should(%d) cardDataCount(%d)", 
					   ret_msg_len, tmpLen, msgBody->cardDataCount);
		
		uint8_* errMsg = new uint8_[sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY)] ;
		memset(errMsg, 0, sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY));
		TS_ZMQ_SERVER_MSG* errStru = (TS_ZMQ_SERVER_MSG*)errMsg;
		memcpy(errStru->deviceId, zmqMsg->deviceId, sizeof(errStru->deviceId));
		errStru->msgCode  = TS_SERVER_MSG_BODY_ERR;
		TS_SERVER_ERROR_BODY* errBody = (TS_SERVER_ERROR_BODY*)errStru->customMsgBody;
		errBody->errMsgCode = zmqMsg->msgCode;
		memcpy(errBody->errData, "Body Len too small", 19);

		Data_Ptr tmpData(errMsg);
		send_msg = tmpData;
		send_msg_len = sizeof(TS_ZMQ_SERVER_MSG) + sizeof(TS_SERVER_ERROR_BODY);
		
		return LWDP_OK;
	}

	uint32_ retResult    = TS_SERVER_OK;
	char_*  retResultStr = "Check OK!";
	uint32_ retSize      = 0;
	RET_LIST errList;
	std::list<pthread_t>::iterator iter;

	uint32_ thread_num = gThreadCount;
	uint32_ domainSize = 0;
	uint32_ inum = msgBody->cardDataCount;
	domainSize = inum / thread_num;
	thread_num += (inum % thread_num)>0?1:0;
		
	LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::NOTICE, 
	           		"TotleNum: %d thread_num: %d domainSize: %d", 
	           		inum, thread_num, domainSize);

	std::list<pthread_t> thrList;
	GET_OBJECT_RET(DbMgr, iDbMgr, NULL);
	uint32_ nowSize = inum;
	for(uint32_ num = 0; num < thread_num; ++num)
	{
		pthread_t t;
		int result = 0;

		DBHandle workDbHandle = iDbMgr->OpenCopy(db_handle);
		if(!workDbHandle)
		{
			LWDP_LOG_PRINT("ACDEVICE", LWDP_LOG_MGR::ERR, 
			           	   "ACD Thread Connect Mysql Error");
			uint32_ retResult    = TS_SERVER_DB_ERR;
			char_*  retResultStr = "Connect DB Error";
			goto RET_TAG;
		}
	
		WORKTHREAD_PARAM* domain = (WORKTHREAD_PARAM*)malloc(sizeof(WORKTHREAD_PARAM));
		domain->startPos 	= (num * domainSize);
		domain->length 		= domainSize;
		if(nowSize < domainSize)
		{
			domain->length = nowSize;
		}	
		nowSize -= domainSize;
		domain->dbHandle 	= workDbHandle;
		domain->msg 		= zmqMsg;
		domain->object 		= this;
		result = pthread_create(&t, NULL, work_thread, domain);
		if(result != 0){
			LWDP_LOG_PRINT("CT", LWDP_LOG_MGR::ERR, 
						   "Can't Create Thread Ret: %d\n", result);

			retResult = TS_SERVER_BULK_THREAD_ERR;
			retResultStr = "Bulk Can't Create Thread";
			goto RET_TAG;
		}

		thrList.push_back(t);			
	}

	FOREACH_STL(iter, thrList)
	{
		RET_LIST* work_ret_list;
		int rc = pthread_join(*iter, (void**)&work_ret_list);
		ASSERT_CHECK(rc == 0);
		if(work_ret_list)
		{
			RET_LIST::iterator erriter;
			FOREACH_STL(erriter, (*work_ret_list))
			{
				errList.push_back(*erriter);
			}
	
			DELETE_SINGLE(work_ret_list);
		}
	}

RET_TAG:
	TS_DEVICE_CARD_DATA_REQ_BODY* tmpBody = NULL;
	tmpBody = (TS_DEVICE_CARD_DATA_REQ_BODY*)msgBody->cardDataEntry;

	if(!errList.empty())
	{
		retSize = sizeof(TS_ZMQ_SERVER_MSG) + 
			      sizeof(TS_DEVICE_BULK_DATA_RSP_BODY) +
			      sizeof(tmpBody->cardId) * errList.size();
	}
	else
	{
		retSize = sizeof(TS_ZMQ_SERVER_MSG) + 
				  sizeof(TS_DEVICE_BULK_DATA_RSP_BODY);
	}
	
	uint8_* returnMsg = NULL;
	returnMsg = new uint8_[retSize];
	memset(returnMsg, 0, retSize);
	ASSERT_CHECK_RET(LWDP_PLUGIN_LOG, LWDP_MALLOC_MEMORY_ERROR, returnMsg, "Malloc Error");

	TS_ZMQ_SERVER_MSG* retStru = (TS_ZMQ_SERVER_MSG*)returnMsg;
	memcpy(retStru->deviceId, zmqMsg->deviceId, sizeof(retStru->deviceId));
	retStru->msgCode  = TS_SERVER_BULK_DATA_RSP_MSG; //
	TS_DEVICE_BULK_DATA_RSP_BODY* retBody = (TS_DEVICE_BULK_DATA_RSP_BODY*)retStru->customMsgBody;
	retBody->msgResult = retResult;
	memcpy(retBody->msgResultData, retResultStr, strlen(retResultStr));

	if(!errList.empty())
	{
		retBody->msgResult = TS_SERVER_BULK_WRITE_DATA_ERROR;
		memcpy(retBody->msgResultData, "Have Error!", 12);

		retBody->errorEntryNum = errList.size();
		for(uint32_ i = 0; i < retBody->errorEntryNum; ++i)
		{
			memcpy(retBody->errCardId + (sizeof(tmpBody->cardId) * i), errList[i].data(), sizeof(tmpBody->cardId));
		}
	}

	Data_Ptr tmpData(returnMsg);
	send_msg     = tmpData;
	send_msg_len = retSize;
	
	return LWDP_OK;
}

LWRESULT Cx_ACDevice::IntArrayToStr(uint32_ int_array[], uint32_ size, std::string& ret_str)
{
	std::string formatStr("");
	char_* tmpStrCell = new char_[32];
	for(uint32_ i = 0; i < size; ++i)
	{
		memset(tmpStrCell, 0, 32);
		Api_snprintf(tmpStrCell, 32, "%d", int_array[i]);
		if(!int_array[i])
			break;
		formatStr += std::string(tmpStrCell);
	}

	ret_str = formatStr;
	DELETE_MULTIPLE(tmpStrCell);
	return LWDP_OK;
}



