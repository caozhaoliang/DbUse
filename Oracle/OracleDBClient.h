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
		//�������ӳ�
		bool CreateConnectionPool(int nMax,int nMin,int nIncrConn,string& strError);
		//������
		bool CreateTables(map<string,string> mapValue,string& strError);
		//��ѯ���ݿ����
		bool QueryData(const string strSql,map<int,vector<string> >& mapValue,string& strError);
		//�ǲ�ѯ���ݿ�Ĳ���
		bool UpdateData(const string strSql,string& strError);
		//�ǲ�ѯ���ݿ����������
		bool UpdateArrayData(const string strSql,vector<void*> vValue,vector<Type> vType,vector<ub2*> vInt,string& strError,int nNum,sb4 nSize = 0);
		//����̽��(̽��oracle�������Ƿ���)
		bool ConnectDetect(string& strError);
		bool ExecProcedure(const string& strProcName,map<int,vector<string> >& mapValue,string& strError);
	private:
		Environment* m_pEnv;  //occi��������
		StatelessConnectionPool* m_pConnectionPool;  //���ӳ�
		string m_strName;     //�������ݿ��û���
		string m_strPwd;      //�������ݿ�����
		string m_strConnect;  //�������ݿ��ַ���(��������ַ:1521/ʵ����)
	};

#endif
