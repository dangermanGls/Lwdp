/*! \file Cx_DbMgr.h
 *  \brief DbMgr Module Impl
 *  \author Guolisen, LwDp
 *  \date   2012.12.15
 */
 
#ifndef CX_DB_MANAGER_H
#define CX_DB_MANAGER_H

#include <boost/shared_ptr.hpp>
#include <list>
#include <Interface/DbMgr/Ix_DbMgr.h>
//#include <Interface/ScriptMgr/Ix_ScriptMgr.h>
#include "DbMgrDef.h"

#include <iostream>


LWDP_NAMESPACE_BEGIN;
#if 0
class Cx_DbMgr
    : public Ix_DbMgr
{
    X3BEGIN_CLASS_DECLARE(Cx_DbQuery)
        X3DEFINE_INTERFACE_ENTRY(Ix_DbQuery)
    X3END_CLASS_DECLARE()
protected:
	Cx_DbQuery();
	virtual ~Cx_DbQuery();

protected:
 	virtual uint32_ NumRow();//������
    virtual int32_  NumFields();//������
    virtual int32_  FieldIndex(const std::string& szField);
 //0...n-1��
    virtual const std::string FieldName(int32_ nCol);

 	virtual u_long  SeekRow(u_long offerset);
    virtual int32_  GetIntField(int32_ nField, int32_ nNullValue);
    virtual int32_  GetIntField(const std::string& szField, int32_ nNullValue);
    virtual double_ GetFloatField(int32_ nField, double_ fNullValue);
    virtual double_ GetFloatField(const std::string& szField, double_ fNullValue);
 //0...n-1��
    virtual const std::string GetStringField(int32_ nField, const std::string& szNullValue);
    virtual const std::string GetStringField(const std::string& szField, const std::string& szNullValue);
    virtual bool FieldIsNull(int32_ nField);
    virtual bool GieldIsNull(const std::string& szField);
    virtual bool Eof();
    virtual void NextRow();
    virtual void Finalize();
};

#endif

class Cx_DbMgr
    : public Ix_DbMgr
{
    X3BEGIN_CLASS_DECLARE(Cx_DbMgr)
        X3DEFINE_INTERFACE_ENTRY(Ix_DbMgr)
    X3END_CLASS_DECLARE()
protected:
	Cx_DbMgr();
	virtual ~Cx_DbMgr();

protected:
	virtual LWRESULT Init();
	virtual int32_ 	Open(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db,
	 					   unsigned int32_ port, unsigned long_ client_flag);
	virtual void 	Close();
	/* ���ؾ�� */
	virtual DBHandle 	GetDbHandle();
	/* �����ض��еĲ�ѯ������Ӱ������� */
	//������������Ϊ��CppMySQLQuery�ĸ�ֵ���캯����Ҫ�ѳ�Ա����_mysql_res��Ϊ��
	//virtual CppMySQLQuery& QuerySQL(const std::string& sql);
	
	/* ִ�зǷ��ؽ����ѯ */
	virtual int32_ 		ExecSQL(const std::string& sql);
	/* ����mysql�������Ƿ��� */
	virtual int32_ 		Ping();
	/* �ر�mysql ������ */
	virtual int32_ 		ShutDown();
	/* ��Ҫ����:��������mysql ������ */
	virtual int32_ 		Reboot();
	/*
	* ˵��:����֧��InnoDB or BDB������
	*/
	/* ��Ҫ����:��ʼ���� */
	virtual int32_ 		StartTransaction();
	/* ��Ҫ����:�ύ���� */
	virtual int32_ 		Commit();
	/* ��Ҫ����:�ع����� */
	virtual int32_ 		Rollback();
	/* �õ��ͻ���Ϣ */
	virtual const std::string 	GetClientInfo();
	/* ��Ҫ����:�õ��ͻ��汾��Ϣ */
	virtual const unsigned long_  GetClientVersion();
	/* ��Ҫ����:�õ�������Ϣ */
	virtual const std::string 	GetHostInfo();
	/* ��Ҫ����:�õ���������Ϣ */
	virtual const std::string 	GetServerInfo();
	/*��Ҫ����:�õ��������汾��Ϣ*/
	virtual const unsigned long_  GetServerVersion();
	/*��Ҫ����:�õ� ��ǰ���ӵ�Ĭ���ַ���*/
	virtual const std::string  	GetCharacterSetName();
	/* �õ�ϵͳʱ�� */
	virtual const std::string 	GetSysTime();
	/* ���������ݿ� */
	virtual int32_ 	CreateDB(const std::string& name);
	/* ɾ���ƶ������ݿ�*/
	virtual int32_ 	DropDB(const std::string& name);
	virtual bool_ 	TableExists(const std::string& table);
	virtual uint32_ LastRowId();
	virtual void 	SetBusyTimeout(int32_ nMillisecs);

protected:
	/* msyql ���Ӿ�� */
	MYSQL* mDb;
	//CppMySQLQuery _db_query;

	
};


LWDP_NAMESPACE_END;

#endif
