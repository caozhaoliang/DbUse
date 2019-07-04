#ifndef ORACLEDB_CLIENT__H
#define ORACLEDB_CLIENT__H

#include <string>
#include <vector>
#include <map>
#include "./include/occiControl.h"
#include <stdlib.h>

using namespace std;
using namespace oracle::occi;
	class COracleDBClient
	{
	public:
		COracleDBClient(const string strName,const string strPwd,const string strConnect);
		~COracleDBClient();
		//创建连接池
		bool CreateConnectionPool(int nMax,int nMin,int nIncrConn,string& strError);
		//创建表
		bool CreateTables(map<string,string> mapValue,string& strError);
		//查询数据库操作
		bool QueryData(const string strSql,map<int,vector<string> >& mapValue,string& strError);
		//非查询数据库的操作
		bool UpdateData(const string strSql,string& strError);
		//非查询数据库的批量操作
		bool UpdateArrayData(const string strSql,vector<void*> vValue,vector<Type> vType,vector<ub2*> vInt,string& strError,int nNum,sb4 nSize = 0);
		//连接探测(探测oracle服务器是否存活)
		bool ConnectDetect(string& strError);
		bool ExecProcedure(const string& strProcName,map<int,vector<string> >& mapValue,string& strError);
	private:
		Environment* m_pEnv;  //occi环境变量
		StatelessConnectionPool* m_pConnectionPool;  //连接池
		string m_strName;     //连接数据库用户名
		string m_strPwd;      //连接数据库密码
		string m_strConnect;  //连接数据库字符串(服务器地址:1521/实例名)
	};

#endif
