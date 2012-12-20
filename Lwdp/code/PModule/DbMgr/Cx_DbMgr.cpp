/*! \file Cx_DbMgr.cpp
 *  \brief DbMgr Module Impl
 *  \author Guolisen, LwDp
 *  \date   2012.10.17
 */
 
#include <LwDp.h>
#include <PluginInc.h>
#include <iostream>

#include <Interface/ConfigMgr/Ix_ConfigMgr.h>
#include <Interface/LogMgr/Ix_LogMgr.h>

#include "DbMgrDef.h"
#include "Cx_DbMgr.h"



LWDP_NAMESPACE_BEGIN;


Cx_DbMgr::Cx_DbMgr()
{
	mDb = NULL;
}

Cx_DbMgr::~Cx_DbMgr()
{
	if ( mDb != NULL )
	{
		Close();
	}
}

LWRESULT Cx_DbMgr::Init()
{
	lw_log_info(LWDP_LUA_LOG, __T("Cx_DbMgr::Init OK!"));



	return LWDP_OK;
}


LWRESULT Cx_DbMgr::Open(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db,
 					   unsigned int32_ port, unsigned long_ client_flag)
{
	mDb = mysql_init(NULL);
	if(NULL == mDb) 
		goto EXT;

	//�������ʧ�ܣ�����NULL�����ڳɹ������ӣ�����ֵ���1��������ֵ��ͬ��
	if (NULL == mysql_real_connect(mDb, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port.c_str(), NULL, client_flag))
		goto EXT;
	//ѡ���ƶ������ݿ�ʧ��
	//0��ʾ�ɹ�����0ֵ��ʾ���ִ���
	if (mysql_select_db(mDb, db) != 0) 
	{
		mysql_close(mDb);
		mDb = NULL;
		goto EXT;
	}

	return LWDP_OK;
EXT:
	//��ʼ��mysql�ṹʧ��
	if (mDb != NULL )
	{
		mysql_close( mDb );
		mDb = NULL;
	}
	
	return LWDP_OPEN_DB_ERROR;
}
void Cx_DbMgr::Close()
{
	if (mDb != NULL)
	{
		mysql_close(mDb);
		mDb = NULL;
	}
}

/* ���ؾ�� */
DBHandle Cx_DbMgr::GetDbHandle()
{
	return (DBHandle)mDb;
}

/* �����ض��еĲ�ѯ������Ӱ������� */
//������������Ϊ��CppMySQLQuery�ĸ�ֵ���캯����Ҫ�ѳ�Ա����_mysql_res��Ϊ��
//CppMySQLQuery& Cx_DbMgr::QuerySQL(const std::string& sql)
//{


//}


/* ִ�зǷ��ؽ����ѯ */
int32_ Cx_DbMgr::ExecSQL(const std::string& sql)
{
	int ret = 0;
	if((ret = mysql_real_query(mDb, sql.c_str(), sql.size())))
	{
		//ִ�в�ѯʧ��
		LWDP_LOG_PRINT("DbMgr", LWDP_LOG_MGR::ERR, 
					   "ExecSQL mysql_real_query(%s) ret Error(%x)", 
					   sql.c_str(), ret);
		return LWDP_ERROR;
	}

	//�õ���Ӱ�������
	return (int32_)mysql_affected_rows(mDb) ;
}

/* ����mysql�������Ƿ��� */
int32_ Cx_DbMgr::Ping()
{
	if(mysql_ping(mDb) == 0)
		return LWDP_OK;
	else 
		return LWDP_PING_DB_ERROR; 
}

/* �ر�mysql ������ */
int32_ Cx_DbMgr::ShutDown()
{
	if(mysql_shutdown(mDb, SHUTDOWN_DEFAULT) == 0)
		return LWDP_OK;
	else 
		return LWDP_SHUTDOWN_DB_ERROR;
}

/* ��Ҫ����:��������mysql ������ */
int32_ Cx_DbMgr::Reboot()
{
	if(!mysql_reload(mDb))
		return LWDP_OK;
	else
		return LWDP_REBOOT_DB_ERROR;
}


/*
* ˵��:����֧��InnoDB or BDB������
*/
/* ��Ҫ����:��ʼ���� */
int32_ Cx_DbMgr::StartTransaction()
{
	int ret = 0;
	if(ret = mysql_real_query(mDb, "START TRANSACTION" ,
						     (unsigned long)strlen("START TRANSACTION")))
	{
		LWDP_LOG_PRINT("DbMgr", LWDP_LOG_MGR::ERR, 
					   "mysql_real_query(START TRANSACTION) ret Error(%x)", 
					   ret);
		return ret;
	}

	return LWDP_OK;

}

/* ��Ҫ����:�ύ���� */
int32_ Cx_DbMgr::Commit()
{
	int ret = 0;
	if(ret = mysql_real_query(mDb, "COMMIT" ,
						     (unsigned long)strlen("COMMIT")))
	{
		LWDP_LOG_PRINT("DbMgr", LWDP_LOG_MGR::ERR, 
					   "mysql_real_query(COMMIT) ret Error(%x)", 
					   ret);
		return ret;
	}

	return LWDP_OK;
}

/* ��Ҫ����:�ع����� */
int32_ Cx_DbMgr::Rollback()
{
	int ret = 0;
	if(ret = mysql_real_query(mDb, "ROLLBACK" ,
						     (unsigned long)strlen("ROLLBACK")))
	{
		LWDP_LOG_PRINT("DbMgr", LWDP_LOG_MGR::ERR, 
					   "mysql_real_query(ROLLBACK) ret Error(%x)", 
					   ret);
		return ret;
	}

	return LWDP_OK;
}

/* �õ��ͻ���Ϣ */
const std::string Cx_DbMgr::GetClientInfo()
{
	return static_cast<const std::string>(mysql_get_client_info());
}

/* ��Ҫ����:�õ��ͻ��汾��Ϣ */
const unsigned long_ Cx_DbMgr::GetClientVersion()
{
	return static_cast<const std::string>(mysql_get_client_version());
}

/* ��Ҫ����:�õ�������Ϣ */
const std::string Cx_DbMgr::GetHostInfo()
{
	return static_cast<const std::string>(mysql_get_host_info(mDb));
}

/* ��Ҫ����:�õ���������Ϣ */
const std::string Cx_DbMgr::GetServerInfo()
{
	return static_cast<const std::string>(mysql_get_server_info(mDb));
}

/*��Ҫ����:�õ��������汾��Ϣ*/
const unsigned long_ Cx_DbMgr::GetServerVersion()
{
	return static_cast<const std::string>(mysql_get_server_version(mDb));
}

/*��Ҫ����:�õ� ��ǰ���ӵ�Ĭ���ַ���*/
const std::string Cx_DbMgr::GetCharacterSetName()
{
	return static_cast<const std::string>(mysql_character_set_name(mDb));
}

/* �õ�ϵͳʱ�� */
const std::string Cx_DbMgr::GetSysTime()
{
	//return ExecQueryGetSingValue("select now()");
	return NULL;
}

/* ���������ݿ� */
int32_ Cx_DbMgr::CreateDB(const std::string& name)
{
	std::string str;
	str = std::string("CREATE DATABASE ") + name;
	int ret = 0;
	if(ret = mysql_real_query(mDb, str.c_str(),
						     (unsigned long)str.size()))
	{
		LWDP_LOG_PRINT("DbMgr", LWDP_LOG_MGR::ERR, 
					   "mysql_real_query(%s) ret Error(%x)", 
					   str.c_str(), ret);
		return ret;
	}

	return LWDP_OK;
}

/* ɾ���ƶ������ݿ�*/
int32_ Cx_DbMgr::DropDB(const std::string& name)
{
	std::string str;
	str = std::string("DROP DATABASE ") + name;
	int ret = 0;
	if(ret = mysql_real_query(mDb, str.c_str(),
						     (unsigned long)str.size()))
	{
		LWDP_LOG_PRINT("DbMgr", LWDP_LOG_MGR::ERR, 
					   "mysql_real_query(%s) ret Error(%x)", 
					   str.c_str(), ret);
		return ret;
	}

	return LWDP_OK;
}

bool_ Cx_DbMgr::TableExists(const std::string& table)
{
	return false;
}

uint32_ Cx_DbMgr::LastRowId()
{
	return LWDP_OK;
}

void Cx_DbMgr::SetBusyTimeout(int32_ nMillisecs)
{


}




uint32_ Cx_DbMgr::NumRow()//������
{


}
int32_  Cx_DbMgr::NumFields()//������
{


}

int32_  Cx_DbMgr::FieldIndex(const std::string& szField)
{


}

//0...n-1��
const std::string Cx_DbMgr::FieldName(int32_ nCol)
{


}


uint32_ Cx_DbMgr::SeekRow(u_long offerset)
{


}

int32_  Cx_DbMgr::GetIntField(int32_ nField, int32_ nNullValue)
{


}

int32_  Cx_DbMgr::GetIntField(const std::string& szField, int32_ nNullValue)
{


}

double_ Cx_DbMgr::GetFloatField(int32_ nField, double_ fNullValue)
{


}

double_ Cx_DbMgr::GetFloatField(const std::string& szField, double_ fNullValue)
{


}

//0...n-1��
const std::string Cx_DbMgr::GetStringField(int32_ nField, const std::string& szNullValue)
{


}

const std::string Cx_DbMgr::GetStringField(const std::string& szField, const std::string& szNullValue)
{


}

bool Cx_DbMgr::FieldIsNull(int32_ nField)
{


}

bool Cx_DbMgr::GieldIsNull(const std::string& szField)
{


}

bool Cx_DbMgr::Eof()
{


}

void Cx_DbMgr::NextRow()
{


}

void Cx_DbMgr::Finalize()
{


}





LWDP_NAMESPACE_END;

