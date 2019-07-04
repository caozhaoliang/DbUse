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
	//����occi��������
	m_pEnv = Environment::createEnvironment("ZHS16GBK","ZHS16GBK",Environment::THREADED_MUTEXED);
}

COracleDBClient::~COracleDBClient()
{
	//�Ͽ����ӳ�
	if (m_pConnectionPool != NULL)
	{
		m_pEnv->terminateStatelessConnectionPool(m_pConnectionPool);
		m_pConnectionPool = NULL;
	}
	//��ֹocci��������
	Environment::terminateEnvironment(m_pEnv);
}
//�������ӳ�
bool COracleDBClient::CreateConnectionPool(int nMax,int nMin,int nIncrConn,string& strError)
{
	try
	{
		m_pConnectionPool = m_pEnv->createStatelessConnectionPool(m_strName,m_strPwd,m_strConnect,nMax,nMin,nIncrConn,StatelessConnectionPool::HOMOGENEOUS);
	}
	catch(SQLException e)
	{
		//�쳣��Ϣ
		strError = e.what();
		return false;
	}
	return true;
}
//������
bool COracleDBClient::CreateTables(map<string,string> mapValue,string& strError)
{
	try
	{
		//�����ӳػ�ȡ����(������Ӷ����ڷ�æ����С�������������ʱ��,�����趨������������)
		Connection *conn = m_pConnectionPool->getConnection();
		//����Statement����
		//Statement�������ִ��SQL�������з���,�Ƕ����ݿ�����ľ���ʵ��
		Statement *stmt = conn->createStatement();
		char szBuf[256] = {0};
		map<string,string>::iterator it = mapValue.begin();
		for(;it != mapValue.end();it++)
		{
			memset(szBuf,0,256);
			snprintf(szBuf,256,"select count(*) from user_tables where table_name =upper('%s')",it->first.c_str());
			stmt->setSQL(szBuf);
			ResultSet *rs = stmt->executeQuery();  //ִ�����еĲ�ѯ���
			rs->next();
			int nCount = rs->getInt(1);
			if (nCount == 0)  //���ݿ��в����ڸñ�
			{
				stmt->setSQL(it->second);
				stmt->executeUpdate();
			}
			//�ͷŽ����
			stmt->closeResultSet(rs);
		}
		//��ֹStatement����
		conn->terminateStatement(stmt);
		//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
		m_pConnectionPool->releaseConnection(conn);
	}
	catch(SQLException e)
	{
		//�쳣��Ϣ
		strError = e.what();
		return false;
	}
	return true;
}
//�ǲ�ѯ���ݿ�Ĳ���
bool COracleDBClient::UpdateData(const string strSql,string& strError)
{
	try
	{
		//if (m_pConnectionPool != NULL)
		//{
			//�����ӳػ�ȡ����(������Ӷ����ڷ�æ����С�������������ʱ��,�����趨������������)
			Connection *conn = m_pConnectionPool->getConnection();
			//����Statement����
			//Statement�������ִ��SQL�������з���,�Ƕ����ݿ�����ľ���ʵ��
			Statement *stmt = conn->createStatement();
			stmt->setSQL(strSql);
			//ִ��һ����¼�ķǲ�ѯ���
			stmt->executeUpdate();  
			//��ֹStatement����
			conn->terminateStatement(stmt);
			//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
			m_pConnectionPool->releaseConnection(conn);
		//}
	}
	catch(SQLException e)
	{
		//�쳣��Ϣ
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
		//����Statement����
		//Statement�������ִ��SQL�������з���,�Ƕ����ݿ�����ľ���ʵ��
		Statement *stmt = conn->createStatement(szProcBuff);
		stmt->setInt(1,atoi(vInParam[1].c_str()));
		stmt->setInt(2,atoi(vInParam[2].c_str()));	//PageSize
		stmt->setInt(3,atoi(vInParam[3].c_str()));	//PageNum ҳ��
		if(atoi(vInParam[4].c_str()) > 0){
			stmt->setInt(4,1);	//Sort By CreateTime Desc
		}else{
			stmt->setInt(4,0);	//Sort By CreateTime ASC
		}
		stmt->registerOutParam(5,OCCICURSOR);;
		int nRet = stmt->execute();
		//ResultSet 
		ResultSet *rs = stmt->getCursor(5);  //��ȡ�α�
		vector<MetaData> vMetaData = rs->getColumnListMetaData();
		int nSize = vMetaData.size();  //������
		string fieldName;
		vector<string> vTemp;
		for(int n=0; n< nSize;n++){
			MetaData md = vMetaData[n];
			fieldName = md.getString(MetaData::ATTR_NAME);
			vTemp.push_back(fieldName);
		}
		mapValue[0]=vTemp;
		int nNum = 1;
		while(rs->next())  //����ѯ�Ľ������ȡ��
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
		//��ֹStatement����
		conn->terminateStatement(stmt);
		//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
		m_pConnectionPool->releaseConnection(conn);		
	}catch(SQLException e){
		strError = e.what();
		return false;
	}
	return true;
}

//��ѯ���ݿ����
bool COracleDBClient::QueryData(const string strSql,map<int,vector<string> >& mapValue,string& strError)
{
	try
	{
		//if (m_pConnectionPool != NULL)
		//{
			//�����ӳػ�ȡ����(������Ӷ����ڷ�æ����С�������������ʱ��,�����趨������������)
			Connection *conn = m_pConnectionPool->getConnection();
			//����Statement����
			//Statement�������ִ��SQL�������з���,�Ƕ����ݿ�����ľ���ʵ��
			Statement *stmt = conn->createStatement();
			stmt->setSQL(strSql);			//"exe SELECT_ORDER_INFO userid  100"
			ResultSet *rs = stmt->executeQuery();  //ִ�����еĲ�ѯ���
			vector<MetaData> vMetaData = rs->getColumnListMetaData();
			int nSize = vMetaData.size();  //������
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
			while(rs->next())  //����ѯ�Ľ������ȡ��
			{
				//vector<string> vTemp;
				vTemp.clear();
				for(int i = 1;i <= nSize;i++)
				{
					vTemp.push_back(rs->getString(i));  
				}
				mapValue[nNum++] = vTemp;
			}
			//�ͷŽ����
			stmt->closeResultSet(rs);
			//��ֹStatement����
			conn->terminateStatement(stmt);
			//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
			m_pConnectionPool->releaseConnection(conn);
		//}
	}
	catch(SQLException e)
	{
		//�쳣��Ϣ
		strError = e.what(); 
		return false;
	}
	return true;
}
//�ǲ�ѯ���ݿ����������
bool COracleDBClient::UpdateArrayData(const string strSql,vector<void*> vValue,vector<Type> vType,vector<ub2*> vInt,string& strError,int nNum,sb4 nSize)
{
	try
	{
		//if (m_pConnectionPool != NULL)
		//{
			//�����ӳػ�ȡ����(������Ӷ����ڷ�æ����С�������������ʱ��,�����趨������������)
			Connection *conn = m_pConnectionPool->getConnection();
			//����Statement����
			//Statement�������ִ��SQL�������з���,�Ƕ����ݿ�����ľ���ʵ��
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
			//�ǲ�ѯ��������
			stmt->executeArrayUpdate(nNum);
			//��ֹStatement����
			conn->terminateStatement(stmt);
			//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
			m_pConnectionPool->releaseConnection(conn);
		//}
	}
	catch(SQLException e)
	{
		//�쳣��Ϣ
		strError = e.what();  
		return false;
	}
	return true;
}
//����̽��(̽��oracle�������Ƿ���)
bool COracleDBClient::ConnectDetect(string& strError)
{
	
	try{
		//�����ӳػ�ȡ����(������Ӷ����ڷ�æ����С�������������ʱ��,�����趨������������)
		Connection *conn = m_pConnectionPool->getConnection();
		//����Statement����
		//Statement�������ִ��SQL�������з���,�Ƕ����ݿ�����ľ���ʵ��
		Statement *stmt = conn->createStatement();
		stmt->setSQL("select sysdate from dual");
		ResultSet *rs = stmt->executeQuery();  //ִ�����еĲ�ѯ���
		while(rs->next())  //����ѯ�Ľ������ȡ��
		{
			if(rs->isNull(1))
			{
				//�ͷŽ����
				stmt->closeResultSet(rs);
				//��ֹStatement����
				conn->terminateStatement(stmt);
				//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
				m_pConnectionPool->releaseConnection(conn);
				return false;
			}
		}
		//�ͷŽ����
		stmt->closeResultSet(rs);
		//��ֹStatement����
		conn->terminateStatement(stmt);
		//�ͷ�����(�ø����Ӵ��ڿ��й�����ʹ��)
		m_pConnectionPool->releaseConnection(conn);
	}
	catch(SQLException e)
	{
		//�쳣��Ϣ
		strError = e.what();
		return false;
	}
	return true;	
}
		
