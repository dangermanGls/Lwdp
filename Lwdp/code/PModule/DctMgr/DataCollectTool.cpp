/*==============================================================================
Copyright (c) 2003, CPIT. All Rights Reserved.

FILENAME:   DataCollectTool.cpp

DESCRIPTION:
  This module implements data collect function.
  The object is initialized only once.

HISTORY:

Date           CR No      Person        Description
----------  ------------  ------       -------------
03 10 23          1        liulj        Initial Version
04-7-8            2        WangLi       Verify Version
==============================================================================*/

//==============================================================================
//                    Includes and Variable Definitions
//==============================================================================

//==============================================================================
// Include Files 
//==============================================================================
#ifndef WIN32
#include <intLib.h>
#include "logLib.h"
#endif

#include <LwDp.h>
//#include "phyDrv.h"

#include "DataCollectTool.h"

//------------------------------------------------------------------------------
// Constant / Defines
//------------------------------------------------------------------------------
const int MML_DATABUF_NUM = 1024;           /*���ݲɼ��������ڵ����*/
const int MML_DATABUF_SIZE = 512;           /*���ݲɼ��������ڵ��С*/
#if defined(SW_CPB)||defined(MCU_SW)
const int MML_BBDDATABUF_NUM = 200;         /*Ϊ����ר�����õ����ݲɼ��������ڵ����*/
const int MML_BBDDATABUF_SIZE = 4096;       /*Ϊ����ר�����õĻ������ݲɼ��������ڵ��С*/
const int MML_BBDDATABUF_LEN = (MML_BBDDATABUF_SIZE+2)*MML_BBDDATABUF_NUM;
#endif

const int MML_ID_SIZE = 1;          /*ͳ�Ʋ�������С*/
const int MML_PARAMETER_VALUE = 4;          /*ͳ�Ʋ���ֵ��С*/
const int MML_TIME = 8;                     /*ʱ�����С*/
const int MML_STATBUF_NUM = 2048;           /*ͳ�����ݻ������ڵ����*/
const int MML_STATBUF_SIZE = MML_ID_SIZE+MML_PARAMETER_VALUE+MML_TIME;  /*ͳ�����ݻ������ڵ��С*/

const int MML_MSGBUF_NUM = 60;              /*���ݲɼ��������ڵ����*/
const int MML_MSGBUF_SIZE = sizeof(MmlMsg);            /*���ݲɼ��������ڵ��С*/

const int MML_CTRL_PIPE_NUM = 32;           /*����pipe�������Ϣ��*/    
const int MML_CTRL_PIPE_SIZE = sizeof(MmlCtrlOrd);/*����pipe��Ϣ����󳤶�*/

const int MML_WATCH_PIPE_NUM = 1024;         /*�û�pipe�������Ϣ��*/
const int MML_WATCH_PIPE_SIZE = 4;          /*�û�pipe��Ϣ����󳤶�*/

const int MML_STAT_PIPE_NUM = 512;          /*ͳ���û�pipe�������Ϣ��*/
const int MML_STAT_PIPE_SIZE = 4;           /*ͳ���û�pipe��Ϣ����󳤶�*/

#ifndef  PRIORITY_MML_LSNTSK
#define   MML_LSNTSK_NAME       "ListenTask"
const int PRIORITY_MML_LSNTSK = 80;         /*TCP������������ȼ�*/ 
const int STACKSIZE_MML_LSNTSK = 0x2000;    /*TCP��������ջ�Ĵ�С*/

#define   MML_TSK_NAME          "DataCollectTask"
const int PRIORITY_MML_TSK = 240;           /*���ݲɼ���������ȼ�*/    
const int STACKSIZE_MML_TSK = 0x10000;       /*���ݲɼ�����ջ�Ĵ�С*/   
#endif

const int KEY_DELETE = 127;                 /*ɾ����*/          
const int KEY_BACKSPACE = 8;                /*�˸��*/      
const int KEY_RETURN = 13;                  /*�س���*/
            
const int DELETE_CHAR = -10;            
const int RETURN_CHAR = 1;  
const int NORMAL_CHAR = 0;  
const int UNKOWN_CHAR = 10;                 /*���з�*/

const int WINID_MML = 17;                   /*֧��MML��������Ŀ��ƴ��ں�*/
/*MML����ĳ���=�ĸ����ţ�:--:��+ ����MML����������+������������+����ֵ���ȣ�*��������+��������*2(,=)+ 1('/0')*/
/*MML_CMD_LEN = 4+ 3*16 +(16+64)*8 +8*2 + 1=709*/
const int MML_CMD_LEN = 4+3*MML_NAME_LEN + (MML_NAME_LEN+MML_VALUE_LEN)*MML_PARA_NUM + MML_PARA_NUM*2+1;                /*MML�����*/

const int MML_NOVER = 5;  /*ָʾ�Ƿ������һ������*/
const int MML_STOP =6;   /*�Է��Ͽ�����*/

#ifndef BASE_PORT
#define BASE_PORT 8800
#endif

#define AGAIN_SEND_TIME 10

#define MML_PORT    (BASE_PORT+1)                       /*����MML�����socket�˿ں�*/
#define DBG_PORT    (BASE_PORT+2)                      /*��ӡ�û��ɼ����ݵ�socket�˿ں�*/
#define STAT_PORT   (BASE_PORT+3)                     /*��ӡͳ����Ϣ���ݵ�socket�οں�*/


//==============================================================================
// Type Declarations
//==============================================================================

//==============================================================================
// Global Constant Definitions 
//==============================================================================

//==============================================================================
// Local Object Definitions
//==============================================================================

int indication[MAX_MML_MMLNUM];

int currentpos[MAX_MML_MMLNUM];



//==============================================================================
// Static Variable Definitions  
//==============================================================================
static char gMmlDataBuf[MAX_MML_MMLNUM][MML_CMD_LEN];
static int g_CaptureDataFlag = 1;  /*1 Ĭ������ģʽ  0 �����ж���*/
char MmlPrintfdatabuf[2048];



winptr      DataCollectTool::WinPtr[MAX_WINPTR_NUM];
statptr     DataCollectTool::StatPtr[MAX_STATPTR_NUM];
MemoryBuf   DataCollectTool::mDataBuf;
#if defined(SW_CPB)||defined(MCU_SW)
MemoryBuf   DataCollectTool::mBBDDataBuf;
#endif
MemoryBuf   DataCollectTool::mMsgBuf;
MemoryBuf   DataCollectTool::mStatBuf;
MmlCallBack DataCollectTool::mMmlCallBack[MAX_MML_USER];
MmlCmdList*  DataCollectTool::mMmlCmdPtr = NULL;

MmlDosCmd   DataCollectTool::mDosCmd[16] =
{
    {"SHOW_DEBUGWIN", (FUNCPTR)CmdShowDebugWin},
    {"CONFIG_WIN", (FUNCPTR)CmdCfgWin},
    {"RESUME_WIN", (FUNCPTR)CmdResumeWin},
    {"PAUSE_WIN", (FUNCPTR)CmdPauseWin},
    {"TIMER_WIN", (FUNCPTR)CmdTimerWin},
    {"REBOOT", (FUNCPTR)CmdReboot},
    {"HELP", (FUNCPTR)CmdHelp},
    {"QUIT", (FUNCPTR)CmdQuit}, 
    {"PRINTDATABUF", (FUNCPTR)PrintDataBuf},
    {"DELETE_CONNECT", (FUNCPTR)deleteconnection},
    {"CAPTURE_DATA", (FUNCPTR)CaptureData},
    {NULL, NULL}
};
MmlWinCfg   DataCollectTool::mMmlCtrl[MAX_MML_MMLNUM];
MmlWinCfg   DataCollectTool::mMmlStat[MAX_MML_STATNUM];
MmlWinCfg   DataCollectTool::mMmlCnfg[MAX_MML_WNDNUM];
int         DataCollectTool::mWndConnNum = 0;
int         DataCollectTool::mMmlConnNum = 0;
int         DataCollectTool::mStatConnNum = 0;
int         DataCollectTool::mInitialized = 0;
int         DataCollectTool::activeId = -1;
int         DataCollectTool::ctrlPipeId;
int         DataCollectTool::mDataCollectTaskId = 0;
int         DataCollectTool::mListenTaskId = 0;
int         DataCollectTool::tty_sockid = 0;
int         DataCollectTool::watch_sockid = 0;
int         DataCollectTool::stat_sockid = 0;
SEM_ID      DataCollectTool::semIdForDataBuf= NULL;
//==============================================================================
// Forward Declarations
//==============================================================================

//==============================================================================
//                              Function Definitions
//==============================================================================

/*==============================================================================
Function:   DataCollectTool::DataCollectTool

Description:
 This function is the constructor function 

Input Value:
    None  
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
DataCollectTool::DataCollectTool()
{ 
    mInitialized = 0;
}
/*==============================================================================
Function:   DataCollectTool::~DataCollectTool

Description:
 This function is the destructor function 

Input Value:
    None  
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/

DataCollectTool::~DataCollectTool()
{   
    MmlExit();  
}

/*==============================================================================
Function:   DataCollectTool::MmlInit

Description:
 This function initializes data collect module

Input Value:
    None 
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlInit()
{
    char pipeName[20];
    int i;

    if( 1 == mInitialized )
    {
        return ERROR;
    }
       /*���ݲɼ�������*/
    memset(&mDataBuf, 0, sizeof(mDataBuf));
#if defined(SW_CPB)||defined(MCU_SW)
    /*Ϊ����ר�����õ����ݲɼ�������*/
    memset(&mBBDDataBuf, 0, sizeof(mBBDDataBuf));
#endif
    /*MML��Ϣ������*/
    memset(&mMsgBuf, 0, sizeof(mMsgBuf));
    /*MML����ص�����ע���*/
    memset(mMmlCallBack, 0, sizeof(mMmlCallBack));
    /*MML���ƴ�������*/
    memset(mMmlCtrl, 0, sizeof(mMmlCtrl));
    /*MML���Ӵ������ñ�*/
    memset(mMmlCnfg, 0, sizeof(mMmlCnfg));
    /*����ͳ�ƴ������ñ�*/
    memset(mMmlStat, 0, sizeof(mMmlStat));
    memset(WinPtr,0xff,sizeof(WinPtr));
    memset(StatPtr,0x00,sizeof(StatPtr));
    
    mMmlConnNum = 0;
    mWndConnNum = 0;
    mStatConnNum = 0;

#ifdef WIN32
    WSADATA stWS;
    WSAStartup(MAKEWORD(2,0),&stWS);
#endif

    for(i = 0; i < MAX_MML_MMLNUM; i++)
    {
        indication[i] = 0;
        currentpos[i] = 0;
    }
    for(i = 0; i < MAX_WINPTR_NUM; i++)
        StatPtr[i].winNum = -1;
    /*�������ݲɼ�������*/
    if(ERROR == InitDataBuf(MML_DATABUF_SIZE, MML_DATABUF_NUM, mDataBuf))
    {
        return ERROR;
    }
#if defined(SW_CPB)||defined(MCU_SW)
    /*����Ϊ����ר�����õ����ݲɼ�������*/
    if(ERROR == InitDataBuf(MML_BBDDATABUF_SIZE, MML_BBDDATABUF_NUM, mBBDDataBuf))
    {
        return ERROR;
    }
#endif
    /*����MML��Ϣ������*/
    if(ERROR == InitDataBuf(MML_MSGBUF_SIZE, MML_MSGBUF_NUM, mMsgBuf))
    {
        MmlExit();
        return ERROR;
    }
    /*����MMLͳ�����ݲɼ�������*/
    if(ERROR == InitDataBuf(MML_STATBUF_SIZE, MML_STATBUF_NUM, mStatBuf))
    {
        return ERROR;
    }
    /*MML����ע���*/
    mMmlCmdPtr = (MmlCmdList*)malloc(sizeof(MmlCmdList));
    if(NULL == mMmlCmdPtr)
    {
        MmlExit();
        return ERROR;
    }
    memset(mMmlCmdPtr, 0, sizeof(MmlCmdList));
    for(i = 0; i < 512; i++)
        mMmlCmdPtr->CmdItem[i].RegisterNum = -1;
    /*��������pipe,���ڽ��տ�������*/
    memset(pipeName, 0, 20);
    sprintf(pipeName, "/mmlPipe/Ctrl");
    if(ERROR == pipeDevCreate(pipeName, MML_CTRL_PIPE_NUM, 
        MML_CTRL_PIPE_SIZE))
    {
        MmlExit();
        return ERROR;
    }
    if((ctrlPipeId = opens(pipeName, O_RDWR, 0)) <= 0)
    {
        MmlExit();
        return ERROR;
    }
 /*   strcpy(mMmlCtrl.Prompt, "NETMML");*/

    /*����ͳ�����ݻ���pipe*/
    for(i = 0; i < MAX_MML_STATNUM; i++)
    {
        memset(pipeName, 0, 20);
        sprintf(pipeName, "/mmlPipe/Stat%d",i);
        if(ERROR == pipeDevCreate(pipeName, MML_STAT_PIPE_NUM,
            MML_STAT_PIPE_SIZE))
        {
            MmlExit();
            return ERROR;
        }
        if((mMmlStat[i].PipeId = opens(pipeName, O_RDWR, 0)) <=0)
        {
            MmlExit();
            return ERROR;
        }
        strcpy(mMmlStat[i].Prompt, "STATISTIC");
        mMmlStat[i].WinId = -1;
        mMmlStat[i].Status = IDLE;
    //    mMmlStat[i].connect = 0;
    }


    /*�������еĻ���pipe�Ͷ�ʱ��*/
    for(i = 0; i < MAX_MML_WNDNUM; i++)
    {
        memset(pipeName, 0, 20);
        sprintf(pipeName, "/mmlPipe/Watch%d", i);
        if(ERROR == pipeDevCreate(pipeName, MML_WATCH_PIPE_NUM, 
            MML_WATCH_PIPE_SIZE))
        {
            MmlExit();
            return ERROR;
        }
        if((mMmlCnfg[i].PipeId = opens(pipeName, O_RDWR, 0)) <= 0)
        {
            MmlExit();
            return ERROR;
        }
        if(NULL == (mMmlCnfg[i].WdTimer = wdCreate()))
        {
            MmlExit();
            return ERROR;
        }
        mMmlCnfg[i].WinId = -1;
        mMmlCnfg[i].Status = IDLE;
        mMmlCnfg[i].kit_telnet_connect = 0;
       
        strcpy(mMmlCnfg[i].Prompt, "DEBUG");
    }
    /*added by suyu for replacing taskLock 2006-12-6 begin*/
    semIdForDataBuf = semMCreate(SEM_Q_PRIORITY |SEM_DELETE_SAFE |SEM_INVERSION_SAFE );
    if(NULL == semIdForDataBuf)
    {
        printf("DataCollectTool::MmlInit :semMCreate semIdForDataBuf fail!\r\n");
        MmlExit();
        return ERROR;
    }
    /*added by suyu for replacing taskLock 2006-12-6 end*/
    /*�������ݲɼ�����*/
    if( ERROR == (mDataCollectTaskId = taskSpawn(MML_TSK_NAME, PRIORITY_MML_TSK, 0, 
        STACKSIZE_MML_TSK, (FUNCPTR)DataCollectTask, 
        (int)this, 0, 0, 0, 0, 0, 0, 0, 0, 0)))
    {
        MmlExit();
        return ERROR;
    }
    /*����TCP��������*/
    if( ERROR == (mListenTaskId = taskSpawn(MML_LSNTSK_NAME, PRIORITY_MML_LSNTSK, 0, 
        STACKSIZE_MML_LSNTSK, (FUNCPTR)ListenTask, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0)))
    {
        MmlExit();
        return ERROR;
    }
    mInitialized = 1;
//    printf("mml module has been initialized successfully\n");
    return OK;
}

/*==============================================================================
Function:   DataCollectTool::MmlRegister

Description:
 This function registers user Mml command call back function 

Input Value:
    objPtr : void*      ----  pointer to object of class
    cbFunPtr : FUNCPTR  ----  pointer to call back function     
    None
Return Value:
    int          register number
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlRegister(void* objPtr, FUNCPTR cbFunPtr)
{
    if( 0 == mInitialized )
    {
        printf("MmlRegister error! has not initialized\r\n");
        return ERROR;
    }
    if(NULL == cbFunPtr)
    {
        printf("MmlRegister error! MML callback function is NULL\r\n");
        return ERROR;
    }
    for(int i = 0; i < MAX_MML_USER; i++)
    {
        if(NULL == mMmlCallBack[i].FnPtr)
        {
            mMmlCallBack[i].FnPtr = cbFunPtr;
            mMmlCallBack[i].ObjPtr = objPtr;
            mMmlCallBack[i].MsgBufNum = 0;
            return i;
        }
    }
    printf("MmlRegister error! MML register list is FULL\r\n");
    return ERROR;
}
/*==============================================================================
Function:   DataCollectTool::MmlUnRegister

Description:
 This function cancels user Mml command call back function 

Input Value:
    num : int   ---- register number
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlUnRegister(int num)
{
    if( 0 == mInitialized )
    {
        printf("MmlUnRegister error! has not initialized\r\n");
        return ERROR;
    }

    if((num < 0)||(num >= MAX_MML_USER))
    {
        printf("MmlUnRegister error! register num %d is error\r\n",num);
        return ERROR;
    }
    mMmlCallBack[num].FnPtr = NULL;
    mMmlCallBack[num].ObjPtr = NULL;
    mMmlCallBack[num].MsgBufNum = 0;
    for(int i = 0; i < MML_TOTAL_CMD_NUM; i++)
    {
        /*�����ص�MML����ִ�к���ע���*/
        if(mMmlCmdPtr->CmdItem[i].RegisterNum == num)
        {
            memset(&mMmlCmdPtr->CmdItem[i], 0, sizeof(MmlCmdItem));
            mMmlCmdPtr->TotalNum -= 1;            
        }
    }
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::MmlInstallCmd

Description:
 This function installs user Mml command function 

Input Value:
    cmdName : char*         ---- command name
    fieldName : char*       ---- subcommand name 1
    subFieldName : char*    ---- subcommand name 2
    function : FUNCPTR      ---- execution function
    registerNum : int       ---- register number of call back fuction
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlInstallCmd(char* cmdName, char* fieldName, 
                                      char* subFieldName, FUNCPTR funPtr, 
                                      void* ObjPtr,
                                      int registerNum)
{
    int i, len;
    int cmd_num = 0;
    int flag = 0;
    int emptyIndex = -1;
    char cmd_name[MML_NAME_LEN];
    char field_name[MML_NAME_LEN];
    char subfield_name[MML_NAME_LEN];

    if( 0 == mInitialized )
    {
        printf("MmlInstallCmd error! has not initialized\r\n");
        return ERROR;
    }
    if((registerNum < 0)||(registerNum >= MAX_MML_USER))
    {
        /*����MmlInstallCmd��������MML���ڽ�����ע�ᣬ������printf��ӡ������Ϣ*/
        printf("MmlInstallCmd error! %s %s %s register number %d error\r\n",
            cmdName,fieldName,subFieldName,registerNum);        
        return ERROR;
    }
    if(NULL == mMmlCallBack[registerNum].FnPtr)
    {
        printf("MmlInstallCmd error! register Number %d callback function is NULL\r\n",registerNum);        
        return ERROR;
    }
    if(mMmlCmdPtr->TotalNum >= MML_TOTAL_CMD_NUM)
        return ERROR;   
    if(NULL == cmdName)
    {
        printf("MmlInstallCmd error!  register name is NULL\r\n");
        return ERROR;
    }
    if(NULL == funPtr)
    {
        printf("MmlInstallCmd error!  register execution function is NULL\r\n");
        return ERROR;
    }
    if('\0' == cmdName[0])
        return ERROR;
    memset(cmd_name, 0, MML_NAME_LEN);
    memset(field_name, 0, MML_NAME_LEN);
    memset(subfield_name, 0, MML_NAME_LEN);

    len = strlen(cmdName);
    if(len > MML_NAME_LEN)
    {
        printf("MmlInstallCmd error! %s command name len exceed 16\r\n",cmdName);   
        return ERROR;
    }
    memcpy(cmd_name, cmdName, len);
    MyStrUpper((char*)cmd_name);  
    cmd_num ++;
    if(NULL == fieldName)
    {        
        flag = 1;
    }
    else
    {
        len = strlen(fieldName);
        if(len > MML_NAME_LEN)
        {
            printf("MmlInstallCmd error! %s command name len exceed MML_NAME_LEN\r\n",fieldName);              
            return ERROR;
        }
        memcpy(field_name, fieldName, len);
        MyStrUpper((char*)field_name);
        cmd_num ++;
    }
    if( NULL != subFieldName)  
    {
        if( 1 == flag )
        {       
            return ERROR;
        }
        len = strlen(subFieldName);
        if(len > MML_NAME_LEN)
        {
            printf("MmlInstallCmd error! %s command name len exceed MML_NAME_LEN\r\n",subFieldName);               
            return ERROR;
        }
        memcpy(subfield_name, subFieldName, len);
        MyStrUpper((char*)subfield_name);
        cmd_num ++;
    }

    flag = 0;
    /*�����ж�ע����Ƿ��п���*/
    for(i=0; i < MML_TOTAL_CMD_NUM; i++)
    {
        if( NULL != mMmlCmdPtr->CmdItem[i].FnPtr)
        {
            if(0 == strcmp(cmd_name,mMmlCmdPtr->CmdItem[i].CmdName[0].Cmd))
            {
                if(0 != strlen(field_name))
                {
                    if(0 == strcmp(field_name,mMmlCmdPtr->CmdItem[i].CmdName[1].Cmd))
                    {
                        if( 0 != strlen(subfield_name))
                        {
                            if(0 == strcmp(subfield_name,mMmlCmdPtr->CmdItem[i].CmdName[2].Cmd))
                            {
                                printf("MmlInstallCmd error! command name %s-%s-%s already exist\r\n",
                                    cmdName,fieldName,subFieldName);
                                return ERROR;
                            }
                        }
                        else
                        {
                            if(0 == strlen(mMmlCmdPtr->CmdItem[i].CmdName[2].Cmd))
                            {
                                printf("MmlInstallCmd error! command name %s-%s already exist\r\n",cmdName,fieldName);                          
                                return ERROR;
                            }
                        }                   
                    }
                }
                else
                {
                    if(0 == strlen(mMmlCmdPtr->CmdItem[i].CmdName[1].Cmd))
                    {
                        printf("MmlInstallCmd error! command name :%s: already exist\r\n",cmdName);                 
                        return ERROR;
                    }
                }           
            }
        }   
        else
        {
            if(0 == flag)
            {
                emptyIndex = i;
                flag = 1;
            }
        }
    }

    mMmlCmdPtr->CmdItem[emptyIndex].FnPtr = funPtr;   
    mMmlCmdPtr->CmdItem[emptyIndex].ObjPtr = ObjPtr;
    mMmlCmdPtr->CmdItem[emptyIndex].RegisterNum = registerNum;
    mMmlCmdPtr->TotalNum += 1;  
   
    memcpy(mMmlCmdPtr->CmdItem[emptyIndex].CmdName[0].Cmd, cmd_name, strlen(cmd_name));
    
    if(cmd_num > 1)
    {
        
        memcpy(mMmlCmdPtr->CmdItem[emptyIndex].CmdName[1].Cmd, field_name, strlen(field_name));
    }
    else
    {
        mMmlCmdPtr->CmdItem[emptyIndex].CmdName[1].Cmd[0] = '\0';
        mMmlCmdPtr->CmdItem[emptyIndex].CmdName[2].Cmd[0] = '\0';
        return OK;
    }
    if(cmd_num > 2)
    {
        
        memcpy(mMmlCmdPtr->CmdItem[emptyIndex].CmdName[2].Cmd, subfield_name, strlen(subfield_name));
    }
    else
        mMmlCmdPtr->CmdItem[emptyIndex].CmdName[2].Cmd[0] = '\0';   

    return OK;
    
}

/*==============================================================================
Function:   DataCollectTool::ProcessMMLMsg

Description:
 This function processes Mml command message for upper user

Input Value:
    msgAddr : int   ---- address of pointer to Mml message structure
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::ProcessMMLMsg(int msgAddr)
{
    MmlMsg* msg_ptr;
    int index, num;

    if( 0 == mInitialized )
    {
        return ERROR;
    }

    if(0 == msgAddr)
        return ERROR;

    msg_ptr = (MmlMsg*)msgAddr;
    index = msg_ptr->CmdIndex;
    activeId = msg_ptr->indexId;
    if((index < 0) || index >= MML_TOTAL_CMD_NUM)
    {
        return ERROR;
    }
    if(NULL == mMmlCmdPtr->CmdItem[index].FnPtr)
    {
        SuperMmlPrintf(activeId,"MML execution function was not installed\r\n");
        return ERROR;
    }
    
    num = mMmlCmdPtr->CmdItem[index].RegisterNum;
    /*����MML�����ִ�к���*/
    mMmlCmdPtr->CmdItem[index].FnPtr(mMmlCmdPtr->CmdItem[index].ObjPtr, msgAddr);
    PutDataBuf((char*)msg_ptr, mMsgBuf);
    mMmlCallBack[num].MsgBufNum--;
    activeId = -1;
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::MmlGetCmdPara

Description:
 This function processes Mml command message for upper user

Input Value:
    handle : int        ---- address of pointer to Mml message structure
    paraIndex : int     ----  parameter Number
    paraPtr[] : char*   ----  pointer to resolved name and value of parameter 

Output Value:
    None
Return Value:
    1        success
    0        end
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlGetCmdPara(int handle, int paraIndex, char* paraPtr[])
{
    MmlMsg* msg_ptr;
    msg_ptr = (MmlMsg*)handle;

    if(paraIndex < 0 || paraIndex >= msg_ptr->ParaNum)
    {
        return 0;
    }
    /*��ȡMML����Ĳ�����*/
    paraPtr[0] = msg_ptr->CmdPara[paraIndex].Para;
    /*��ȡMML����Ĳ���ֵ*/
    paraPtr[1] = msg_ptr->CmdPara[paraIndex].Value;
    return 1;
}



/*==============================================================================
Function:   DataCollectTool::MmlPrintf

Description:
 This function is the data collect interface function for upper App

Input Value:
    paraPtr : char*     ----  print content
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlPrintf(const char* paraPtr,...)
{
    unsigned short usParaLen = 0;
    char buf_mml_printf[512];
    va_list argptr;
    
    if( 0 == mInitialized )
    {
        return ERROR;
    }
    if((activeId < 0)||(activeId > MAX_MML_MMLNUM))
        return ERROR;    
    va_start(argptr, paraPtr);
    usParaLen = vsprintf(buf_mml_printf, paraPtr, argptr);
    va_end(argptr);
    /*���buf_mml_printf����򽫵�ǰ�������*/
    if( usParaLen >= 512 )
    {
        taskSuspend( taskIdSelf() );
        return ERROR;
    }
    
    if( ERROR == SendToTelnet(WINID_MML, buf_mml_printf))
    {
        return ERROR;
    }
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::SuperMmlPrintf

Description:
 This function is the data collect function for data logger, it's not outer interface

Input Value:
    mode:   int         ----  mode = 0: send data to MML 0 connection 
                              mode = 1: send data to MML 1 connection
                              mode = 2: send data to all MML connection
    paraPtr : char*     ----  print content
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::SuperMmlPrintf(int mode, const char* paraPtr, ...)
{
    unsigned short usParaLen = 0;
    char buf[512];
    va_list argptr;  
    int number;

    
    va_start(argptr, paraPtr);
    usParaLen = vsprintf(buf, paraPtr, argptr);
    va_end(argptr);
    /*���buf����򽫵�ǰ�������*/
    if( usParaLen >= 512 )
    {
        taskSuspend( taskIdSelf() );
        return ERROR;
    }

    if(MAX_MML_MMLNUM == mode)
    {
        for(number = 0; number < MAX_MML_MMLNUM; number++)
        {
            if(mMmlCtrl[number].SockId > 0)
            {   
                send(mMmlCtrl[number].SockId,buf,strlen(buf),0);            
                send(mMmlCtrl[number].SockId,mMmlCtrl[number].Prompt,strlen(mMmlCtrl[number].Prompt),0);            
            }
            else
                return ERROR;
        }
    }
    else
    {
        if(mMmlCtrl[mode].SockId > 0)
        {
            send(mMmlCtrl[mode].SockId,buf,strlen(buf),0);      
            send(mMmlCtrl[mode].SockId,mMmlCtrl[mode].Prompt,strlen(mMmlCtrl[mode].Prompt),0);
        }
        else
            return ERROR;
    }
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::MmlPrintf

Description:
 This function is the data collect interface function for upper App

Input Value:
    winId : int         ---- ID of debug windows 
    paraPtr : char*     ---- printf content 
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlPrintf(int winId, const char* paraPtr,...)
{
#ifdef LOCALDEBUG
    /*�����ص���ʱ��Ϊ�۲췽�㣬���ô�ӡprintf����ʽ*/
    char* buf_ptr = NULL;   
    va_list argptr;  
    
    va_start(argptr, paraPtr);
    vsprintf(MmlPrintfdatabuf, paraPtr, argptr);
    va_end(argptr);

    printf("%s",MmlPrintfdatabuf);

#else
    /*�����ݴ�ӡ�����Դ�����*/
    int pipe_id = 0, data_ptr, msg_num, len;
    int winNum;
    unsigned short usParaLen = 0;
    char* buf_ptr = NULL;    
    va_list argptr;

#ifndef WIN32   
    if(TRUE == intContext())
    {
        logMsg("don't allow call DataCollectTool::MmlPrintf in interrupt context!\r\n",0,0,0,0,0,0);
        return ERROR;
    }
#endif
    if( 0 == mInitialized )
        return ERROR;
    if((winId < 0)||(winId >= MAX_WINPTR_NUM))
        return ERROR;
    winNum=WinPtr[winId].winNum;
    if((winNum < 0)||(winNum >= MAX_MML_WNDNUM))
        return ERROR;   
    pipe_id = mMmlCnfg[winNum].PipeId;
    if(0 == pipe_id)
    {
        return ERROR;
    }
    /*���ڵ�pipe�Ѿ������,�����·��Ͳ���pend,�������ж��Ƿ��п���*/    
    ioctl(pipe_id, FIONMSGS, (int)&msg_num);
    if(MML_WATCH_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    /*ֻ��PAUSE��ACTIVE״̬���԰����ݴ洢�ڻ�����,�������Ӳ���������Ҫд���ݵ�pipe��*/
    if(ACTIVE == mMmlCnfg[winNum].Status
        || PAUSE == mMmlCnfg[winNum].Status)
    {
        /*���뻺�����ڴ�*/
        buf_ptr = GetDataBuf(mDataBuf);
        if( NULL == buf_ptr )
        {
#ifdef DATACOLLECT_DEBUG            
            SuperMmlPrintf(MAX_MML_MMLNUM, "the debug buffer is full\r\n");
#endif
            return ERROR;
        }
        memset(buf_ptr, 0x00, MML_DATABUF_SIZE+2);
        /*��ɲɼ����ݵĸ�ʽ��*/
        va_start(argptr, paraPtr);
        usParaLen = vsprintf(MmlPrintfdatabuf, paraPtr, argptr);
        va_end(argptr);
        /*���buf����򽫵�ǰ�������*/
        if( usParaLen >= 2048 )
        {
            taskSuspend( taskIdSelf() );
            return ERROR;
        }
        /*�ɼ�������Ϣ��󳤶�ΪMML_DATABUF_SIZE*/
        len = strlen(MmlPrintfdatabuf);
    
        /* ���ڿ�����telnet����Ҳ����ʱkit����,ÿ�η�����Ϣʱ��������Ϣ����,
         * ������Ϊ��ʾ������Ϣ,WinPtr[].config������ʾ�Ƿ��һ�η���,
         * ��ʾ��һ�η���,1��ʾ���ǵ�һ�η���.��װ��ʽ����:
         *  LEN  |  2  | 1 |       <=512-1     |
         *       |-----|---|-------------------|
         *  DATA |LEN+3| 0 |       DATA        |
         *       |-----|---|-------------------|
         */
        if(len > MML_DATABUF_SIZE-1)
        {           
            len = MML_DATABUF_SIZE-1;
        }
        *(char*)(buf_ptr+2)  = 0;      /*������������*/
        WinPtr[winId].config = 1;      /*�����Ƿ��һ�η��ͱ�־Ϊ1*/
        *(short*)buf_ptr     = len+3;  /*�������� ���� ����*/
        memcpy(buf_ptr+3, MmlPrintfdatabuf, len);

        /*���������ڴ��׵�ַ���͵��û�����pipe��*/
        data_ptr = (int)buf_ptr;
        if(MML_POINTER_SIZE != writes(pipe_id, (char*)&data_ptr, MML_POINTER_SIZE))
        {
            PutDataBuf(buf_ptr,mDataBuf);
            return ERROR;
        }
    }
#endif
    return OK;
}

/*==============================================================================
Function:   DataCollectTool::MmlLog

Description:
 This function is the data collect interface function for upper App

Input Value:
    winId : int     ---- ID of log windows
    buf : char*     ---- log content
    lenth: int      ---- length of log content
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlLog(int winId, const char* buf, int length)
{
    int pipe_id = 0, data_ptr, msg_num;
    int winNum;
    char* buf_ptr = NULL;
#ifndef WIN32   
    if(TRUE == intContext())
    {
        logMsg("don't allow call DataCollectTool::MmlLog in interrupt context!\r\n",0,0,0,0,0,0);
        return ERROR;
    }
#endif
    if( 0 == mInitialized )
    {
        return ERROR;
    }    
    if((winId < 0)||(winId >= MAX_WINPTR_NUM))
        return ERROR;
    winNum=WinPtr[winId].winNum;
    if((winNum < 0)||(winNum >= MAX_MML_WNDNUM))
        return ERROR;       
    pipe_id = mMmlCnfg[winNum].PipeId;
    if(0 == pipe_id)
    {
        return ERROR;
    }
    /*���ڵ�pipe�Ѿ������,�����·��Ͳ���pend,�������ж��Ƿ��п���*/    
    ioctl(pipe_id, FIONMSGS, (int)&msg_num);
    if(MML_WATCH_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    /*ֻ��PAUSE��ACTIVE״̬���԰����ݴ洢�ڻ�����*/
    if(ACTIVE == mMmlCnfg[winNum].Status
        || PAUSE == mMmlCnfg[winNum].Status)
    {
        /*���뻺�����ڴ�*/
        buf_ptr = GetDataBuf(mDataBuf);
        if( NULL == buf_ptr)
        {
#ifdef DATACOLLECT_DEBUG
            
                SuperMmlPrintf(MAX_MML_MMLNUM, "the debug buffer is full\r\n");
        
#endif
            return ERROR;
        }
        
        /* ���ڿ�����telnet����Ҳ����ʱkit����,ÿ�η�����Ϣʱ��������Ϣ����,
         * ������Ϊ��ʾ������Ϣ,WinPtr[].config������ʾ�Ƿ��һ�η���,
         * ��ʾ��һ�η���,1��ʾ���ǵ�һ�η���.��װ��ʽ����:
         *  LEN  |  2  | 1 |       <=512-1     |
         *       |-----|---|-------------------|
         *  DATA |LEN+3| 1 |       DATA        |
         *       |-----|---|-------------------|
         */
        if(length > MML_DATABUF_SIZE-1)
        {           
            length = MML_DATABUF_SIZE-1;
        }
        *(short*)buf_ptr     = length+3;  /*�������� ���� ����*/
        *(char*)(buf_ptr+2)  = 1;         /*������������*/
        WinPtr[winId].config = 1;         /*�����Ƿ��һ�η��ͱ�־Ϊ1*/
        memcpy(buf_ptr+3, buf, length);

        /*���������ڴ��׵�ַ���͵��û�����pipe��*/
        data_ptr = (int)buf_ptr;
        if(MML_POINTER_SIZE != writes(pipe_id, (char*)&data_ptr, MML_POINTER_SIZE))
        {
            PutDataBuf(buf_ptr,mDataBuf);
            return ERROR;
        }
    }
    return OK;
}


/*==============================================================================
Function:   DataCollectTool::MmlLogEx

Description:
 This function is the data collect interface function for upper App

Input Value:
    winId : int     ---- ID of log windows
    buf : char*     ---- log content
    lenth: int      ---- length of log content
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
#if defined(SW_CPB)||defined(MCU_SW)
STATUS DataCollectTool::MmlLogEx(int winId, const char* buf, int length)
{
    int pipe_id = 0, data_ptr, msg_num;
    int winNum;
    char* buf_ptr = NULL;
#ifndef WIN32   
    if(TRUE == intContext())
    {
        logMsg("don't allow call DataCollectTool::MmlLog in interrupt context!\r\n",0,0,0,0,0,0);
        return ERROR;
    }
#endif
    if( 0 == mInitialized )
    {
        return ERROR;
    }    
    if((winId < 0)||(winId >= MAX_WINPTR_NUM))
        return ERROR;
    winNum=WinPtr[winId].winNum;
    if((winNum < 0)||(winNum >= MAX_MML_WNDNUM))
        return ERROR;       
    pipe_id = mMmlCnfg[winNum].PipeId;
    if(0 == pipe_id)
    {
        return ERROR;
    }
    /*���ڵ�pipe�Ѿ������,�����·��Ͳ���pend,�������ж��Ƿ��п���*/    
    ioctl(pipe_id, FIONMSGS, (int)&msg_num);
    if(MML_WATCH_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    /*ֻ��PAUSE��ACTIVE״̬���԰����ݴ洢�ڻ�����*/
    if(ACTIVE == mMmlCnfg[winNum].Status
        || PAUSE == mMmlCnfg[winNum].Status)
    {
        /*���뻺�����ڴ�*/
        buf_ptr = GetDataBuf(mBBDDataBuf);
        if( NULL == buf_ptr)
        {
#ifdef DATACOLLECT_DEBUG
            
                SuperMmlPrintf(MAX_MML_MMLNUM, "the debug buffer is full\r\n");
        
#endif
            return ERROR;
        }
        
        /* ���ڿ�����telnet����Ҳ����ʱkit����,ÿ�η�����Ϣʱ��������Ϣ����,
         * ������Ϊ��ʾ������Ϣ,WinPtr[].config������ʾ�Ƿ��һ�η���,
         * ��ʾ��һ�η���,1��ʾ���ǵ�һ�η���.��װ��ʽ����:
         *  LEN  |  2  | 1 |       <=4096-1     |
         *       |-----|---|-------------------|
         *  DATA |LEN+3| 1 |       DATA        |
         *       |-----|---|-------------------|
         */
        if(length > MML_BBDDATABUF_SIZE-1)
        {           
            length = MML_BBDDATABUF_SIZE-1;
        }
        *(short*)buf_ptr     = length+3;  /*�������� ���� ����*/
        *(char*)(buf_ptr+2)  = 1;         /*������������*/
        WinPtr[winId].config = 1;         /*�����Ƿ��һ�η��ͱ�־Ϊ1*/
        memcpy(buf_ptr+3, buf, length);

        /*���������ڴ��׵�ַ���͵��û�����pipe��*/
        data_ptr = (int)buf_ptr;
        if(MML_POINTER_SIZE != writes(pipe_id, (char*)&data_ptr, MML_POINTER_SIZE))
        {
            PutDataBuf(buf_ptr,mBBDDataBuf);
            return ERROR;
        }
    }
    return OK;
}
#endif

/*==============================================================================
Function:   DataCollectTool::MmlLogPrintf

Description:
 This function is the data collect interface function for upper App

Input Value:
    winId : int     ---- ID of log and print windows
    buf : char*     ---- log and print content
    lenth: int      ---- length of log and print content
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlLogPrintf(int winId, const char* buf, int length)
{
    int pipe_id = 0, data_ptr, msg_num;
    int winNum;
    char* buf_ptr = NULL;
    short len;
#ifndef WIN32   
    if(TRUE == intContext())
    {
        logMsg("don't allow call DataCollectTool::MmlLogPrintf in interrupt context!\r\n",0,0,0,0,0,0);
        return ERROR;
    }
#endif
    if( 0 == mInitialized )
    {
        return ERROR;
    }    
    if((winId < 0)||(winId >= MAX_WINPTR_NUM))
        return ERROR;
    winNum=WinPtr[winId].winNum;
    if((winNum < 0)||(winNum >= MAX_MML_WNDNUM))
        return ERROR;    
        pipe_id = mMmlCnfg[winNum].PipeId;
    if(0 == pipe_id)
    {
        return ERROR;
    }
    /*���ڵ�pipe�Ѿ������,�����·��Ͳ���pend,�������ж��Ƿ��п���*/    
    ioctl(pipe_id, FIONMSGS, (int)&msg_num);
    if(MML_WATCH_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    /*ֻ��PAUSE��ACTIVE״̬���԰����ݴ洢�ڻ�����*/
    if(ACTIVE == mMmlCnfg[winNum].Status
        || PAUSE == mMmlCnfg[winNum].Status)
    {
        /*���뻺�����ڴ�*/
        buf_ptr = GetDataBuf(mDataBuf);
        if( NULL == buf_ptr)
        {
#ifdef DATACOLLECT_DEBUG
            
                SuperMmlPrintf(MAX_MML_MMLNUM, "the debug buffer is full\r\n");
#endif          
            return ERROR;
        }
       
        /* ���ڿ�����telnet����Ҳ����ʱkit����,ÿ�η�����Ϣʱ��������Ϣ����,
         * ������Ϊ��ʾ������Ϣ,WinPtr[].config������ʾ�Ƿ��һ�η���,
         * ��ʾ��һ�η���,1��ʾ���ǵ�һ�η���.��װ��ʽ����:
         *  LEN  |  2  | 1 |  2   |     <=512-3     |
         *       |-----|---|------|-----------------|
         *  DATA |LEN+5| 2 |  LEN |     DATA        |
         *       |-----|---|------|-----------------|
         */
        if(length > MML_DATABUF_SIZE-3)
        {           
            length = MML_DATABUF_SIZE-3;
        }
        *(short*)buf_ptr     = length+5;  /*��������  ����   ����   ����*/
        *(char*)(buf_ptr+2)  = 2;         /*������������*/
        WinPtr[winId].config = 1;         /*�����Ƿ��һ�η��ͱ�־Ϊ1*/
#ifdef WIN32
        len = htons((short)length);
#else
        len = (short)length;
#endif
        memcpy(buf_ptr+3, &len, 2);
        memcpy(buf_ptr+5, buf, length);

        /*���������ڴ��׵�ַ���͵��û�����pipe��*/
        data_ptr = (int)buf_ptr;
        if(MML_POINTER_SIZE != writes(pipe_id, (char*)&data_ptr, MML_POINTER_SIZE))
        {
            PutDataBuf(buf_ptr,mDataBuf);
            return ERROR;
        }
    }
    return OK;

}
/*==============================================================================
Function:   DataCollectTool::MmlLogPrintfEx

Description:
 This function is the data collect interface function for upper App
 This function is specially used in BBDlsAgent module. 

Input Value:
    winId : int     ---- ID of log and print windows
    buf : char*     ---- log and print content
    lenth: int      ---- length of log and print content
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
#if defined(SW_CPB)||defined(MCU_SW)
STATUS DataCollectTool::MmlLogPrintfEx( int winId, const char* buf, int length )
{
    int pipe_id   = 0;
    int data_ptr  = 0;
    int msg_num   = 0;
    int winNum    = 0;
    short len     = 0;
    char* buf_ptr = NULL;

    if( NULL == buf || length <= 0 )
    { 
#ifndef WIN32
        logMsg( "DataCollectTool::MmlLogPrintfEx():parameter(buf=0x%x,length=%d) error!\r\n", (int)buf,length,0,0,0,0 );
#else
        printf( "DataCollectTool::MmlLogPrintfEx():parameter(buf=0x%x,length=%d) error!\r\n", buf,length );
#endif
        return ERROR;
    }
    
#ifndef WIN32
    /*�ж��в�������øú���*/
    if( TRUE == intContext() )
    {
        logMsg( "don't allow call DataCollectTool::MmlLogPrintf in interrupt context!\r\n", 0,0,0,0,0,0 );
        return ERROR;
    }
#endif

    if( 0 == mInitialized )
    {
        return ERROR;
    }
    if( (winId < 0) || (winId >= MAX_WINPTR_NUM) )
    {
        return ERROR;
    }
    winNum = WinPtr[winId].winNum;
    if( (winNum < 0) || (winNum >= MAX_MML_WNDNUM) )
    {
        return ERROR;    
    }
    /*�õ��û��Ļ���pipe ID*/
    pipe_id = mMmlCnfg[winNum].PipeId;
    if( 0 == pipe_id )
    {
        return ERROR;
    }    
    /*���ڵ�pipe�Ѿ������,�����·��Ͳ���pend,�������ж��Ƿ��п���*/    
    ioctl( pipe_id, FIONMSGS, (int)&msg_num );
    if(MML_WATCH_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    
    /*ֻ��PAUSE��ACTIVE״̬���԰����ݴ洢�ڻ�����*/
    if( ACTIVE == mMmlCnfg[winNum].Status
        || PAUSE == mMmlCnfg[winNum].Status )
    {
        /*���뻺�����ڴ�*/
        buf_ptr = GetDataBuf( mBBDDataBuf );
        if( NULL == buf_ptr)
        {
#ifdef DATACOLLECT_DEBUG            
            SuperMmlPrintf( MAX_MML_MMLNUM, "the debug buffer is full\r\n" );
#endif          
            return ERROR;
        }
       
        /* ���ڿ�����telnet����Ҳ����ʱkit����,ÿ�η�����Ϣʱ��������Ϣ����,
         * ������Ϊ��ʾ������Ϣ,WinPtr[].config������ʾ�Ƿ��һ�η���,
         * ��ʾ��һ�η���,1��ʾ���ǵ�һ�η���.��װ��ʽ����:
         *  LEN  |  2  | 1 |  2   |     <=4096-3    |
         *       |-----|---|------|-----------------|
         *  DATA |LEN+5| 2 |  LEN |     DATA        |
         *       |-----|---|------|-----------------|
         */
        if(length > MML_BBDDATABUF_SIZE-3)
        {           
            length = MML_BBDDATABUF_SIZE-3;
        }
        *(short*)buf_ptr     = length+5;  /*��������  ����   ����   ����*/
        *(char*)(buf_ptr+2)  = 2;         /*������������*/
        WinPtr[winId].config = 1;         /*�����Ƿ��һ�η��ͱ�־Ϊ1*/
#ifdef WIN32
        len = htons((short)length);
#else
        len = (short)length;
#endif
        memcpy(buf_ptr+3, &len, 2);
        memcpy(buf_ptr+5, buf, length);
        
        /*���������ڴ��׵�ַ���͵��û�����pipe��*/
        data_ptr = (int)buf_ptr;
        if( MML_POINTER_SIZE != writes( pipe_id, (char*)&data_ptr, MML_POINTER_SIZE ) )
        {
            PutDataBuf( buf_ptr, mBBDDataBuf );
            return ERROR;
        }
    }
    
    return OK;
}
#endif
/*==============================================================================
Function: DataCollectTool::StatRegister

Description:
This function is used to register statistic identifier

Input Value:
    winId:int           ---- ID of statistic windows
    paraName: char*     ---- name of statistic parameter
    
Output Value:
    None
Return Value:
    char         Statistic parameter identifier
    ERROR        fail
Side Effect:
    None
==============================================================================*/
char DataCollectTool::StatRegister(int winId, char* ParaName)
{
    
    char buf[65];
    int len;
    int i;
    int num;
    char tempId;
    char* buf_ptr = buf;
    memset(buf, 0, 65); 

    if( 0 == mInitialized ) 
        return ERROR;  
    if((winId < 0)||(winId >= MAX_STATPTR_NUM)) 
        return ERROR;
    len=strlen(ParaName);
    if(len > 63)
    {
        return ERROR;
    }
    strcpy((buf_ptr+1), ParaName);
    
    for(i=0; i<STAT_NAME_NUM; i++)
    {
        if( 0 == StatPtr[winId].mStatName[i].flag )
            break;
    }
    if( STAT_NAME_NUM == i)
    {
        return ERROR;
    }
    tempId = (char)i;
    *(char*)buf_ptr = tempId;
    memcpy(StatPtr[winId].mStatName[i].buf, buf, 65);
    
    /*��ʶ��ͳ��ID�ѱ������*/
    StatPtr[winId].mStatName[i].flag = 1;
    /*ֻ��ͳ�����ӽ������Ż�����StatPtr[winId].winNum��������������ӽ�����������ͬ*/
    if((StatPtr[winId].winNum >= 0)&&(StatPtr[winId].winNum < MAX_MML_STATNUM)) 
    {
        num = StatPtr[winId].winNum;
        if(mMmlStat[num].SockId > 0)
        {
            send(mMmlStat[num].SockId, buf, 65, 0);         
        }       
    }   
    return tempId;
}

/*==============================================================================
Function: DataCollectTool::StatUnRegister

Description:
This function is used to unregister statistic identifier

Input Value:
    winId:int       ----  ID of statistic windows
    StatId:char     ----  Statistic parameter identifier
    
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/

STATUS DataCollectTool::StatUnRegister(int winId, signed char StatId)
{

    char temp;

    if( 0 == mInitialized )
    {
        return ERROR;
    }

    if(winId <0 ||winId >= MAX_STATPTR_NUM)
    {
        return ERROR;
    }

    /*ͳ��ID�Ų�����3*/
    if((StatId < 0)||(StatId > 3))
    {
        return ERROR;
    }
    /*��ID��û�б������*/
    if( 0 == StatPtr[winId].mStatName[StatId].flag)
    {
        return ERROR;
    }
    /*֪ͨKit���ͷ�ͳ��ID��*/
    if( (StatPtr[winId].winNum >= 0)&&(StatPtr[winId].winNum < MAX_MML_STATNUM))
    {
        temp = StatId & (0x3f);
        temp = temp ^ (0x80);
    
        if(mMmlStat[StatPtr[winId].winNum].SockId > 0)
        {
            send(mMmlStat[StatPtr[winId].winNum].SockId,(char*)&temp, 1, 0);            
        }
    }
    StatPtr[winId].mStatName[StatId].flag = 0;
    memset(StatPtr[winId].mStatName[StatId].buf, 0, 65);
    
    return OK;

}

/*==============================================================================
Function: DataCollectTool::MmlStatistic

Description:
This function is the data collect interface function for upper App

Input Value:
    paraName: char*     ---- name of statistic parameter
    StatId:char         ---- Statistic parameter identifier
    paraSize: int       ---- parameter value
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlStatistic(int winId, signed char StatId, int paraSize)
{
    int msg_num, data_ptr;
    int num;
#ifdef WIN32
    INT64 StatTime;
#else
    long long StatTime;
#endif
    char sid;
    char* buf_ptr = NULL;
    
#ifndef WIN32
    if(TRUE == intContext())
    {
        logMsg("don't allow call DataCollectTool::MmlStatistic in interrupt context!\r\n",0,0,0,0,0,0);
        return ERROR;
    }
#endif
    if( 0 == mInitialized )
    {
        return ERROR;
    }
    if(winId <0 ||winId >= MAX_STATPTR_NUM)
        return ERROR;
    num = StatPtr[winId].winNum;  
    /*������ж����������ã�һ���Ǳ߽��飬һ�����ж�ͳ�������Ƿ����*/
    if(num < 0 || num >= MAX_MML_STATNUM)
        return ERROR;       
    if((StatId < 0)||(StatId >= STAT_NAME_NUM))
    {
        /*  MmlPrintf("the wrong StatId %d\r\n", StatId);*/
        return ERROR;
    }
    if( 0 == StatPtr[winId].mStatName[StatId].flag)
    {
        /*  MmlPrintf("has not register the StatId %d\r\n", StatId);*/
        return ERROR;
    }

    /*��ȡĬ�ϵĺ�����*/
#ifdef WIN32
    StatTime = getTimeBase();     /*��ȷ��ms*/
#else
    StatTime = getTimeBase();

    StatTime=StatTime/PHY_TICK_1MS;     /*��ȷ��ms*/
#endif  

    if(0 == mMmlStat[num].PipeId)
    {
        return ERROR;
    }
    ioctl(mMmlStat[num].PipeId, FIONMSGS, (int)&msg_num);
    if(MML_STAT_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    
    buf_ptr = GetDataBuf(mStatBuf);
    if( NULL == buf_ptr)
    {
#ifdef DATACOLLECT_DEBUG
        
            SuperMmlPrintf(MAX_MML_MMLNUM, "the statistic buffer is full\r\n");
        
#endif
        return ERROR;
    }
    memset(buf_ptr, 0x00, MML_STATBUF_SIZE+2);
    /*ǰ2��bit�Ǳ�ʶ��Ϊ01�����ڱ�ʾ���͵��ǲ���ֵ������6��bit��ͳ��id��*/
    sid = StatId & 0x3f;
    sid = sid ^ 0x40;
    *(buf_ptr+2) = sid;
    /*��ʵ���Դ�buf_ptr��ʼ��ţ���2��byte��ԭ����Ϊ���Ժ���չ�ã���ŷ������ݵĳ���*/
#ifdef WIN32
    *(unsigned long*)(buf_ptr+3) = (htonl)((unsigned long)paraSize);
    *(INT64*)(buf_ptr+7) = htonll(StatTime);
#else   
    *(unsigned long*)(buf_ptr+3) = (unsigned long)paraSize;
    *(long long*)(buf_ptr+7) = StatTime;

#endif
     data_ptr = (int)buf_ptr;
     /*  *((_int64*)(buf_ptr+7)+1)=0xff; */

     /*����������д�뵽PIPE�У��ȴ�����ʱ���͵�ͳ��������*/
     if(MML_POINTER_SIZE != writes(mMmlStat[num].PipeId, (char*)&data_ptr, MML_POINTER_SIZE))
     {
         PutDataBuf(buf_ptr,mStatBuf);
         return ERROR;
     }
     return OK;
}

/*==============================================================================
Function: DataCollectTool::MmlStatistic

Description:
This function is the data collect interface function for upper App

Input Value:
    winId                       ---- ID of statistic windows
    StatId:char                 ---- Statistic parameter identifier
    paraSize: int               ---- parameter value
    coordinate: unsigned int    ---- parameter abscissa
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/

STATUS DataCollectTool::MmlStatistic(int winId, signed char StatId, int paraSize, unsigned int coordinate) 
{
    int msg_num, data_ptr, num;
#ifdef WIN32
    INT64 StatTime;
#else
    long long StatTime;
#endif
    char sid;
    char* buf_ptr = NULL;

#ifndef WIN32
    if(TRUE == intContext())
    {
        logMsg("don't allow call DataCollectTool::MmlStatistic with parameter in interrupt context!\r\n",0,0,0,0,0,0);
        return ERROR;
    }
#endif
    if( 0 == mInitialized )
    {
        return ERROR;
    }
    if(winId <0 ||winId >= MAX_STATPTR_NUM)
        return ERROR;
    num = StatPtr[winId].winNum;  
    
    /*������ж����������ã�һ���Ǳ߽��飬һ�����ж�ͳ�������Ƿ����*/
    if(num < 0 || num >= MAX_MML_STATNUM)
        return ERROR;       
    if((StatId < 0)||(StatId >= STAT_NAME_NUM))
    {
        //  MmlPrintf("the wrong StatId %d\r\n", StatId);
        return ERROR;
    }
    if( 0 == StatPtr[winId].mStatName[StatId].flag)
    {
        //  MmlPrintf("has not register the StatId %d\r\n", StatId);
        return ERROR;
    }
#ifdef WIN32
    StatTime = (INT64)coordinate;
#else
    StatTime = (long long)coordinate;
#endif

    if(0 == mMmlStat[StatPtr[winId].winNum].PipeId)
    {
        return ERROR;
    }
    ioctl(mMmlStat[StatPtr[winId].winNum].PipeId, FIONMSGS, (int)&msg_num);
    if(MML_STAT_PIPE_NUM == msg_num)
    {
        return ERROR;
    }
    
    buf_ptr = GetDataBuf(mStatBuf);
    if( NULL == buf_ptr)
    {
#ifdef DATACOLLECT_DEBUG
        
            SuperMmlPrintf(MAX_MML_MMLNUM, "the statistic buffer is full\r\n");
    
#endif
        return ERROR;
    }
    memset(buf_ptr, 0x00, MML_STATBUF_SIZE+2);
    /*ǰ2��bit�Ǳ�ʶ��Ϊ01�����ڱ�ʾ���͵��ǲ���ֵ������6��bit��ͳ��id��*/
    sid = StatId & 0x3f;
    sid = sid ^ 0x40;
    *(buf_ptr+2) = sid;
    /*��ʵ���Դ�buf_ptr��ʼ��ţ���2��byte��ԭ����Ϊ���Ժ���չ�ã���ŷ������ݵĳ���*/


#ifdef WIN32
    *(unsigned long*)(buf_ptr+3) = (htonl)((unsigned long)paraSize);
    *(INT64*)(buf_ptr+7) = htonll(StatTime);
#else   
    *(unsigned long*)(buf_ptr+3) = (unsigned long)paraSize;
    *(long long*)(buf_ptr+7) = StatTime;

#endif

    data_ptr = (int)buf_ptr;
    /*  *((_int64*)(buf_ptr+7)+1)=0xff; */

     /*����������д�뵽PIPE�У��ȴ�����ʱ���͵�ͳ��������*/  
    if(MML_POINTER_SIZE != writes(mMmlStat[StatPtr[winId].winNum].PipeId, (char*)&data_ptr, MML_POINTER_SIZE) )
    {
         PutDataBuf(buf_ptr,mStatBuf);
         return ERROR;
    }
    return OK;
}

/*==============================================================================
Function:   DataCollectTool::MmlPause

Description:
 This function pauses printing user data information 

Input Value:
    winId : int     ---- ID of debug windows
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlPause(int winId)
{
    MmlCtrlOrd ctrl_ord;
        
    ctrl_ord.OrdCode = TOPAUSE;
    ctrl_ord.OrdMsg.WinId  = winId;    
    return SendToPipe(&ctrl_ord);
}

/*==============================================================================
Function:   DataCollectTool::MmlResume

Description:
 This function resumes to print user data information 

Input Value:
    winId : int     ---- ID of debug windows
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlResume(int winId)
{
    MmlCtrlOrd ctrl_ord;
  
    ctrl_ord.OrdCode = TOACTIVE;
    ctrl_ord.OrdMsg.WinId = winId;
    return SendToPipe(&ctrl_ord);
}

/*==============================================================================
Function:   DataCollectTool::MmlTimer

Description:
 This function starts to print user data information every some period 

Input Value:
    winId : int         ---- ID of debug windows
    period : int        ---- period of time print
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlTimer(int winId, int period)
{
    MmlCtrlOrd ctrl_ord;
   
    ctrl_ord.OrdCode = TOTIMER;
    ctrl_ord.OrdMsg.WinId = winId;
    ctrl_ord.OrdMsg.Peroid = period;   
    return SendToPipe(&ctrl_ord);
}

/*==============================================================================
Function:   DataCollectTool::MmlExit

Description:
 This function releases resources when data collect tool exits 

Input Value:
    None
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlExit()
{
    int i;
    
    if( 0 != mDataCollectTaskId )
        taskDelete(mDataCollectTaskId);
    if(0 != mListenTaskId )
        taskDelete(mListenTaskId);
    if(NULL != semIdForDataBuf)
        semDelete(semIdForDataBuf);
    if(tty_sockid != 0)
        MmlCloseSocket(tty_sockid);
    if(watch_sockid != 0)
        MmlCloseSocket(watch_sockid);
    if(stat_sockid != 0)
        MmlCloseSocket(stat_sockid);
    if(ctrlPipeId > 0)
    {
        closes(ctrlPipeId);
    }
    /*�ͷ�MML������Դ*/
    for(i=0; i < MAX_MML_MMLNUM; i++)
    {
        if(mMmlCtrl[i].SockId > 0)
        {
            MmlCloseSocket(mMmlCtrl[i].SockId);
            mMmlCtrl[i].SockId = 0;
            mMmlCtrl[i].WinId = 0;
            mMmlCtrl[i].Status = IDLE;
            currentpos[i] = 0;
            indication[i] = 0;
            mMmlConnNum--;
        }
    }
    /* �ͷŵ���������Դ*/
    for(i = 0; i < MAX_MML_WNDNUM; i++)
    {
        if(mMmlCnfg[i].PipeId > 0)   
        {
            closes(mMmlCnfg[i].PipeId);
        }
        if(mMmlCnfg[i].SockId > 0)
        {
            MmlCloseSocket(mMmlCnfg[i].SockId);
            UserSockFail(i);
        }
        if(mMmlCnfg[i].WdTimer != NULL)
        {
            wdDelete(mMmlCnfg[i].WdTimer);
        }
    }
    /*�ͷ�ͳ��������Դ*/
    for(i=0; i < MAX_MML_STATNUM; i++)
    {
        if(mMmlStat[i].PipeId > 0)   
        {
            closes(mMmlStat[i].PipeId);
        }
        if(mMmlStat[i].SockId > 0)
        {
            MmlCloseSocket(mMmlStat[i].SockId);
            StatSockFail(i);
        }
    }
    if( NULL != mMmlCmdPtr)
    {
        free(mMmlCmdPtr);
        mMmlCmdPtr = NULL;
    }
    memset(mMmlCallBack, 0, sizeof(mMmlCallBack));    
    mWndConnNum = 0;
    mMmlConnNum = 0;
    mInitialized = 0;
    /*�ͷ������ڴ�*/
    ExitDataBuf();
    pipeDelete("/mmlPipe/Ctrl");
    pipeDelete("/mmlPipe/Stat0");
    pipeDelete("/mmlPipe/Stat1");
    pipeDelete("/mmlPipe/Stat2");
    pipeDelete("/mmlPipe/Stat3");
    pipeDelete("/mmlPipe/Stat4");
    pipeDelete("/mmlPipe/Stat5");
    pipeDelete("/mmlPipe/Stat6");
    pipeDelete("/mmlPipe/Stat7");
    pipeDelete("/mmlPipe/Watch0");
    pipeDelete("/mmlPipe/Watch1");
    pipeDelete("/mmlPipe/Watch2");
    pipeDelete("/mmlPipe/Watch3");
    pipeDelete("/mmlPipe/Watch4");
    pipeDelete("/mmlPipe/Watch5");
    pipeDelete("/mmlPipe/Watch6");
    pipeDelete("/mmlPipe/Watch7");
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::InitDataBuf

Description:
 This function initializes data collect buffer 

Input Value:
    bufSize : int           ---- buffer size 
    bufNum : int            ---- buffer number
    memBuf : MemoryBuf&     ---- memory class instance  
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::InitDataBuf(int bufSize, int bufNum, MemoryBuf& memBuf)
{
    char* tmp_ptr;
    
    memBuf.PhyMemPtr = (char*)malloc((bufSize+2)*bufNum);
    if(NULL == memBuf.PhyMemPtr)
    {
        return ERROR;
    }
#ifdef DATACOLLECT_DEBUG
    memBuf.FreeNum = bufNum;
#endif
    memBuf.HeadPtr = memBuf.PhyMemPtr;
    tmp_ptr = memBuf.HeadPtr;
    while(bufNum-1)
    {
        *(int*)tmp_ptr = (int)(tmp_ptr + bufSize + 2);
        tmp_ptr = tmp_ptr + bufSize + 2;
        bufNum--;
    }
    memBuf.TailPtr = tmp_ptr;
    *(int*)tmp_ptr = 0;
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::GetDataBuf

Description:
 This function gets a buf from data collect buffer 

Input Value:
    memBuf : MemoryBuf&     ---- memory class instance
Output Value:
    None
Return Value:
    char*       buffer pointer
    NULL        fail
Side Effect:
    None
==============================================================================*/
char* DataCollectTool::GetDataBuf(MemoryBuf& memBuf)
{
    char* tmp_ptr=NULL;
    
    /*if(ERROR == taskLock())
    {
        printf("error in taskLock\r\n");
        return NULL;
    }*/
   
    if( NULL == memBuf.HeadPtr)
    {
        /*taskUnlock();*/
    
        return NULL;
    }
   if(ERROR ==semTake(semIdForDataBuf,NO_WAIT))
   {
    MmlPrintf(" DataCollectTool::GetDataBuf :semTake fail!\r\n");
        return NULL;
    }
    if(memBuf.HeadPtr == memBuf.TailPtr)
    {
        tmp_ptr = memBuf.HeadPtr;
        memBuf.HeadPtr = memBuf.TailPtr = NULL;
    }
    else
    {
        tmp_ptr = memBuf.HeadPtr;
        memBuf.HeadPtr = (char*)*(int*)memBuf.HeadPtr;
    }
#ifdef DATACOLLECT_DEBUG
    memBuf.BusyNum++;
    memBuf.FreeNum--;
#endif
    /*taskUnlock();*/
    if(ERROR ==     semGive(semIdForDataBuf))
    {
        MmlPrintf(" DataCollectTool::GetDataBuf :semGive fail2!\r\n");
    }
/*
#ifdef DATACOLLECT_DEBUG
    MmlPrintf("get a node 0x%08X\r\n", (int)tmp_ptr);
#endif*/
    return tmp_ptr;
}
/*==============================================================================
Function:   DataCollectTool::PutDataBuf

Description:
 This function puts a buf to data collect buffer 

Input Value:
    ptr : char*             ---- buffer pointer 
    memBuf : MemoryBuf&     ---- memory class instance
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::PutDataBuf(char* ptr, MemoryBuf& memBuf)
{
    if( NULL == ptr)
    {
        return ERROR;
    }
    /*taskLock();*/
   if(ERROR ==semTake(semIdForDataBuf,WAIT_FOREVER))
   {
    MmlPrintf(" DataCollectTool::PutDataBuf :semTake fail!\r\n");
    return NULL;
   }
    if( NULL == memBuf.HeadPtr)
    {
        memBuf.HeadPtr = ptr;
        memBuf.TailPtr = ptr;
    }
    else
    {
        *(int*)memBuf.TailPtr = (int)ptr;
        memBuf.TailPtr = ptr;
    }
    *(int*)memBuf.TailPtr = 0;
#ifdef DATACOLLECT_DEBUG
    memBuf.BusyNum--;    
    memBuf.FreeNum++;
#endif
   /* taskUnlock();*/
    if(ERROR ==     semGive(semIdForDataBuf))
    {
        MmlPrintf(" DataCollectTool::PutDataBuf :semGive fail2!\r\n");
        return ERROR;
    }
/*
#ifdef DATACOLLECT_DEBUG
    MmlPrintf("free a node 0x%08X\r\n", (int)ptr);
#endif*/
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::ExitDataBuf

Description:
 This function releases data collect buffer 

Input Value:
    None
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::ExitDataBuf()
{
    if( NULL != mDataBuf.PhyMemPtr)
    {
        free(mDataBuf.PhyMemPtr);
        mDataBuf.PhyMemPtr = NULL;
        mDataBuf.HeadPtr = NULL;
        mDataBuf.TailPtr = NULL;
    }
#if defined(SW_CPB)||defined(MCU_SW)
    if( NULL != mBBDDataBuf.PhyMemPtr)
    {
        free(mBBDDataBuf.PhyMemPtr);
        mBBDDataBuf.PhyMemPtr = NULL;
        mBBDDataBuf.HeadPtr = NULL;
        mBBDDataBuf.TailPtr = NULL;
    }
#endif
    if( NULL != mMsgBuf.PhyMemPtr)
    {
        free(mMsgBuf.PhyMemPtr);
        mMsgBuf.PhyMemPtr = NULL;
        mMsgBuf.HeadPtr = NULL;
        mMsgBuf.TailPtr = NULL;
    }
    if( NULL != mStatBuf.PhyMemPtr)
    {
        free(mStatBuf.PhyMemPtr);
        mStatBuf.PhyMemPtr = NULL;
        mStatBuf.HeadPtr = NULL;
        mStatBuf.TailPtr = NULL;

    }
    printf("release mmlDataBuf\n");
}

/*==============================================================================
Function:   DataCollectTool::MmlBind

Description:
 This function binds socket 

Input Value:
    sockfd : int    ---- socket ID
    port : int      ---- port ID
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlBind(int sockfd, int port)
{
    struct sockaddr_in s;
    
    memset(&s, 0 ,sizeof(s));
    s.sin_family = AF_INET;
    s.sin_port = htons((short)port);
    s.sin_addr.s_addr = INADDR_ANY;
    return bind(sockfd, (struct sockaddr*)&s, sizeof(s));
}
/*==============================================================================
Function:   DataCollectTool::MmlCloseSocket

Description:
 This function closes socket 

Input Value:
    sockfd : int    ---- socket ID
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlCloseSocket(int sockfd)
{
    if( 0 ==sockfd )
        return 0;
    shutdown(sockfd, 2);
#ifdef WIN32
    return closesocket(sockfd);
#else
    return closes(sockfd);
#endif
}

/*==============================================================================
Function:   DataCollectTool::SendToTelnet

Description:
 This function sends string to telnet 

Input Value:
    index : int         ---- connection ID
    bufPtr : char*      ---- pointer to data for sending
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::SendToTelnet(int index, char* bufPtr)
{ 
    int len;    
    int lSendLen = 0;/*��ʾ�Ѿ����͵ĳ���*/
    int lSendCnt = 0;/*��ʾ�Ѿ����͵Ĵ���*/
    int lResult  = 0;/*��ʾ���͵Ľ��*/
    int lDlyTime = 0;/*��ʾ�����ӳ�*/
    char cSendType = -1;  /*��¼������������*/

    /*�������ݵ�MML����*/
    if( WINID_MML == index)     
    {      
        if(mMmlCtrl[activeId].SockId > 0)
        {
            send(mMmlCtrl[activeId].SockId, bufPtr, strlen(bufPtr),0);                  
            send(mMmlCtrl[activeId].SockId, mMmlCtrl[activeId].Prompt, strlen(mMmlCtrl[activeId].Prompt),0);            
        }
        else
            return ERROR;
    }
    else
    {
        /*�������ݵ�Kit�ĵ�������*/
        if( 1 == mMmlCnfg[index].kit_telnet_connect)    
        {
            len = *(short*)bufPtr;
            cSendType = *(bufPtr+2); /*ȡ����������*/
            if(mMmlCnfg[index].SockId > 0)
            {
                /*MmlPrintf()���������:ֻ����һ�Σ����ʧ�ܾͶ���*/
                if( 0 == cSendType )
                {
                    /*ֻ�������ݲ��֣����������ͺͳ���*/
                    send( mMmlCnfg[index].SockId, bufPtr+3, len-3, 0 );
                    return OK;
                }
                /*��������Ϊ1��2�Ĵ���:�����ܵķ���*/
                lDlyTime = sysClkRateGet()/100; /*100��֮1��*/
                lResult = send( mMmlCnfg[index].SockId, bufPtr, len, 0 );
    		    if( 1 == g_CaptureDataFlag ) 
    		    {
		    		/*�����ܵķ������ݣ�������ݴ���������ܻᵼ������ʱ��Delay*/
                    if( lResult != ERROR )
                    {
                        lSendLen += lResult;
                    }
                    lSendCnt++;
                    /*���û�з�����ϣ���������ʣ�������*/
                    while( lSendLen < len )
                    {
                        /*���ӳٺ��������*/
                        taskDelay( lDlyTime );
                        lDlyTime += lDlyTime;
                        /*�����ܷ���*/
                        lResult = send( mMmlCnfg[index].SockId, bufPtr+lSendLen, 
                                        len-lSendLen, 0 );
                        if( lResult != ERROR )
                        {
                            lSendLen += lResult;
                        }
                        /*�������10�λ�û�з�����ϣ����˳�*/
                        lSendCnt++;
                        if( 10 == lSendCnt )
                        {
                            break;
                        }
                    }
                    /*ȷʵ���Ͳ���ȥ�����󷵻�*/
                    if( lSendLen < len )
                    {
                        printf( "DataCollectTool::SendToTelnet():send (%d < %d) failed\r\n", lSendLen, len );
                        return ERROR;
                    }               
                }
                else   
                {
                    /*�������ݾ�����Ϊ���������ʧ�ܣ����˾Ͷ��˰�*/
                    return OK;
                }
        	}
            else
                return ERROR;
        }
        /*�������ݵ�telnet�ĵ�������*/
        else if( 2 == mMmlCnfg[index].kit_telnet_connect)    
        {
            len = *(short*)bufPtr;        
            /*����telnet������Ҫ��ӡ����ʾ��*/
            if(mMmlCnfg[index].SockId > 0)
            {
                if(send(mMmlCnfg[index].SockId, bufPtr+3, len-3, 0) <= 0)
                {
                    taskDelay(AGAIN_SEND_TIME);
                    send(mMmlCnfg[index].SockId, bufPtr+3, len-3, 0);
                }               
                if(send(mMmlCnfg[index].SockId, mMmlCnfg[index].Prompt, strlen(mMmlCnfg[index].Prompt), 0) < 0)
                {
                    taskDelay(AGAIN_SEND_TIME);
                    send(mMmlCnfg[index].SockId, mMmlCnfg[index].Prompt, strlen(mMmlCnfg[index].Prompt), 0);
                }           
            }
            else
                return ERROR;
        }
        //sending data to connection which was not configured
        else                                    
        {           
            if(mMmlCnfg[index].SockId > 0)
            {
                send(mMmlCnfg[index].SockId, bufPtr, strlen(bufPtr),0);             
                send(mMmlCnfg[index].SockId, mMmlCnfg[index].Prompt, strlen(mMmlCnfg[index].Prompt),0);             
            }
            else
                return ERROR;
        }
    }
    return OK;
 
}
/*==============================================================================
Function:   DataCollectTool::SendToPipe

Description:
 This function sends contrl order to contrl pipe 

Input Value:
    ptr : MmlCtrlOrd*   ---- pointer to MML control command structure 
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::SendToPipe(MmlCtrlOrd* ptr)
{
    if( MML_CTRL_PIPE_SIZE == writes(ctrlPipeId, (char*)ptr, MML_CTRL_PIPE_SIZE))
    {
        return OK;
    }
    else 
        return ERROR;
}

/*==============================================================================
Function:   DataCollectTool::MyStrUpper

Description:
 This function converts string to capital letters

Input Value:
    ptr : char*     ---- alphabet
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::MyStrUpper(char* ptr)
{
    char c;

    for(int i = 0; (c=ptr[i]) != 0; i++)
    {
        if(c >= 'a' && c <= 'z')
        {
            ptr[i] = c-('a'-'A');
        }
    }
}

/*==============================================================================
Function:   DataCollectTool::IsNormalChar

Description:
 This function decides whether the char is normal 

Input Value:
    c : char        ---- character
Output Value:
    None
Return Value:
    0        abnormal
    1        normal
Side Effect:
    None
==============================================================================*/
int DataCollectTool::IsNormalChar(char c)
{
    int ret = 0;

    if((c >= 32) && (c <= 127))
    {
        ret = 1;
    }
    return ret;
}
/*==============================================================================
Function:   DataCollectTool::DelBlank

Description:
 This function deletes the blanks before the string 

Input Value:
    bufPtr : char*  ---- buffer
Output Value:
    None
Return Value:
    int
Side Effect:
    None
==============================================================================*/
int DataCollectTool::DelBlank(char* bufPtr)
{
    int i=0;

    while((( ' ' == bufPtr[i]) || ( 9 == bufPtr[i])
        || ( '\t' == bufPtr[i])) && ( '\0' != bufPtr[i]))
    {
        i++;
    }
    return i;
}
/*==============================================================================
Function:   DataCollectTool::HasSemicolon

Description:
 This function decides whether the string contains semicolon 

Input Value:
    ptr : char*     ---- character string
    len : int       ---- character string length
Output Value:
    None
Return Value:
    0        no
    1        yes
Side Effect:
    None
==============================================================================*/
int DataCollectTool::HasSemicolon(char* ptr, int len)
{
    int i, j;
    
    for(i = 0; i < len; i++)
    {
        if( ';' == ptr[i])
        {
            return 1;
        }
        if( '#' == ptr[i])
        {
            for (j = i; j < len; j++)
            {
                ptr[j] = ' ';
            }
            return 0;
        }
    }
    return 0;
}
/*==============================================================================
Function:   DataCollectTool::GetOneChar

Description:
 This function reads one char from the buffer 

Input Value:
    None
Output Value:
    None
Return Value:
    char
Side Effect:
    None
==============================================================================*/
char DataCollectTool::GetOneChar(int i) 
{
    char ch;
    int result;

    result = recv(mMmlCtrl[i].SockId, &ch, 1, 0);
    if( 1 == result)
    {
        return ch;
    }
    /*��MML���ڻ�û�������ݣ�����ͣ�������ݵ����*/
    else if( -1 == result)
    {
        return MML_NOVER;
    }
    else
    {       
        return MML_STOP;
    }
}

/*==============================================================================
Function:   DataCollectTool::ProcessChar

Description:
 This function decides the char type 

Input Value:
    ch : char       ---- character
Output Value:
    None
Return Value:
    DELETE_CHAR        delete key
    RETURN_CHAR        return key
    UNKOWN_CHAR        unkown key or error
Side Effect:
    None
==============================================================================*/
int DataCollectTool::ProcessChar(char ch)
{
    int ret = NORMAL_CHAR;

    switch(ch)
    {
        case KEY_DELETE:
        case KEY_BACKSPACE:
            ret = DELETE_CHAR;
            break;
        case KEY_RETURN:
            ret = RETURN_CHAR;
            break;
        case UNKOWN_CHAR:
            ret = UNKOWN_CHAR;  
            break;
        case MML_NOVER:
            ret = MML_NOVER;
            break;
        case MML_STOP:    
            return MML_STOP;
            break;

        default:
            break;
    }
    return ret;
}
/*==============================================================================
Function:   DataCollectTool::DeleteChar

Description:
 This function display the delete key 

Input Value:
    None
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::DeleteChar(int i)
{
    char buf[3] = {8, ' ', 8};
    
    if(mMmlCtrl[i].SockId > 0)
    {
        send(mMmlCtrl[i].SockId, buf, 3, 0);        
        return  OK;
    }
    return ERROR;
}
/*==============================================================================
Function:   DataCollectTool::GetOneLine

Description:
 This function reads one line character to buffer from mml connection

Input Value:
    i:int       ---- mml connection
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::GetOneLine(int i)
{
    int temp;
    char ch;
    char tempbuf[18];
    int recvlen=0;
    
    for (;;)
    {
        /*��MML�����Ͻ����ַ�*/
        ch = GetOneChar(i); 
        /*����ǰ�ַ�����*/
        temp = ProcessChar(ch);             
        switch (temp)
        {
            case NORMAL_CHAR:
                indication[i]=0;
                /*���δ��MML���ڷ��͹������ַ���ȫ�ֵ�buffer��*/
                if(currentpos[i] >= (MML_CMD_LEN - 1))
                {
                    SuperMmlPrintf(i, "\r\nMML command length exceed given length\r\n");
                    currentpos[i]=0;
                    return ERROR;
                }
                gMmlDataBuf[i][currentpos[i]++] = ch;               
                send(mMmlCtrl[i].SockId, &ch, 1, 0);
                recvlen +=1;
                break;              
            
            case DELETE_CHAR:
                indication[i]=0;
                /*�ӻ�������ɾ��ǰһ���ַ�*/
                if(currentpos[i] > 0)
                {
                    gMmlDataBuf[i][currentpos[i]-1]='\0';
                    currentpos[i]--;
                    if( ERROR == DeleteChar(i))
                    {
                        return ERROR;
                    }
                }
                recvlen +=1;
                break;
            case RETURN_CHAR:
                indication[i]=0;
                if(( ':' == gMmlDataBuf[i][0])&&( 1 != HasSemicolon(gMmlDataBuf[i],currentpos[i]))&&(0 != currentpos[i]))
                {
                    indication[i] = MML_NOVER;
                    SuperMmlPrintf(i, "\r\nmissing semicolon,continue enter MML command\r\n");
                    recvlen +=1;
                    break;                  
                }
                else
                {
                    /*�����س���ֹͣ��ȡ������*/
                    if(currentpos[i] >= MML_CMD_LEN )
                    {
                        SuperMmlPrintf(i, "\r\nMML command length exceed given length\r\n");
                        currentpos[i]=0;
                        return ERROR;
                    }
                    gMmlDataBuf[i][currentpos[i]]='\0';
                    currentpos[i]=0;
                    sprintf(tempbuf, "\r\n%s",mMmlCtrl[i].Prompt);
                    if(mMmlCtrl[i].SockId > 0)
                    {
                        send(mMmlCtrl[i].SockId, tempbuf, strlen(tempbuf), 0);          
                        return OK;                      
                    }
                    return ERROR;
                }
            case UNKOWN_CHAR:
                indication[i]=0;
                recvlen +=1;
                return ERROR;
                break;
            case MML_NOVER:
                if(0 == recvlen)
                {
                    MmlCloseSocket(mMmlCtrl[i].SockId);
                    mMmlCtrl[i].SockId = 0;
                    mMmlCtrl[i].Status = IDLE;
                    mMmlCtrl[i].WinId = 0;
                    currentpos[i] = 0;
                    indication[i] = 0;
                    mMmlConnNum--;
                    return ERROR;
                }
                /*MML����δ������*/
                indication[i] = MML_NOVER;
                return OK;
                break;
            case MML_STOP:
                MmlCloseSocket(mMmlCtrl[i].SockId);
                mMmlCtrl[i].SockId = 0;
                mMmlCtrl[i].Status = IDLE;
                mMmlCtrl[i].WinId = 0;
                currentpos[i] = 0;
                indication[i] = 0;
                mMmlConnNum--;
                return ERROR;
                break;
            default:
                break;
        }
    }  
    return OK;

}

/*==============================================================================
Function:   DataCollectTool::AnalyseDosCmd

Description:
 This function Analyses Dos command  

Input Value:
    strPtr : char*          ---- buffer for saving character string 
    dosCmd : DosCmdType&    ---- dos command structure
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
int DataCollectTool::AnalyseDosCmd(char* strPtr, DosCmdType& dosCmd)
{
    char cmd_para[20];
    int i = 0, tmp, para_num = 0;

    while( '\0' != strPtr[i])
    {
        tmp = DelBlank(strPtr+i);
        i = i + tmp;
        while( '\0' != strPtr[i] && ' ' != strPtr[i] )
        {
            if(1 == IsNormalChar(strPtr[i]))
            {
                if(para_num >= MML_NAME_LEN)
                {
                    //MmlPrintf("Dos command parameter is too long\r\n");
                    return ERROR;
                }
                cmd_para[para_num++] = strPtr[i++];
            }
            else
            {
                strPtr[i] = '\0';
            }
        }
        memcpy(&dosCmd.Para[dosCmd.ParaNum++], cmd_para, para_num);
        para_num = 0;
        if(dosCmd.ParaNum >= MML_PARA_NUM)
        {
            //MmlPrintf("Dos command parameter num exceed MML_PARA_NUM\r\n");
            return ERROR;
        }
    }
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::AnalyseMmlCmd

Description:
 This function analyses mml command

Input Value:
    strPtr : char*          ---- buffer for saving character string 
    mmlCmd : MmlCmdType&    ---- mml command structure
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::AnalyseMmlCmd(char* strPtr, MmlCmdType& mmlCmd)
{
    int len;
 
    char cmd_para[256];
    int maohao_num=0, cmdpara_num=0, cmd_num=0;
    int i=0, para_num=0;
    int j=0, str_len=0;

    len = strlen(strPtr);
    
    /*����MML����*/
    if(':' == strPtr[i])
    {
        i++;
        maohao_num++;
    }
    else
    {
        //MmlPrintf("illegal mml command\r\n");
        return ERROR;
    }
    while( '\0' != strPtr[i])
    {
        switch(strPtr[i])
        {
            /*add by yangj for 8144 MML CMD 2009-0402*/
            //:8144-MML-CMD:STRING="SET-MML-TEST��PARA=0;";
                case '"':
                {
                    if((maohao_num != 2)||(strPtr[i-1]!=':'))
                    {
                        //��δ����MML����
                        return ERROR;
                    }
                    str_len = strlen(strPtr);
                    if((str_len >= (MML_VALUE_LEN+i)))
                    {
                        return ERROR;
                    }
                    if(strPtr[str_len-2]!='"')
                    {
                        return ERROR;
                    }
                    int j = 0;
                    mmlCmd.ParaNum = 1;
                    memcpy(mmlCmd.CmdPara[0].Para,"STRING",7);
                    for(i=i+1; i<(str_len-2); i++)
                    {
                        mmlCmd.CmdPara[0].Value[j]=strPtr[i];
                        j++;
                    }
                    return OK;
                }
                break;

            /*end add by yangj for 8144 MML CMD 2009-0402*/
            case ':':
                /*����ð�Ÿ���������ֵ*/
                if(maohao_num >= 2 || 0 == cmdpara_num|| cmdpara_num >= MML_NAME_LEN)
                {
                    //MmlPrintf("illegal mml command\r\n");
                    return ERROR;
                }
                memcpy(mmlCmd.CmdName[cmd_num++].Cmd, cmd_para, cmdpara_num);
                cmdpara_num = 0;
                maohao_num++;
                break;
            case '-':
                /*�������Ÿ���������ֵ*/
                if(0 == cmdpara_num || cmd_num >= 2 || cmdpara_num >= MML_NAME_LEN)
                {
                    //MmlPrintf("illegal mml command\r\n");
                    return ERROR;
                }
                memcpy(mmlCmd.CmdName[cmd_num++].Cmd, cmd_para, cmdpara_num);
                cmdpara_num = 0;
                break;
            case '=':
                /*�����ȺŸ���������ֵ*/
                if(0 == cmdpara_num || cmdpara_num >= MML_NAME_LEN)
                {
                    //MmlPrintf("illegal mml command\r\n");
                    return ERROR;
                }
                memcpy(mmlCmd.CmdPara[para_num].Para, cmd_para, cmdpara_num);
                cmdpara_num = 0;
                break;
            case ',':
                /*�������Ÿ�����ֵ��ֵ*/
                if(0 == cmdpara_num || cmdpara_num >= MML_VALUE_LEN || para_num >= (MML_PARA_NUM-1))
                {
                    //MmlPrintf("illegal mml command\r\n");
                    return ERROR;
                }
                memcpy(mmlCmd.CmdPara[para_num++].Value, cmd_para, cmdpara_num);
                cmdpara_num = 0;
                break;
            case ';':
                /*�����в�������������������ֵ*/
                if(1 == maohao_num)
                {
                    if(cmdpara_num > MML_NAME_LEN)
                    {
                        //MmlPrintf("command name len exceed MML_NAME_LEN\r\n");
                        return ERROR;
                    }
                    memcpy(mmlCmd.CmdName[cmd_num++].Cmd, cmd_para, cmdpara_num);
                }
                /*�����д�������������ֵ��ֵ*/
                if(2 == maohao_num)
                {
                    if(cmdpara_num > MML_VALUE_LEN)
                    {
                        //MmlPrintf("parameter value len exceed MML_NAME_LEN\r\n");
                        return ERROR;
                    }
                    memcpy(mmlCmd.CmdPara[para_num++].Value, cmd_para, cmdpara_num);
                    mmlCmd.ParaNum = para_num;
                }
                return OK;
                break;
            default:
                /*��������������ַ�*/
                if( 1 == IsNormalChar(strPtr[i]))
                {
                    if( cmdpara_num >= 256 )
                    {
                        return ERROR;
                    }
                    cmd_para[cmdpara_num++] = strPtr[i];
                }
                else
                {
                    // MmlPrintf("illegal mml command, abnormal char\r\n");
                    return ERROR;
                }
                break;
        }
        i++;
    }
    /*   MmlPrintf("illegal mml command\r\n");*/
    return ERROR;
}

/*==============================================================================
Function:   DataCollectTool::ExecuteMmlCmd

Description:
 This function executes Mml command 

Input Value:
    mmlCmd : const MmlCmdType&      ---- mml command structure
    sockId: int                     ---- mml socket ID
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::ExecuteMmlCmd(const MmlCmdType& mmlCmd, int indexId)
{
    int num;
    int j;
    struct MmlMsg* msg_ptr = NULL;

    
    
    /*����MML����*/
    for(j = 0; j < mMmlCmdPtr->TotalNum; j++)
    {
        if( 0 != strcmp(mMmlCmdPtr->CmdItem[j].CmdName[0].Cmd,
            mmlCmd.CmdName[0].Cmd))
        {
            continue;
        }
        if( 0 != strcmp(mMmlCmdPtr->CmdItem[j].CmdName[1].Cmd,
            mmlCmd.CmdName[1].Cmd))
        {
            continue;
        }
        if( 0 != strcmp(mMmlCmdPtr->CmdItem[j].CmdName[2].Cmd,
            mmlCmd.CmdName[2].Cmd))
        {
            continue;
        }
        break;
    }
    if(j >= mMmlCmdPtr->TotalNum)
    {
        SuperMmlPrintf(indexId, "MML command was not installed\r\n");
        return ERROR;
    }
    /*�õ�ע���*/
    num = mMmlCmdPtr->CmdItem[j].RegisterNum;

    if((num < 0)||(num >= MAX_MML_USER))
    {
        SuperMmlPrintf(indexId, "register number %d error\r\n",num);
        return ERROR;
    }
    if(NULL == mMmlCallBack[num].FnPtr)
    {
        SuperMmlPrintf(indexId, "callback function is NULL\r\n");
        return ERROR;
    }

    if(mMmlCallBack[num].MsgBufNum >= 4)
    {
        SuperMmlPrintf(indexId, "callback's buffer already full\r\n");
        return ERROR;
    }
    if( NULL == (msg_ptr = (MmlMsg*)GetDataBuf(mMsgBuf)))
    {
        SuperMmlPrintf(indexId, "Mml message's buffer already full\r\n");
        return ERROR;
    }
    memset(msg_ptr, 0x00, MML_MSGBUF_SIZE+2);
    mMmlCallBack[num].MsgBufNum++;
    msg_ptr->CmdIndex = j;
    msg_ptr->ParaNum = mmlCmd.ParaNum;
    msg_ptr->indexId = indexId;
    memcpy(msg_ptr->CmdPara, mmlCmd.CmdPara, sizeof(mmlCmd.CmdPara));

    /*����MML�����Ӧ�Ļص�����*/
   
    if(ERROR == mMmlCallBack[num].FnPtr(mMmlCallBack[num].ObjPtr, msg_ptr))
    {
        SuperMmlPrintf(indexId, "call callback function error\r\n");
        PutDataBuf((char*)msg_ptr, mMsgBuf);
        mMmlCallBack[num].MsgBufNum--;
        return ERROR;
    }
    return OK;
}
/*==============================================================================
Function:   DataCollectTool::ExecuteDosCmd

Description:
 This function executes dos command

Input Value:
    dosCmd : const DosCmdType&      ---- dos command structure
    sockId : int                    ---- mml socket ID
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::ExecuteDosCmd(const DosCmdType& dosCmd, int indexId)
{
    for(int i = 0; mDosCmd[i].NamePtr; i++)
    {
        if( 0 == strcmp(mDosCmd[i].NamePtr, dosCmd.Para[0].Cmd))
        {
            activeId = indexId;
            mDosCmd[i].FnPtr(&dosCmd);
            activeId = -1;
            return OK;
        }
    }   
    SuperMmlPrintf(indexId, "Dos command was not installed\r\n");   
    return ERROR;
}

/*==============================================================================
Function:   DataCollectTool::deleteconnection

Description:
 This function delete debug window or statistic window 

Input Value:
    dosCmdPtr : DosCmdType*     ---- dos command structre
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::deleteconnection(DosCmdType* dosCmdPtr)
{
     int index; 
    int k;
    char * szName[]=
    {
        "MML",
        "DEBUG",
        "STATISTIC",        
        0,
    };
   

    if( 3 != dosCmdPtr->ParaNum)
    {
        MmlPrintf("missing parameters\r\n");
    }    
    index = atoi(dosCmdPtr->Para[2].Cmd);
    for (k=0;szName[k];k++)
        {
            if (strcmp(dosCmdPtr->Para[1].Cmd,szName[k])==0) 
                break;
        }
    switch(k)
        {
        case 0:
            if((index < 0)||(index >= MAX_MML_MMLNUM))
                {
                    MmlPrintf("DOS command error in MML connection number\r\n");
                    return ERROR;
                }
            if(mMmlCtrl[index].SockId > 0)
            {
                MmlCloseSocket(mMmlCtrl[index].SockId);
                mMmlCtrl[index].SockId = 0;
                mMmlCtrl[index].Status = IDLE;
                mMmlCtrl[index].WinId = 0;
                currentpos[index] = 0;
                indication[index] = 0;
                mMmlConnNum--;
            }
            else
            {
                MmlPrintf("DOS command error, MML window was not exist\r\n");
            }
            break;
        case 1:
            if((index < 0)||(index >= MAX_WINPTR_NUM))
            {
                MmlPrintf("DOS command error in DEBUG window number\r\n");
                return ERROR;
            }
            if((WinPtr[index].winNum < 0)||(WinPtr[index].winNum >= MAX_MML_WNDNUM))
            {
                MmlPrintf("DOS command error, DEBUG window was not exist\r\n");
                return ERROR;
            }
            MmlCloseSocket(mMmlCnfg[WinPtr[index].winNum].SockId);
            UserSockFail(WinPtr[index].winNum);
            break;
        case 2:
            if((index < 0)||(index >= MAX_STATPTR_NUM))
            {
                MmlPrintf("DOS command error in STATISTIC window number\r\n");
                return ERROR;
            }
            if((StatPtr[index].winNum < 0)||(StatPtr[index].winNum >= MAX_MML_STATNUM))
            {
                MmlPrintf("DOS command error, STATISTIC window was not exist\r\n");
                return ERROR;
            }
            MmlCloseSocket(mMmlStat[StatPtr[index].winNum].SockId);
            StatSockFail(StatPtr[index].winNum);            
            break;
     
        default:
            return 0;
        }
        return OK;
}

/*==============================================================================
Function:   int DataCollectTool::CaptureData()

Description:
    This function control capture data flag.
    1  ----  send data is complete
    0  ----  send data may be lost 

Input Value:
	
Output Value:
	None
Return Value:
	OK           success
Side Effect:
	None
==============================================================================*/
int DataCollectTool::CaptureData()
{
    if( g_CaptureDataFlag == 0 )
    {
      g_CaptureDataFlag = 1;
      MmlPrintf( "Capture data flag is 1, send data is complete!\r\n" );
    }
    else
    {
        g_CaptureDataFlag = 0;
        MmlPrintf( "Capture data flag is 0, send data may be lost!\r\n" );
    }

    return OK;    
}
/*==============================================================================
Function:   DataCollectTool::CmdCfgWin

Description:
 This function configs Mml monitor window 

Input Value:
    dosCmdPtr : DosCmdType*     ---- dos command structre
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
int DataCollectTool::CmdCfgWin(DosCmdType* dosCmdPtr)
{
    int index1, index2;
    int len;

    if( 4 != dosCmdPtr->ParaNum)
    {
        MmlPrintf("missing parameters\r\n");
        return ERROR;
    }
    index1 = atoi(dosCmdPtr->Para[1].Cmd);
    index2 = atoi(dosCmdPtr->Para[2].Cmd);

    if((index1 < 0)||(index1 >= MAX_MML_WNDNUM))
    {
        MmlPrintf("the connection number %d error\r\n",index1);
        return ERROR;
    }
    if((index2 < 0)||(index2 >= MAX_WINPTR_NUM))
    {
        MmlPrintf("the windows number %d error\r\n",index2);
        return ERROR;
    }

    /*���ں��Ѿ����ù�*/
    if( -1 != WinPtr[index2].winNum)
    {
        MmlPrintf("the debug windows %d already exist\r\n",index2);
        return ERROR;
    }
       
    if(CONNECT == mMmlCnfg[index1].Status)
    {       
        mMmlCnfg[index1].WinId = index2;
        WinPtr[index2].winNum = index1;
        WinPtr[index2].config = 1;
        mMmlCnfg[index1].Status = ACTIVE;
        memcpy(mMmlCnfg[index1].Prompt, dosCmdPtr->Para[3].Cmd, MML_NAME_LEN);
        len = strlen(mMmlCnfg[index1].Prompt);
        mMmlCnfg[index1].Prompt[len] = '>';

        SendToTelnet(index1, "\r\n");
        mMmlCnfg[index1].kit_telnet_connect = 2;
        /*���ȡMML_NAME_LEN���ַ�*/
 
        MmlPrintf("config_win SUCCESS\r\n");
        return OK;
            
        
    }
    else
    {
        if(ACTIVE == mMmlCnfg[index1].Status)
            MmlPrintf("connection % d already exist, please choose another connection numer\r\n", index1);
        else if(IDLE == mMmlCnfg[index1].Status)
            MmlPrintf("connection %d in IDLE \r\n",index1);
        else if(RELEASE == mMmlCnfg[index1].Status)
            MmlPrintf("connection %d in RELEASE\r\n",index1);
        else if(PAUSE == mMmlCnfg[index1].Status )
            MmlPrintf("connction %d in PAUSE\r\n",index1);
        return ERROR;
    }
}
 
/*==============================================================================
Function:   DataCollectTool::CmdShowDebugWin

Description:
 This function shows mml monitor window information 

Input Value:
    None
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdShowDebugWin()
{
    char* status;
    int i;

    MmlPrintf("=======================================\r\n");
    for( i = 0; i<MAX_MML_MMLNUM; i++)
        MmlPrintf(" mMmlCtrl.SockId = %d       IP  %s\r\n", mMmlCtrl[i].SockId,mMmlCtrl[i].IpAddr);
    MmlPrintf(" ctrlPipeId = %d\r\n", ctrlPipeId);
    MmlPrintf("the following is debug windows\r\n");
    MmlPrintf(" index   winId   Status  Prompt  Period(ms)  IP\r\n");
    MmlPrintf("---------------------------------------\r\n");
    for(i = 0; i < MAX_MML_WNDNUM; i++)
    {
        switch(mMmlCnfg[i].Status)
        {
        case 0:
            status = "IDLE";
            break;
        case 1:
            status = "CONNECT";
            break;
        case 2:
            status = "ACTIVE";
            break;
        case 3:
            status = "PAUSE";
            break;
        case 4:
            status = "RELEASE";
            break;
        default:
            status = "unknow status";
            break;
        }
        MmlPrintf(" %d  %d  %s  %s  %d      %s\r\n",
            i, mMmlCnfg[i].WinId, status, mMmlCnfg[i].Prompt,
            mMmlCnfg[i].Period, mMmlCnfg[i].IpAddr);
    }
    MmlPrintf("---------------------------------------\r\n");
    MmlPrintf("the following is statistic windows\r\n");
    MmlPrintf(" index   winId   Status  Prompt  Period(ms)  IP\r\n");
    MmlPrintf("---------------------------------------\r\n");
    for(i = 0; i < MAX_MML_STATNUM; i++)
    {
        switch(mMmlStat[i].Status)
        {
        case 0:
            status = "IDLE";
            break;
        case 1:
            status = "CONNECT";
            break;
        case 2:
            status = "ACTIVE";
            break;
        case 3:
            status = "PAUSE";
            break;
        case 4:
            status = "RELEASE";
            break;
        default:
            status = "unknow status";
            break;
        }
        MmlPrintf(" %d  %d  %s  %s  %d      %s\r\n",
            i, mMmlStat[i].WinId, status, mMmlStat[i].Prompt,
            mMmlStat[i].Period, mMmlStat[i].IpAddr);
    }    
    MmlPrintf("=======================================\r\n");

}
/*==============================================================================
Function:   DataCollectTool::CmdPauseWin

Description:
 This function stops printing data collect information 

Input Value:
    dosCmdPtr : DosCmdType*     ---- dos command structre
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdPauseWin(DosCmdType* dosCmdPtr)
{
    int win_id;

    if( 2 != dosCmdPtr->ParaNum)
    {
        MmlPrintf("missing parameters\r\n");
    }
    win_id = atoi(dosCmdPtr->Para[1].Cmd);
    DataCollectTool::MmlPause(win_id);
}
/*==============================================================================
Function:   DataCollectTool::CmdResumeWin

Description:
 This function restarts to print data collect information 

Input Value:
    dosCmdPtr : DosCmdType*     ---- dos command structre
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdResumeWin(DosCmdType* dosCmdPtr)
{
    int win_id;
    
    if( 2 != dosCmdPtr->ParaNum)
    {
        MmlPrintf("missing parameters\r\n");
    }
    win_id = atoi(dosCmdPtr->Para[1].Cmd);
    DataCollectTool::MmlResume(win_id);
}
/*==============================================================================
Function:   DataCollectTool::CmdTimerWin

Description:
 This function prints data collect information periodically 

Input Value:
    dosCmdPtr : DosCmdType*     ---- dos command structre
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdTimerWin(DosCmdType* dosCmdPtr)
{
    int win_id, internal;

    if( 3 != dosCmdPtr->ParaNum)
    {
        MmlPrintf("missing parameters\r\n");
    }
    win_id = atoi(dosCmdPtr->Para[1].Cmd);
    internal = atoi(dosCmdPtr->Para[2].Cmd);
    DataCollectTool::MmlTimer(win_id, internal);
}
/*==============================================================================
Function:   DataCollectTool::CmdReboot

Description:
 This function reboots the system 

Input Value:
    None
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdReboot()
{
#ifndef WIN32
    reboot(0);
#endif
}

/*==============================================================================
Function:   DataCollectTool::CmdHelp

Description:
 This function displays dos command information 

Input Value:
    None
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdHelp()
{
    char* description;
    char* usage;
    MmlPrintf("=======================================\r\n");
    MmlPrintf("Name\t\tDescription\r\n");
    MmlPrintf("---------------------------------------\r\n");
    for(int i = 0; mDosCmd[i].NamePtr; i++)
    {
        switch(i)
        {
        case 0:
            description = "Display mml debug window information";
            usage = "Usage : show_debugwin";
            break;
        case 1:
            description = "Config mml debug window";
            usage = "Usage : config_win index winId prompt";
            break;
        case 2:
            description = "Restart to print data collect information";
            usage = "resume_win winId";
            break;
        case 3:
            description = "Stop printing data collect information";
            usage = "Usage : pause_win winId";
            break;
        case 4:
            description = "Print data collect information periodically";
            usage = "Usage : timer_win winId period";
            break;
        case 5:
            description = "Reboot the system";
            usage = "Usage : reboot";
            break;
        case 6:
            description = "Display dos command help information";
            usage = "Usage : help";
            break;
        case 7:
            description = "Close mml command window";
            usage = "Usage : quit";
            break;
        case 8:
            description = "Print data collect buf information";
            usage = "Usage : printdatabuf";
            break;
        case 9:
            description = "delete MML/DEBUG/STATISTIC connection";
            usage = "Usage : delete_connect MML/DEBUG/STATISTIC index";
            break;
        default:
            description = "Illegal command";
            usage = "Usage : not known";
            break;
        }
        if( 5 == i || 6 == i || 7 == i)
        {
            MmlPrintf("%s\t\t%s\r\n", mDosCmd[i].NamePtr, description);
        }
        else
        {
            MmlPrintf("%s\t%s\r\n", mDosCmd[i].NamePtr, description);
        }
        MmlPrintf("%s\r\n", usage);
    }
    MmlPrintf("=======================================\r\n");
}
/*==============================================================================
Function:   DataCollectTool::CmdQuit

Description:
 This function closes the mml command window 

Input Value:
    None
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::CmdQuit()
{
    
    if(mMmlCtrl[activeId].SockId > 0)
    {
        MmlCloseSocket(mMmlCtrl[activeId].SockId);
        mMmlCtrl[activeId].SockId = 0;
        mMmlCtrl[activeId].WinId = 0;
        mMmlCtrl[activeId].Status = IDLE;
        currentpos[activeId] = 0;
        indication[activeId] = 0;
        mMmlConnNum--;      
    }
    return; 
}
/*==============================================================================
Function:   DataCollectTool::MmlTimeout

Description:
 This function is the callback function when timeout 

Input Value:
    winId : int     ---- debug windows ID
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::MmlTimeout(int winId)
{
    MmlCtrlOrd ctrl_ord;
   
    ctrl_ord.OrdCode = TOTIMEOUT;
    ctrl_ord.OrdMsg.WinId = winId;
    return SendToPipe(&ctrl_ord);
}

/*==============================================================================
Function:   DataCollectTool::UserSockFail

Description:
 This function executes when the telnet connection is cut 

Input Value:
    i : int     ---- debug connection ID
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::UserSockFail(int i)
{
    int msg_num = 0;
    if((mMmlCnfg[i].WinId >= 0)&&(mMmlCnfg[i].WinId < MAX_WINPTR_NUM))
    {
        WinPtr[mMmlCnfg[i].WinId].winNum = -1;
        WinPtr[mMmlCnfg[i].WinId].config = 0;
    }
    mMmlCnfg[i].SockId = 0;
    mMmlCnfg[i].WinId = -1;
    mMmlCnfg[i].kit_telnet_connect = 0;
    
    
    switch(mMmlCnfg[i].Status)
    {
        case ACTIVE:
        case PAUSE:         
            ioctl(mMmlCnfg[i].PipeId, FIONMSGS, (int)&msg_num);
            if( 0 == msg_num)
            {
                mMmlCnfg[i].Status = IDLE;
                break;
            }
            if( 0 != mMmlCnfg[i].Period)
            {
                wdCancel(mMmlCnfg[i].WdTimer);
                mMmlCnfg[i].Period = 0;
            }
            mMmlCnfg[i].Status = RELEASE;
            break;
        case CONNECT:
            mMmlCnfg[i].Status = IDLE;
            break;
        default:
            break;
    }
    mWndConnNum--;
    strcpy(mMmlCnfg[i].Prompt, "DEBUG");
    memset(mMmlCnfg[i].IpAddr, 0x00, sizeof(mMmlCnfg[i].IpAddr));
    return;
}

/*==============================================================================
Function:   DataCollectTool::StatSockFail

Description:
 This function executes when the telnet connection is cut 

Input Value:
    i : int     ---- statistic connection ID
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::StatSockFail(int i)
{
    int msg_num;

    /*�ͷ�ͳ������ռ�õ��ڴ���Դ*/
    ioctl(mMmlStat[i].PipeId, FIONMSGS, (int)&msg_num);
    for(int j = 0; j < msg_num; j++)
    {
        char* ptr;
        if(MML_POINTER_SIZE != reads(mMmlStat[i].PipeId, (char*)&ptr, MML_POINTER_SIZE))
        {
            return;
        }

        PutDataBuf((char*)ptr, mStatBuf);
    }
    if((mMmlStat[i].WinId > 0)&&(mMmlStat[i].WinId < MAX_STATPTR_NUM))
        StatPtr[mMmlStat[i].WinId].winNum = -1;
    mMmlStat[i].SockId=0;
//  mMmlStat[i].connect = 0;
    mMmlStat[i].WinId = -1;
    mMmlStat[i].Status = IDLE;
    mStatConnNum--;
    memset(mMmlStat[i].IpAddr, 0x00, sizeof(mMmlStat[i].IpAddr));
    return;
}
/*==============================================================================
Function:   DataCollectTool::DealMmlCtrlOrd

Description:
 This function deals mml contrl order 

Input Value:
    ordPtr : MmlCtrlOrd*    ---- pointer to mml command structure
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::DealMmlCtrlOrd(MmlCtrlOrd* ordPtr)
{
    int i, j, msg_num=0;
    char tempbuf[17];
     
    switch( ordPtr->OrdCode )
    {
        /*����һ��MML����*/
        case TOADDMML:
            for(i = 0; i < MAX_MML_MMLNUM; i++)
            {
                if( (0 == mMmlCtrl[i].SockId)&&(ordPtr->OrdMsg.ConSockId > 0) )
                {
                    mMmlCtrl[i].SockId = ordPtr->OrdMsg.ConSockId;
                    strcpy(mMmlCtrl[i].IpAddr, (char*)ordPtr->OrdMsg.IpAddr);               
                    mMmlCtrl[i].Status = ACTIVE;
                    sprintf(mMmlCtrl[i].Prompt, "NETMML%d>",i);
                    mMmlCtrl[i].Prompt[9] = '\0';
                    sprintf(tempbuf,"\r\n%s",mMmlCtrl[i].Prompt);

                    send(mMmlCtrl[i].SockId, tempbuf,strlen(tempbuf),0);                                 
                    break;
                }               
            }
            break;
        /*����һ��ͳ������*/
        case TOADDSTAT:
            for(i=0;i<MAX_MML_STATNUM;i++)
            {
                if( 0 == mMmlStat[i].SockId)
                {
                    mMmlStat[i].SockId = ordPtr->OrdMsg.ConSockId;
                    strcpy(mMmlStat[i].IpAddr, (char*)ordPtr->OrdMsg.IpAddr);
                    mMmlStat[i].Status = ACTIVE;
                    break;
                }
            }
            break;
        /*����һ����������*/
        case TOADDDBG:
            for(i = 0; i < MAX_MML_WNDNUM; i++)
            {
                if( 0 == mMmlCnfg[i].SockId)
                {
                    mMmlCnfg[i].SockId = ordPtr->OrdMsg.ConSockId;
                    strcpy(mMmlCnfg[i].IpAddr, (char*)ordPtr->OrdMsg.IpAddr);
                    mMmlCnfg[i].Status = CONNECT;
  
                    break;
                }
            }
            break;
        /*�����Դ��ڼ���*/
        case TOACTIVE:
            for(i = 0; i < MAX_MML_WNDNUM; i++)
            {
                if(mMmlCnfg[i].WinId == ordPtr->OrdMsg.WinId
                    &&  PAUSE == mMmlCnfg[i].Status)
                {                   
                    wdCancel(mMmlCnfg[i].WdTimer);
                    mMmlCnfg[i].Period = 0;                 
                    mMmlCnfg[i].Status = ACTIVE;    
                    break;
                }   
            }
            break;
        /*��ͣ���Դ������*/
        case TOPAUSE:
            for(i = 0; i < MAX_MML_WNDNUM; i++)
            {
                if(mMmlCnfg[i].WinId == ordPtr->OrdMsg.WinId)
                {
                    if( PAUSE == mMmlCnfg[i].Status
                        &&  0 != mMmlCnfg[i].Period)
                    {
                        wdCancel(mMmlCnfg[i].WdTimer);
                        mMmlCnfg[i].Period = 0;
                    }
                    if( ACTIVE == mMmlCnfg[i].Status)
                    {
                        mMmlCnfg[i].Status = PAUSE;     
                    }      
                    break;
                }
            }
            break;
        /*��ʼ��ʱ�����������*/
        case TOTIMER:
            if( 0 == ordPtr->OrdMsg.Peroid)
            {
                break;
            }
            for(i = 0; i < MAX_MML_WNDNUM; i++)
            {               
                if(mMmlCnfg[i].WinId == ordPtr->OrdMsg.WinId)
                {
                    mMmlCnfg[i].Status = PAUSE;
                    mMmlCnfg[i].Period = ordPtr->OrdMsg.Peroid;
                    /*����Timer*/
                    wdStart(mMmlCnfg[i].WdTimer, ordPtr->OrdMsg.Peroid,
                        (FUNCPTR)MmlTimeout, mMmlCnfg[i].WinId);  
                }
            }
            break;
        /*��ʱʱ�䵽�������������*/
        case TOTIMEOUT:
            for(i = 0; i < MAX_MML_WNDNUM; i++)
            {
                if(mMmlCnfg[i].WinId == ordPtr->OrdMsg.WinId)
                {
                    /*�쿴ָ��pipe����Ϣ��������telnet���ߴ�ӡ*/
                    ioctl(mMmlCnfg[i].PipeId, FIONMSGS, (int)&msg_num);
                    /*     SendToTelnet(i, "------------Timeout begin---------\r\n");*/
                    for(j = 0; j < msg_num; j++)
                    {
                        char* ptr;
                        if(MML_POINTER_SIZE != reads(mMmlCnfg[i].PipeId, (char*)&ptr, MML_POINTER_SIZE))
                        {
                            return;         
                        }
                        SendToTelnet(i, ptr);
#if defined(SW_CPB)||defined(MCU_SW)
                        if( (ptr >= mBBDDataBuf.PhyMemPtr) 
                            && (ptr <= mBBDDataBuf.PhyMemPtr + MML_BBDDATABUF_LEN) )
                        {
                            PutDataBuf((char*)ptr,mBBDDataBuf);
                        }
                        else
#endif
                        {
                            PutDataBuf((char*)ptr,mDataBuf);
                        }
                    }
                    /*    SendToTelnet(i, "------------Timeout End-----------\r\n");*/
                    /*����������ʱ��*/
                    wdStart(mMmlCnfg[i].WdTimer, mMmlCnfg[i].Period, 
                        (FUNCPTR)MmlTimeout, mMmlCnfg[i].WinId);
                    break;
                }
            }
            break;
        default:
            break;
    }
}
/*==============================================================================
Function:   DataCollectTool::DealMmlCmd

Description:
 This function deals Mml command 

Input Value:
    i : int         ---- mml connection ID
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::DealMmlCmd(int i)
{
    int ret;
    char buffer[MML_CMD_LEN];
    char* cmd_ptr = buffer;

    memset(buffer, 0, sizeof(buffer));
    /*��MML�����Ͻ���һ������*/
    ret = GetOneLine(i);
    /*�������δ�����꣬�ȷ��أ������������Ϻ��ٴ���*/
    if( MML_NOVER == indication[i])
    {           
        return; 
    }
    /*���յ����������ʼ����*/
    if( ERROR != ret)
    {
        int tmp;
        DosCmdType dos_cmd;
        MmlCmdType mml_cmd;
        memset(&dos_cmd, 0x00, sizeof(dos_cmd));
        memset(&mml_cmd, 0x00, sizeof(mml_cmd));
        memcpy(buffer,gMmlDataBuf[i],MML_CMD_LEN);
        tmp = DelBlank(buffer);
        cmd_ptr = cmd_ptr + tmp;
        if(strlen(cmd_ptr) < 1)
        {
            return;
        }
        MyStrUpper((char*)cmd_ptr);
        /*���ж��Ƿ�ΪMML����*/
        if ( ':' == cmd_ptr[0])
        {
            /*��MML����Ƚ���MML����*/
            if( OK == AnalyseMmlCmd(cmd_ptr, mml_cmd))
            {
                /*ִ��MML����*/
                ExecuteMmlCmd(mml_cmd, i);          
            }
            else
            {
                SuperMmlPrintf(i,"illegal mml command\r\n");            
            }
        }
        /*Ϊע�ͣ�������*/
        else if ( '#' == cmd_ptr[0]) 
        {
        }
        /*ΪDOS����*/
        else
        {
            /*������DOS����*/
            if( OK == AnalyseDosCmd(cmd_ptr, dos_cmd))
            {
                /*ִ����DOS����*/
                ExecuteDosCmd(dos_cmd, i);
            }
            else
            {
                SuperMmlPrintf(i,"illegal dos command\r\n");
            }
        }
        return;
    }    
        return;
   
}
/*=============================================================================
Function:   DataCollectTool::DealStatSock

Description:
    This function deals statistic socket

Input Value:
    i : int         ---- statistic connection ID
Output Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::DealStatSock(int i)
{
    
    char buf1[6];
    int len;    
    int num=0;
    unsigned long buf3;

    memset(buf1, 0x00, sizeof(buf1)); 
    /*���յ����ӶϿ�*/
    if((len=recv(mMmlStat[i].SockId, buf1, sizeof(buf1),0)) <= 0)
    {               
        MmlCloseSocket(mMmlStat[i].SockId);
        StatSockFail(i);        
        return; 
    }

    /*����ͳ�ƴ��ں�*/ 
    if( 0 == mMmlStat[i].connect)
    {
        if(( 6 == len)&&( 0 == *(short*)buf1))
        {
            buf3 = *(unsigned long*)(buf1+2);
            buf3 = ntohl(buf3);
            num = (int)buf3;
            if((num < 0 )||(num > MAX_STATPTR_NUM))
            {           
                SuperMmlPrintf(MAX_MML_MMLNUM,"the statistic windows number %d error\r\n",num);         
                MmlCloseSocket(mMmlStat[i].SockId);
                StatSockFail(i);
                return;
            }
            /*�ô��ں��Ѿ�����*/
            if((StatPtr[num].winNum >= 0)&&(StatPtr[num].winNum < MAX_MML_STATNUM))
            {
                SuperMmlPrintf(MAX_MML_MMLNUM,"statistic window number %d already exist\r\n",num);              
                MmlCloseSocket(mMmlStat[i].SockId);
                StatSockFail(i);      
                return;
            }
            for(int j=0; j < STAT_NAME_NUM; j++)
            {
                if( 1 == StatPtr[num].mStatName[j].flag)
                {
                    if(mMmlStat[i].SockId > 0)
                    {
                        send(mMmlStat[i].SockId, StatPtr[num].mStatName[j].buf,65,0);                       
                    }
                    else 
                        return ;
                }
            }
        
            mMmlStat[i].WinId = num;
            StatPtr[num].winNum = i;
        //  mMmlStat[i].connect = 1;                
            return;         
        }
    }
    else
    {
        return;
    }
}

/*=============================================================================
Function:   DataCollectTool::DealStatPipe

Description:
    This function deals statistic pipe

Input Value:
     i : int        ---- statistic pipe ID
Output Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::DealStatPipe(int i)
{
    int msg_num = 0;
    
    ioctl(mMmlStat[i].PipeId, FIONMSGS, (int)&msg_num);
    for(int j = 0; j < msg_num; j++)
    {
        char* ptr;
        if(MML_POINTER_SIZE != reads(mMmlStat[i].PipeId, (char*)&ptr, MML_POINTER_SIZE))
        {
            return;
        }
        if(mMmlStat[i].SockId > 0)
        {
            send(mMmlStat[i].SockId, ptr+2, MML_STATBUF_SIZE, 0);           
            PutDataBuf((char*)ptr, mStatBuf);
        }
    }
    return;
}

/*==============================================================================
Function:   DataCollectTool::DealWatchSock

Description:
 This function deals user socket for debug connection

Input Value:
    i : int         ---- debug connection ID
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::DealWatchSock(int i)
{
    char buf1[6];
    int len;
    int num;
    unsigned long buf3;

    memset(buf1, 0x00, sizeof(buf1));
    /*���յ����ӶϿ�*/
    if((len=recv(mMmlCnfg[i].SockId, buf1, sizeof(buf1),0)) <= 0)
    {               
        MmlCloseSocket(mMmlCnfg[i].SockId);
        UserSockFail(i); 
        return; //wangli
    }
    /*���õ��Դ��ں�*/
    if( 0 == mMmlCnfg[i].kit_telnet_connect)
    {
        if(( 6 == len)&&( 0 == *(short*)buf1))
        {
            buf3 = *(unsigned long*)(buf1+2);
            buf3 = ntohl(buf3);
            num = (int)buf3;

            /*�߽���*/
            if((num < 0)||(num >= MAX_WINPTR_NUM))
            {
                SuperMmlPrintf(MAX_MML_MMLNUM,"the debug window number %d error\r\n",num);
                MmlCloseSocket(mMmlCnfg[i].SockId);
                UserSockFail(i);
                return;
            }
            /*�Ѿ����ڸô��ں�*/
            if( -1 != WinPtr[num].winNum )
            {
                SuperMmlPrintf(MAX_MML_MMLNUM,"debug window number %d aready exist\r\n",num);                               
                MmlCloseSocket(mMmlCnfg[i].SockId);             
                UserSockFail(i);    
                return;
            }
            /*���ô��ں�*/
            mMmlCnfg[i].WinId = num;
            WinPtr[num].winNum = i;
            mMmlCnfg[i].Status = ACTIVE;
            mMmlCnfg[i].kit_telnet_connect = 1;
        
            return;
            
        }
    }
    else
    {
        return;
    }
}
/*==============================================================================
Function:   DataCollectTool::DealWatchPipe

Description:
 This function deals user pipe 

Input Value:
    i : int     ---- debug pipe ID
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::DealWatchPipe(int i)
{
    int msg_num = 0;
    
    ioctl(mMmlCnfg[i].PipeId, FIONMSGS, (int)&msg_num);
    for(int j = 0; j < msg_num; j++)
    {
        char* ptr;
        if(MML_POINTER_SIZE != reads(mMmlCnfg[i].PipeId, (char*)&ptr, MML_POINTER_SIZE))
        {
            return;
        }
        if( RELEASE != mMmlCnfg[i].Status)
        {
            SendToTelnet(i, ptr);
        }

#if defined(SW_CPB)||defined(MCU_SW)
        if( (ptr >= mBBDDataBuf.PhyMemPtr) 
            && (ptr <= (mBBDDataBuf.PhyMemPtr + MML_BBDDATABUF_LEN)) )
        {
            PutDataBuf((char*)ptr,mBBDDataBuf);
        }
        else
#endif
        {
            PutDataBuf((char*)ptr,mDataBuf);
        }
    }
    /*RELEASE״̬,���û�����ݽ���IDLE״̬*/
    if( RELEASE == mMmlCnfg[i].Status)
    {
        mMmlCnfg[i].Status = IDLE;
    }
}

/*==============================================================================
Function:   DataCollectTool::ListenTask

Description:
 This function is the TCP listen task function 

Input Value:
    None
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::ListenTask()
{
    int con_sockid;
    int temp = sizeof(struct sockaddr);
    fd_set read_fd;
    struct sockaddr con_addr;
    MmlCtrlOrd ctrl_ord;

#ifdef WIN32
    static u_long flag = 1;    
#else
    static int  flag = 1;
#endif   
    
    /*��������MML���ӵ�socket*/
    tty_sockid = socket(AF_INET, SOCK_STREAM, 0);
    if( ERROR == tty_sockid)
    {
        printf("open tty Socket error\n");
        return ERROR;
    }
    if( OK != MmlBind(tty_sockid, MML_PORT))
    {
        MmlCloseSocket(tty_sockid);
        printf("bind TTY Socket error\n");
        return ERROR;
    }
    if( OK != listen(tty_sockid, 3))
    {
        MmlCloseSocket(tty_sockid);
        printf("listen tty Socket error\n");
        return ERROR;
    }

    /*��������ͳ�����ӵ�socket*/
    stat_sockid = socket(AF_INET, SOCK_STREAM,0);
    if( ERROR == stat_sockid)
    {
        printf("open statistic Socket error\n");
        return ERROR;
    }
    if( OK != MmlBind(stat_sockid, STAT_PORT))
    {
        MmlCloseSocket(stat_sockid);
        printf("bind statistic Socket error\n");
        return ERROR;
    }
    if( OK != listen(stat_sockid, 3))
    {
        MmlCloseSocket(stat_sockid);
        printf("listen statistic Socket error\n");
        return ERROR;
    }

    /*���������������ӵ�socket*/
    watch_sockid = socket(AF_INET, SOCK_STREAM,0);
    if( ERROR == watch_sockid)
    {
        printf("open watch Socket error\n");
        return ERROR;
    }
    if( OK != MmlBind(watch_sockid, DBG_PORT))
    {
        MmlCloseSocket(watch_sockid);
        printf("bind watch Socket error\n");
        return ERROR;
    }
    if( OK != listen(watch_sockid, 3))
    {
        MmlCloseSocket(watch_sockid);
        printf("listen watch Socket error\n");
        return ERROR;
    }
    
    for(;;)
    {
        /*�Ѿ�������뵽select����*/
        FD_ZERO(&read_fd);
        FD_SET((unsigned int)tty_sockid, &read_fd);
        FD_SET((unsigned int)watch_sockid, &read_fd);
        FD_SET((unsigned int)stat_sockid, &read_fd);
    
        if( ERROR == select(FD_SETSIZE, &read_fd, NULL, NULL, NULL))
            break;
    
        if(FD_ISSET(tty_sockid, &read_fd))
        {           
            con_sockid = accept(tty_sockid, &con_addr, &temp);
            if( -1 == con_sockid)
            {
#ifdef DATACOLLECT_DEBUG
            #ifdef WIN32
                int test2=WSAGetLastError();
                printf("the failed reason is %d\r\n",test2);
            #endif
#endif
                continue;
            }
            /*������socket����*/
#ifdef WIN32
            ioctlsocket(con_sockid,FIONBIO,&flag);
#else
            ioctl(con_sockid,FIONBIO,(int)&flag);
#endif
                
            if(mMmlConnNum >= MAX_MML_MMLNUM)
            {
                MmlCloseSocket(con_sockid);
            }
            else
            {
                ctrl_ord.OrdCode = TOADDMML;
                ctrl_ord.OrdMsg.ConSockId = con_sockid;
                sprintf((char*)ctrl_ord.OrdMsg.IpAddr, "%d.%d.%d.%d",
                    (unsigned char)con_addr.sa_data[2], (unsigned char)con_addr.sa_data[3],
                    (unsigned char)con_addr.sa_data[4], (unsigned char)con_addr.sa_data[5]);
                /*���������MML����*/
                SendToPipe(&ctrl_ord);
                mMmlConnNum++;
            }
        }


        if(FD_ISSET(stat_sockid, &read_fd))
        {
            con_sockid = accept(stat_sockid, &con_addr, &temp);
            if( -1 == con_sockid)
            {
#ifdef DATACOLLECT_DEBUG
            #ifdef WIN32
                int test2=WSAGetLastError();
                printf("the failed reason is %d\r\n",test2);
            #endif
#endif
               continue;
            }
            /*������socket����*/
#ifdef WIN32
            ioctlsocket(con_sockid,FIONBIO,&flag);
#else
            ioctl(con_sockid,FIONBIO,(int)&flag);
#endif
            if(mStatConnNum >= MAX_MML_STATNUM)
            {
                MmlCloseSocket(con_sockid);
            }
            else
            {               
                ctrl_ord.OrdCode = TOADDSTAT;
                ctrl_ord.OrdMsg.ConSockId = con_sockid;
                sprintf((char*)ctrl_ord.OrdMsg.IpAddr, "%d.%d.%d.%d",
                    (unsigned char)con_addr.sa_data[2], (unsigned char)con_addr.sa_data[3],
                    (unsigned char)con_addr.sa_data[4], (unsigned char)con_addr.sa_data[5]);
                /*���������ͳ������*/
                SendToPipe(&ctrl_ord);
                mStatConnNum++;
            }
        }


        if(FD_ISSET(watch_sockid, &read_fd))
        {
            con_sockid = accept(watch_sockid, &con_addr, &temp);
            if( -1 == con_sockid)
            {
#ifdef DATACOLLECT_DEBUG
        #ifdef WIN32
                int test2=WSAGetLastError();
                printf("the failed reason is %d\r\n",test2);
        #endif
#endif
                continue;
            }
            /*������socket����*/
#ifdef WIN32
            ioctlsocket(con_sockid,FIONBIO,&flag);
#else
            ioctl(con_sockid,FIONBIO,(int)&flag);
#endif
            if(mWndConnNum >= MAX_MML_WNDNUM)
            {
                MmlCloseSocket(con_sockid);
            }
            else
            {
                ctrl_ord.OrdCode = TOADDDBG;
                ctrl_ord.OrdMsg.ConSockId = con_sockid;
                sprintf((char*)ctrl_ord.OrdMsg.IpAddr, "%d.%d.%d.%d",
                    (unsigned char)con_addr.sa_data[2], (unsigned char)con_addr.sa_data[3],
                    (unsigned char)con_addr.sa_data[4], (unsigned char)con_addr.sa_data[5]);
                /*�����������������*/
                SendToPipe(&ctrl_ord);
                mWndConnNum++;
            }
        }
    }
    FD_ZERO(&read_fd);
    MmlCloseSocket(tty_sockid);
    MmlCloseSocket(watch_sockid);
    MmlCloseSocket(stat_sockid); //wangli
                                //wangli �Ƿ���Ҫ����MmlExit�ͷ������Դ��ע�ⲻ��ֱ�ӵ���
    return ERROR;
}

/*==============================================================================
Function:   DataCollectTool::DataCollectTask

Description:
 This function is the data collect task 

Input Value:
    appObjPtr : DataCollectTool*    ---- pointer to class object
Output Value:
    None
Return Value:
    OK           success
    ERROR        fail
Side Effect:
    None
==============================================================================*/
STATUS DataCollectTool::DataCollectTask(DataCollectTool* appObjPtr)
{
    int i, recv_len;
    fd_set read_fd;
    MmlCtrlOrd ctrl_ord;

    FOREVER
    {
        FD_ZERO(&read_fd);

        /*��pipe��socket����SELECT��*/
       
        for(i = 0; i < MAX_MML_MMLNUM; i++)
        {
            if( ACTIVE == mMmlCtrl[i].Status)
                 FD_SET((unsigned int)mMmlCtrl[i].SockId, &read_fd);
        }

        FD_SET((unsigned int)ctrlPipeId, &read_fd);

        for(i=0;i<MAX_MML_STATNUM;i++)
        {
            if( ACTIVE == mMmlStat[i].Status)
            {
                FD_SET((unsigned int)mMmlStat[i].SockId, &read_fd);
                FD_SET((unsigned int)mMmlStat[i].PipeId, &read_fd);
            }
        }        
        
        for(i = 0; i < MAX_MML_WNDNUM; i++)
        {
            if( RELEASE == mMmlCnfg[i].Status)
            {
                FD_SET((unsigned int)mMmlCnfg[i].PipeId, &read_fd);
            }
            if( CONNECT == mMmlCnfg[i].Status ||  PAUSE == mMmlCnfg[i].Status)
            {
                FD_SET((unsigned int)mMmlCnfg[i].SockId, &read_fd);
            }
            if( ACTIVE == mMmlCnfg[i].Status)
            {
                FD_SET((unsigned int)mMmlCnfg[i].PipeId, &read_fd);
                FD_SET((unsigned int)mMmlCnfg[i].SockId, &read_fd);
            }
        }
    
        if( ERROR == select(FD_SETSIZE, &read_fd, NULL, NULL, NULL)) 
            break;
    
        /*����MML����*/
        for(i=0;i < MAX_MML_MMLNUM; i++)
        {
            if(FD_ISSET(mMmlCtrl[i].SockId, &read_fd))
            {
                appObjPtr->DealMmlCmd(i);
            }
        }

        /*�������pipe*/
        if(FD_ISSET(ctrlPipeId, &read_fd))
        {
            recv_len = reads(ctrlPipeId, (char*)&ctrl_ord, sizeof(ctrl_ord));
            if( sizeof(ctrl_ord) == recv_len)
            {
                appObjPtr->DealMmlCtrlOrd(&ctrl_ord);
            }
        }

        for(i=0;i<MAX_MML_STATNUM;i++)
        {
            /*����ͳ���û�socket*/
            if(FD_ISSET(mMmlStat[i].SockId, &read_fd))
            {
                appObjPtr->DealStatSock(i);
            }
            /*����ͳ���û�pipe*/
            if(FD_ISSET(mMmlStat[i].PipeId, &read_fd))
            {
                appObjPtr->DealStatPipe(i);
            }
        }

        for(i = 0; i < MAX_MML_WNDNUM; i++)
        {
            if( IDLE == mMmlCnfg[i].Status )
                continue;
            
            /*�����û�socket*/
            if(FD_ISSET(mMmlCnfg[i].SockId, &read_fd))
            {
                appObjPtr->DealWatchSock(i);
            }
            /*�����û�pipe*/
            if(FD_ISSET(mMmlCnfg[i].PipeId, &read_fd))
            {
                appObjPtr->DealWatchPipe(i);
            }
        }
    }
    return ERROR;
}
/*==============================================================================
Function:   DataCollectTool::printDataBuf

Description:
 This function prints data collect buf information 

Input Value:
    None  
Output Value:
    None
Return Value:
    None
Side Effect:
    None
==============================================================================*/
void DataCollectTool::PrintDataBuf()
{
#ifdef DATACOLLECT_DEBUG
    MmlPrintf("============databuf begin=============\r\n");
    MmlPrintf("databuf busy node number = %d\r\n", mDataBuf.BusyNum);
    MmlPrintf("databuf free node number = %d\r\n", mDataBuf.FreeNum);

#if defined(SW_CPB)||defined(MCU_SW)
    MmlPrintf("============BBD databuf begin=========\r\n");
    MmlPrintf("databuf busy node number = %d\r\n", mBBDDataBuf.BusyNum);
    MmlPrintf("databuf free node number = %d\r\n", mBBDDataBuf.FreeNum);
#endif

    MmlPrintf("============msgbuf begin==============\r\n");
    MmlPrintf("msgbuf busy node number = %d\r\n", mMsgBuf.BusyNum);
    MmlPrintf("msgbuf free node number = %d\r\n", mMsgBuf.FreeNum);

    MmlPrintf("============statbuf begin==============\r\n");
    MmlPrintf("statbuf busy node number = %d\r\n", mStatBuf.BusyNum);
    MmlPrintf("statbuf free node number = %d\r\n", mStatBuf.FreeNum);
#endif
}
