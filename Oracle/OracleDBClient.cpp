#include "OracleDBClient.h"
#include <string.h>
#include <stdio.h>
//#include "global.h"

static void split(string s,string sep,vector<string>& v){
	v.clear();

	::size_t pos = 0, from = 0;

	while ((pos = s.find(sep, from)) != std::string::npos) {
		if (pos != 0) v.push_back(s.substr(from, pos - from));
		//if (v.size() == maxsplit) return;
		from = pos + sep.size();
	}

	if (from < s.size()) v.push_back(s.substr(from));	
}
COracleDBClient::COracleDBClient(const string strName,const string strPwd,const string strConnect)
{
	m_strName = strName;
	m_strPwd = strPwd;
	m_strConnect = strConnect;
	m_pConnectionPool = NULL;
	//创建occi环境变量
	m_pEnv = Environment::createEnvironment("ZHS16GBK","ZHS16GBK",Environment::THREADED_MUTEXED);
}

COracleDBClient::~COracleDBClient()
{
	//断开连接池
	if (m_pConnectionPool != NULL)
	{
		m_pEnv->terminateStatelessConnectionPool(m_pConnectionPool);
		m_pConnectionPool = NULL;
	}
	//终止occi环境变量
	Environment::terminateEnvironment(m_pEnv);
}
//创建连接池
bool COracleDBClient::CreateConnectionPool(int nMax,int nMin,int nIncrConn,string& strError)
{
	try
	{
		m_pConnectionPool = m_pEnv->createStatelessConnectionPool(m_strName,m_strPwd,m_strConnect,nMax,nMin,nIncrConn,StatelessConnectionPool::HOMOGENEOUS);
	}
	catch(SQLException e)
	{
		//异常信息
		strError = e.what();
		return false;
	}
	return true;
}
//创建表
bool COracleDBClient::CreateTables(map<string,string> mapValue,string& strError)
{
	try
	{
		//从连接池获取连接(如果连接都处于繁忙中且小于最大连接数的时候,增加设定的连接数连接)
		Connection *conn = m_pConnectionPool->getConnection();
		//创建Statement对象
		//Statement类包含了执行SQL语句的所有方法,是对数据库操作的具体实现
		Statement *stmt = conn->createStatement();
		char szBuf[256] = {0};
		map<string,string>::iterator it = mapValue.begin();
		for(;it != mapValue.end();it++)
		{
			memset(szBuf,0,256);
			snprintf(szBuf,256,"select count(*) from user_tables where table_name =upper('%s')",it->first.c_str());
			stmt->setSQL(szBuf);
			ResultSet *rs = stmt->executeQuery();  //执行所有的查询语句
			rs->next();
			int nCount = rs->getInt(1);
			if (nCount == 0)  //数据库中不存在该表
			{
				stmt->setSQL(it->second);
				stmt->executeUpdate();
			}
			//释放结果集
			stmt->closeResultSet(rs);
		}
		//终止Statement对象
		conn->terminateStatement(stmt);
		//释放连接(让该连接处于空闲供其他使用)
		m_pConnectionPool->releaseConnection(conn);
	}
	catch(SQLException e)
	{
		//异常信息
		strError = e.what();
		return false;
	}
	return true;
}
//非查询数据库的操作
bool COracleDBClient::UpdateData(const string strSql,string& strError)
{
	try
	{
		//if (m_pConnectionPool != NULL)
		//{
			//从连接池获取连接(如果连接都处于繁忙中且小于最大连接数的时候,增加设定的连接数连接)
			Connection *conn = m_pConnectionPool->getConnection();
			//创建Statement对象
			//Statement类包含了执行SQL语句的所有方法,是对数据库操作的具体实现
			Statement *stmt = conn->createStatement();
			stmt->setSQL(strSql);
			//执行一条记录的非查询语句
			stmt->executeUpdate();  
			//终止Statement对象
			conn->terminateStatement(stmt);
			//释放连接(让该连接处于空闲供其他使用)
			m_pConnectionPool->releaseConnection(conn);
		//}
	}
	catch(SQLException e)
	{
		//异常信息
		strError = e.what();
		return false;
	}
	return true;
}
bool COracleDBClient::ExecProcedure(const string& strProcName,map<int,vector<string> >& mapValue,string& strError){
	vector<string> vInParam;
	split(strProcName," ",vInParam);
	char szProcBuff[1024] = {0};
	if(vInParam.size() < 4){
		//return false;
	}
	snprintf(szProcBuff,1024,"BEGIN %s(:1,:2,:3,:4,:5);END;",vInParam[0].c_str());
	try{
		Connection *conn = m_pConnectionPool->getConnection();
		//创建Statement对象
		//Statement类包含了执行SQL语句的所有方法,是对数据库操作的具体实现
		Statement *stmt = conn->createStatement(szProcBuff);
		stmt->setInt(1,atoi(vInParam[1].c_str()));
		stmt->setInt(2,atoi(vInParam[2].c_str()));	//PageSize
		stmt->setInt(3,atoi(vInParam[3].c_str()));	//PageNum 页码
		if(atoi(vInParam[4].c_str()) > 0){
			stmt->setInt(4,1);	//Sort By CreateTime Desc
		}else{
			stmt->setInt(4,0);	//Sort By CreateTime ASC
		}
		stmt->registerOutParam(5,OCCICURSOR);;
		int nRet = stmt->execute();
		//ResultSet 
		ResultSet *rs = stmt->getCursor(5);  //获取游标
		vector<MetaData> vMetaData = rs->getColumnListMetaData();
		int nSize = vMetaData.size();  //表列数
		string fieldName;
		vector<string> vTemp;
		for(int n=0; n< nSize;n++){
			MetaData md = vMetaData[n];
			fieldName = md.getString(MetaData::ATTR_NAME);
			vTemp.push_back(fieldName);
		}
		mapValue[0]=vTemp;
		int nNum = 1;
		while(rs->next())  //将查询的结果逐条取出
		{
			vTemp.clear();
			for(int i = 1;i <= nSize;i++)
			{
				vTemp.push_back(rs->getString(i));  
			}
			mapValue[nNum++] = vTemp;
		}
		//strError = stmt->getString(1);
		stmt->closeResultSet(rs);
		//终止Statement对象
		conn->terminateStatement(stmt);
		//释放连接(让该连接处于空闲供其他使用)
		m_pConnectionPool->releaseConnection(conn);		
	}catch(SQLException e){
		strError = e.what();
		return false;
	}
	return true;
}

//查询数据库操作
bool COracleDBClient::QueryData(const string strSql,map<int,vector<string> >& mapValue,string& strError)
{
	try
	{
		//if (m_pConnectionPool != NULL)
		//{
			//从连接池获取连接(如果连接都处于繁忙中且小于最大连接数的时候,增加设定的连接数连接)
			Connection *conn = m_pConnectionPool->getConnection();
			//创建Statement对象
			//Statement类包含了执行SQL语句的所有方法,是对数据库操作的具体实现
			Statement *stmt = conn->createStatement();
			stmt->setSQL(strSql);			//"exe SELECT_ORDER_INFO userid  100"
			ResultSet *rs = stmt->executeQuery();  //执行所有的查询语句
			vector<MetaData> vMetaData = rs->getColumnListMetaData();
			int nSize = vMetaData.size();  //表列数
			string fieldName;
			vector<string> vTemp;
			for(int n=0; n< nSize;n++){
				//vTemp.clear();
				MetaData md = vMetaData[n];
				fieldName = md.getString(MetaData::ATTR_NAME);
				vTemp.push_back(fieldName);
			}
			mapValue[0]=vTemp;
			int nNum = 1;
			while(rs->next())  //将查询的结果逐条取出
			{
				//vector<string> vTemp;
				vTemp.clear();
				for(int i = 1;i <= nSize;i++)
				{
					vTemp.push_back(rs->getString(i));  
				}
				mapValue[nNum++] = vTemp;
			}
			//释放结果集
			stmt->closeResultSet(rs);
			//终止Statement对象
			conn->terminateStatement(stmt);
			//释放连接(让该连接处于空闲供其他使用)
			m_pConnectionPool->releaseConnection(conn);
		//}
	}
	catch(SQLException e)
	{
		//异常信息
		strError = e.what(); 
		return false;
	}
	return true;
}
//非查询数据库的批量操作
bool COracleDBClient::UpdateArrayData(const string strSql,vector<void*> vValue,vector<Type> vType,vector<ub2*> vInt,string& strError,int nNum,sb4 nSize)
{
	try
	{
		//if (m_pConnectionPool != NULL)
		//{
			//从连接池获取连接(如果连接都处于繁忙中且小于最大连接数的时候,增加设定的连接数连接)
			Connection *conn = m_pConnectionPool->getConnection();
			//创建Statement对象
			//Statement类包含了执行SQL语句的所有方法,是对数据库操作的具体实现
			Statement *stmt = conn->createStatement();
			stmt->setSQL(strSql);
			vector<Type>::iterator vIt = vType.begin();
			unsigned int n = 1;
			int i = 0;
			for(;vIt != vType.end();vIt++)
			{
				if (*vIt == SQLT_INT)
				{
					nSize = sizeof(int);
				}else if(*vIt == SQLT_FLT)
				{
					nSize = sizeof(float);
				}
				stmt->setDataBuffer(n++, (void*)vValue[i],*vIt,nSize,vInt[i]);
				i++;
			}
			//非查询批量操作
			stmt->executeArrayUpdate(nNum);
			//终止Statement对象
			conn->terminateStatement(stmt);
			//释放连接(让该连接处于空闲供其他使用)
			m_pConnectionPool->releaseConnection(conn);
		//}
	}
	catch(SQLException e)
	{
		//异常信息
		strError = e.what();  
		return false;
	}
	return true;
}
//连接探测(探测oracle服务器是否存活)
bool COracleDBClient::ConnectDetect(string& strError)
{
	
	try{
		//从连接池获取连接(如果连接都处于繁忙中且小于最大连接数的时候,增加设定的连接数连接)
		Connection *conn = m_pConnectionPool->getConnection();
		//创建Statement对象
		//Statement类包含了执行SQL语句的所有方法,是对数据库操作的具体实现
		Statement *stmt = conn->createStatement();
		stmt->setSQL("select sysdate from dual");
		ResultSet *rs = stmt->executeQuery();  //执行所有的查询语句
		while(rs->next())  //将查询的结果逐条取出
		{
			if(rs->isNull(1))
			{
				//释放结果集
				stmt->closeResultSet(rs);
				//终止Statement对象
				conn->terminateStatement(stmt);
				//释放连接(让该连接处于空闲供其他使用)
				m_pConnectionPool->releaseConnection(conn);
				return false;
			}
		}
		//释放结果集
		stmt->closeResultSet(rs);
		//终止Statement对象
		conn->terminateStatement(stmt);
		//释放连接(让该连接处于空闲供其他使用)
		m_pConnectionPool->releaseConnection(conn);
	}
	catch(SQLException e)
	{
		//异常信息
		strError = e.what();
		return false;
	}
	return true;	
}
		
