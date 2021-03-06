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
#include "DbMgrDef.h"
#include <mysql.h>  
#include <iostream>


LWDP_NAMESPACE_BEGIN;


struct DBHandleClass
{
	MYSQL* mDb;

	std::string  mHost;
	std::string  mUser;
	std::string  mPasswd;
	std::string  mDbStr;
	int32_       mPort;
	int32_       mClientFlag;

	DBHandleClass():mHost(""), mDb(0),
                         mUser(""), 
                         mPasswd(""),
                         mDbStr(""),
                         mPort(0),
                         mClientFlag(0){};

	DBHandleClass(const DBHandleClass& other)
	{
		mDb = other.mDb;

		mHost = other.mHost;
		mUser = other.mUser;
		mPasswd = other.mPasswd;
		mDbStr 	= other.mDbStr;
		mPort 	= other.mPort;
		mClientFlag = other.mClientFlag;
	};
	DBHandleClass& operator=(const DBHandleClass& right)
	{
		mDb = right.mDb;

		mHost = right.mHost;
		mUser = right.mUser;
		mPasswd = right.mPasswd;
		mDbStr 	= right.mDbStr;
		mPort 	= right.mPort;
		mClientFlag = right.mClientFlag;
	};

	
};

class Cx_DbQuery
    : public Ix_DbQuery
{
	friend class Cx_DbMgr;
    X3BEGIN_CLASS_DECLARE(Cx_DbQuery)
        X3DEFINE_INTERFACE_ENTRY(Ix_DbQuery)
    X3END_CLASS_DECLARE()
public:
	Cx_DbQuery();
	virtual ~Cx_DbQuery();
	
public:
 	virtual uint32_ NumRow();//������
    virtual int32_  NumFields();//������
    virtual int32_  FieldIndex(const std::string& szField);
 //0...n-1��
    virtual const std::string FieldName(int32_ nCol);

 	virtual uint32_  SeekRow(uint32_ offerset);
    virtual int32_   GetIntField(int32_ nField, int32_ nNullValue = 0);
    virtual int32_   GetIntField(const std::string& szField, int32_ nNullValue = 0);
    virtual double_  GetFloatField(int32_ nField, double_ fNullValue = 0.0);
    virtual double_  GetFloatField(const std::string& szField, double_ fNullValue = 0.0);
 //0...n-1��
    virtual const std::string GetStringField(int32_ nField, const std::string& szNullValue = "");
    virtual const std::string GetStringField(const std::string& szField, const std::string& szNullValue = "");
    virtual bool FieldIsNull(int32_ nField);
    virtual bool GieldIsNull(const std::string& szField);
    virtual bool Eof();
    virtual void NextRow();
    virtual void Finalize();

protected:
	void freeRes();

	Cx_DbQuery(const Cx_DbQuery& rQuery);
	Cx_DbQuery& operator=(const Cx_DbQuery& rQuery);

protected:
	 MYSQL_RES*  	mMysqlRes;
	 MYSQL_FIELD* 	mField;
	 MYSQL_ROW  	mRow;
	 uint32_   		mRow_count;
	 uint32_   		mField_count;
};



class Cx_DbMgr
    : public Ix_DbMgr
{
	friend class Cx_DbQuery;
    X3BEGIN_CLASS_DECLARE(Cx_DbMgr)
        X3DEFINE_INTERFACE_ENTRY(Ix_DbMgr)
    X3END_CLASS_DECLARE()
protected:
	Cx_DbMgr();
	virtual ~Cx_DbMgr();

protected:
	virtual LWRESULT Init();
	virtual DBHandle Open(const std::string& host, const std::string& user, const std::string& passwd, const std::string& db,
	 					    int32_ port, long_ client_flag);
	virtual DBHandle OpenCopy(DBHandle otherHandle);
	
	virtual void 	Close(DBHandle db);

	virtual DBHandle 	GetDbHandle();

	virtual LWRESULT    QuerySQL(DBHandle db, const std::string& sql, Cx_Interface<Ix_DbQuery>& query_out);

	virtual int32_ 		ExecSQL(DBHandle db, const std::string& sql);

	virtual int32_ 		Ping(DBHandle db);

	virtual int32_ 		ShutDown(DBHandle db);

	virtual int32_ 		Reboot(DBHandle db);

	virtual int32_ 		StartTransaction(DBHandle db);

	virtual int32_ 		Commit(DBHandle db);

	virtual int32_ 		Rollback(DBHandle db);

	virtual const std::string 	GetClientInfo();

	virtual const long_  GetClientVersion();

	virtual const std::string 	GetHostInfo(DBHandle db);

	virtual const std::string 	GetServerInfo(DBHandle db);

	virtual const long_  GetServerVersion(DBHandle db);

	virtual const std::string  	GetCharacterSetName(DBHandle db);

	virtual const std::string 	GetSysTime(DBHandle db);

	virtual int32_ 	CreateDB(DBHandle db, const std::string& name);

	virtual int32_ 	DropDB(DBHandle db, const std::string& name);
	virtual bool_ 	TableExists(DBHandle db, const std::string& table);
	virtual uint32_ LastRowId(DBHandle db);
	virtual void 	SetBusyTimeout(DBHandle db, int32_ nMillisecs);

};


LWDP_NAMESPACE_END;

#endif
