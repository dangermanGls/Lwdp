/*! \file   ExternalInterface.h
 *  \brief  Interface
 *  \author Guolisen, LwDp
 *  \date   2012.12.14
 */
 
#ifndef __EXTERNAL_INTERFACE_H
#define __EXTERNAL_INTERFACE_H


LWDP_NAMESPACE_BEGIN;
////////////////////////////////////////////
// Zmq Msg Body
////////////////////////////////////////////
typedef struct stru_zmq_server_msg
{
	uint32_ deviceId;
	uint32_ msgCode;
	uint8_  customMsgBody[0];  //��Ϣ��
}TS_ZMQ_SERVER_MSG;


////////////////////////////////////////////
// Tcp Msg Body
////////////////////////////////////////////
enum TCP_SERVER_RETURN_CODE
{
	TS_SERVER_TCP_MSG_LEN_ERROR = 1,  //�ṹ������Ϣ�����ֶ���ֵ����ʵ�ʽ������ݳ���
};

typedef struct stru_tcp_server_msg
{
	uint32_ msgLength;  //��Ϣ�峤��  �˳���Ϊ������Ϣ���ȣ�����msgLength����ĳ���
	uint32_ returnCode; //��ϢTcpServer������
	uint8_  zmqMsgBody[0];  //��Ϣ��
}TS_TCP_SERVER_MSG;

////////////////////////////////////////////
// SERVER MSG CODE
////////////////////////////////////////////
#define TS_SYSTEM_MSG_BASE    0x00000000 //Module:0x00 Code0x000000
enum TS_SYSTEM_MSG_ENUM
{
	TS_SERVER_OK = TS_SYSTEM_MSG_BASE,
	TS_SERVER_UNKNOW_MSG, //�ͻ��˷��͵���ϢΪδ֪����
	TS_SERVER_MSG_BODY_ERR, //�ͻ��˷��͵���ϢΪδ֪����
};

typedef struct stru_server_err_body
{	
	uint32_  errMsgCode; //��Ϣ���ͽ��
	char_    errData[128];  // ���ܵĴ�����Ϣ�ַ���
}TS_SERVER_ERROR_BODY;


////////////////////////////////////////////
// Client Msg Code
////////////////////////////////////////////

#define TS_GATECHECK_MSG_BASE 0x01000000 //Module:0x01 Code0x000000

enum TS_GATECHECK_MSG_ENUM
{
	//ACDevice
	TS_DEVICE_INIT_REQ_MSG = TS_GATECHECK_MSG_BASE,
	TS_SERVER_INIT_RSP_MSG,
	TS_DEVICE_CONFIG_REQ_MSG,
	TS_SERVER_CONFIG_RSP_MSG,
	TS_DEVICE_HEART_BEAT_REQ_MSG,
	TS_SERVER_HEART_BEAT_RSP_MSG,
	TS_DEVICE_CARD_DATA_REQ_MSG,
	TS_SERVER_CARD_DATA_RSP_MSG,
	TS_DEVICE_BULK_DATA_REQ_MSG,
	TS_SERVER_BULK_DATA_RSP_MSG,
};



////////////////////////////////////////////
// �Ž�������ʼ����Ϣ
////////////////////////////////////////////
enum TS_INIT_MSG_RESAULT_ENUM
{	
	TS_SERVER_CHECK_OK_RECONFIG = 1, //��֤�ɹ��������������
	TS_SERVER_ID_ERROR,     //�豸IDδ֪����
	TS_SERVER_TYPE_ERROR,   //�豸���ʹ���
	TS_SERVER_UNKNOW_ERROR //δ֪���󣬾����������Ϣ
};
typedef struct stru_device_init_req_body
{	
	uint32_ deviceType;  //�豸����
	uint8_  sceneryId[8];  //����ID
	uint32_ checkResult; //�豸�Լ���
}TS_DEVICE_INIT_REQ_BODY;

typedef struct stru_server_init_rsp_body
{	
	uint32_  msgResult; //��Ϣ���ͽ��
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ���
	uint32_  appendDataLength; //�������ݳ���
	uint8_*  appendData;
}TS_SERVER_INIT_RSP_BODY;


////////////////////////////////////////////
// �豸����������Ϣ
////////////////////////////////////////////
enum TS_SERVER_CONFIG_MSG_RESAULT_ENUM
{	
	TS_SERVER_CONFIG_ERROR = 1, //ʧ��
};

typedef struct stru_dev_config_body
{	
	uint32_  msgResult; //��Ϣ���ͽ��
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ���
	uint32_  deviceId;  //�豸����
	uint32_  deviceType;  //�豸����
	uint8_   sceneryId[8];  //����ID
	uint8_   sceneryDomainId[8];  //����ID
	uint32_  sceneryPostion;  //ƫ��
}TS_DEV_CONFIG_RSP_BODY;


////////////////////////////////////////////
// ״̬�����Ϣ
////////////////////////////////////////////
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
	uint32_  msgResult; //��Ϣ���ͽ��
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ���
	uint32_  oprationCode;  //������
	uint32_  appendDataLength; //�������ݳ���
	uint8_   appendData[0];
}TS_DEV_STATUS_RSP_BODY;

////////////////////////////////////////////
// ˢ��������Ϣ
////////////////////////////////////////////
enum TS_CARD_DATA_MSG_RESAULT_ENUM
{	
	TS_SERVER_WRITE_DATA_ERROR = 1, //д���ݿ�ʧ��
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
	uint32_  msgResult; //��Ϣ���ͽ��
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ���
}TS_DEVICE_CARD_DATA_RSP_BODY;



////////////////////////////////////////////
// ˢ�����������ϱ���Ϣ
////////////////////////////////////////////
enum TS_BULK_DATA_MSG_RESAULT_ENUM
{	
	TS_SERVER_BULK_WRITE_DATA_ERROR = 1, //д���ݿ�ʧ��
	TS_SERVER_BULK_UNKONW_ERROR,     //�豸IDδ֪����
};

typedef struct stru_dev_bulk_data_msg
{  
	uint32_  cardDataCount;   //��������Ŀ����
	uint8_   cardDataEntry[0];   //��ΪID
}TS_DEVICE_BULK_DATA_REQ_BODY;

typedef struct stru_dev_bulk_data_rsp_msg
{	
	uint32_  msgResult; //��Ϣ���ͽ��
	char_    msgResultData[32];  // ���ܵĴ�����Ϣ�ַ���
	uint32_  errorEntryNum;
	uint8_   errCardId[0]; //errorEntryNum * 8
} TS_DEVICE_BULK_DATA_RSP_BODY;

LWDP_NAMESPACE_END;

#endif











