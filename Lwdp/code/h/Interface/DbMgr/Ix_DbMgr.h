#ifndef DBMGR_INTERFACE_H
#define DBMGR_INTERFACE_H

#include <Interface/Ix_Object.h>
#include "Id_DbMgr.h"

LWDP_NAMESPACE_BEGIN;
 
typedef void* DBHandle;


INTERFACE_BEGIN(DbQuery)
 	virtual uint32_ NumRow() = 0;//多少行
    virtual int32_ NumFields() = 0;//多少列
    virtual int32_ FieldIndex(const std::string& szField) = 0;
 //0...n-1列
    virtual const std::string FieldName(int32_ nCol) = 0;
 //   const std::string fieldDeclType(int32_ nCol) = 0;
 //   int32_ fieldDataType(int32_ nCol) = 0;
 	virtual uint32_ SeekRow(uint32_ offerset) = 0;
    virtual int32_ GetIntField(int32_ nField, int32_ nNullValue) = 0;
    virtual int32_ GetIntField(const std::string& szField, int32_ nNullValue) = 0;
    virtual double_ GetFloatField(int32_ nField, double_ fNullValue) = 0;
    virtual double_ GetFloatField(const std::string& szField, double_ fNullValue) = 0;
 //0...n-1列
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
	/* 返回句柄 */
	virtual DBHandle GetDbHandle() = 0;
	/* 处理返回多行的查询，返回影响的行数 */
	//返回引用是因为在CppMySQLQuery的赋值构造函数中要把成员变量_mysql_res置为空
	virtual LWRESULT QuerySQL(DBHandle db, const std::string& sql, Cx_Interface<Ix_DbQuery>& query_out) = 0;
	
	/* 执行非返回结果查询 */
	virtual int32_ ExecSQL(DBHandle db, const std::string& sql) = 0;
	/* 测试mysql服务器是否存活 */
	virtual int32_ Ping(DBHandle db) = 0;
	/* 关闭mysql 服务器 */
	virtual int32_ ShutDown(DBHandle db) = 0;
	/* 主要功能:重新启动mysql 服务器 */
	virtual int32_ Reboot(DBHandle db) = 0;
	/*
	* 说明:事务支持InnoDB or BDB表类型
	*/
	/* 主要功能:开始事务 */
	virtual int32_ StartTransaction(DBHandle db) = 0;
	/* 主要功能:提交事务 */
	virtual int32_ Commit(DBHandle db) = 0;
	/* 主要功能:回滚事务 */
	virtual int32_ Rollback(DBHandle db) = 0;
	/* 得到客户信息 */
	virtual const std::string GetClientInfo() = 0;
	/* 主要功能:得到客户版本信息 */
	virtual const long_  GetClientVersion() = 0;
	/* 主要功能:得到主机信息 */
	virtual const std::string GetHostInfo(DBHandle db) = 0;
	/* 主要功能:得到服务器信息 */
	virtual const std::string GetServerInfo(DBHandle db) = 0;
	/*主要功能:得到服务器版本信息*/
	virtual const long_  GetServerVersion(DBHandle db) = 0;
	/*主要功能:得到 当前连接的默认字符集*/
	virtual const std::string  GetCharacterSetName(DBHandle db) = 0;
	/* 得到系统时间 */
	virtual const std::string GetSysTime(DBHandle db) = 0;
	/* 建立新数据库 */
	virtual int32_ CreateDB(DBHandle db, const std::string& name) = 0;
	/* 删除制定的数据库*/
	virtual int32_ DropDB(DBHandle db, const std::string& name) = 0;
	virtual bool_ TableExists(DBHandle db, const std::string& table) = 0;
	virtual uint32_ LastRowId(DBHandle db) = 0;
	virtual void SetBusyTimeout(DBHandle db, int32_ nMillisecs)= 0;

INTERFACE_END();


LWDP_NAMESPACE_END;

#endif // 
