#ifndef DBMGR_INTERFACE_H
#define DBMGR_INTERFACE_H

#include <Interface/Ix_Object.h>
#include "Id_DbMgr.h"

LWDP_NAMESPACE_BEGIN;
 
typedef void* DBHandle;


INTERFACE_BEGIN(DbQuery)
 	virtual uint32_ NumRow() = 0;//������
    virtual int32_ NumFields() = 0;//������
    virtual int32_ FieldIndex(const std::string& szField) = 0;
 //0...n-1��
    virtual const std::string FieldName(int32_ nCol) = 0;
 //   const std::string fieldDeclType(int32_ nCol) = 0;
 //   int32_ fieldDataType(int32_ nCol) = 0;
 	virtual uint32_ SeekRow(uint32_ offerset) = 0;
    virtual int32_ GetIntField(int32_ nField, int32_ nNullValue) = 0;
    virtual int32_ GetIntField(const std::string& szField, int32_ nNullValue) = 0;
    virtual double_ GetFloatField(int32_ nField, double_ fNullValue) = 0;
    virtual double_ GetFloatField(const std::string& szField, double_ fNullValue) = 0;
 //0...n-1��
    virtual const std::string GetStringField(int32_ nField, const std::string& szNullValue) = 0;
    virtual const std::string GetStringField(const std::string& szField, const std::string& szNullValue) = 0;
    virtual bool FieldIsNull(int32_ nField) = 0;
    virtual bool GieldIsNull(const std::string& szField) = 0;
    virtual bool Eof() = 0;
    virtual void NextRow() = 0;
    virtual void Finalize() = 0;

INTERFACE_END();






INTERFACE_BEGIN(DbMgr)
	virtual LWRESULT Init() = 0;
	virtual DBHandle Open(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db,
	 			        int32_ port, long_ client_flag) = 0;
	virtual DBHandle OpenCopy(DBHandle otherHandle) = 0;
	virtual void Close(DBHandle db) = 0;
	/* ���ؾ�� */
	virtual DBHandle GetDbHandle() = 0;
	/* �����ض��еĲ�ѯ������Ӱ������� */
	//������������Ϊ��CppMySQLQuery�ĸ�ֵ���캯����Ҫ�ѳ�Ա����_mysql_res��Ϊ��
	virtual LWRESULT QuerySQL(DBHandle db, const std::string& sql, Cx_Interface<Ix_DbQuery>& query_out) = 0;
	
	/* ִ�зǷ��ؽ����ѯ */
	virtual int32_ ExecSQL(DBHandle db, const std::string& sql) = 0;
	/* ����mysql�������Ƿ��� */
	virtual int32_ Ping(DBHandle db) = 0;
	/* �ر�mysql ������ */
	virtual int32_ ShutDown(DBHandle db) = 0;
	/* ��Ҫ����:��������mysql ������ */
	virtual int32_ Reboot(DBHandle db) = 0;
	/*
	* ˵��:����֧��InnoDB or BDB������
	*/
	/* ��Ҫ����:��ʼ���� */
	virtual int32_ StartTransaction(DBHandle db) = 0;
	/* ��Ҫ����:�ύ���� */
	virtual int32_ Commit(DBHandle db) = 0;
	/* ��Ҫ����:�ع����� */
	virtual int32_ Rollback(DBHandle db) = 0;
	/* �õ��ͻ���Ϣ */
	virtual const std::string GetClientInfo() = 0;
	/* ��Ҫ����:�õ��ͻ��汾��Ϣ */
	virtual const long_  GetClientVersion() = 0;
	/* ��Ҫ����:�õ�������Ϣ */
	virtual const std::string GetHostInfo(DBHandle db) = 0;
	/* ��Ҫ����:�õ���������Ϣ */
	virtual const std::string GetServerInfo(DBHandle db) = 0;
	/*��Ҫ����:�õ��������汾��Ϣ*/
	virtual const long_  GetServerVersion(DBHandle db) = 0;
	/*��Ҫ����:�õ� ��ǰ���ӵ�Ĭ���ַ���*/
	virtual const std::string  GetCharacterSetName(DBHandle db) = 0;
	/* �õ�ϵͳʱ�� */
	virtual const std::string GetSysTime(DBHandle db) = 0;
	/* ���������ݿ� */
	virtual int32_ CreateDB(DBHandle db, const std::string& name) = 0;
	/* ɾ���ƶ������ݿ�*/
	virtual int32_ DropDB(DBHandle db, const std::string& name) = 0;
	virtual bool_ TableExists(DBHandle db, const std::string& table) = 0;
	virtual uint32_ LastRowId(DBHandle db) = 0;
	virtual void SetBusyTimeout(DBHandle db, int32_ nMillisecs)= 0;

INTERFACE_END();


LWDP_NAMESPACE_END;

#endif // 
