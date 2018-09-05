#include <iostream>
#include <stdio.h>
#include <string>
#include "OracleDBClient.h"
#include <vector>
#include <map>
#include "occiControl.h"

using namespace std;
using namespace thinkive;

int main()
{
/*
	Environment* m_pEnv = NULL;
	StatelessConnectionPool* m_ConnPool=NULL;
	Connection *conn=NULL;
	Statement *stmt=NULL;
	string name = "TK_QH_TJD";
	string pwd = "TK_QH_TJD";
	string connStr = "192.168.1.23:1521/thinkive";
	m_pEnv = Environment::createEnvironment(Environment::THREADED_MUTEXED);
	m_ConnPool = m_pEnv->createStatelessConnectionPool(name,pwd,connStr,8,2,2,StatelessConnectionPool::HOMOGENEOUS);
	conn = m_pEnv->createConnection(name,pwd,connStr);
	if(NULL == conn){
		cout<< "conn is NULL"<<endl;
	}	
	printf("%p\n",conn);	
*/
	map<int,vector<string> > m_v;
	string strError;
	string name = "TK_QH_TJD";
	string pwd = "TK_QH_TJD";
	string connStr = "192.168.1.23:1521/thinkive";
	COracleDBClient* odbc = new COracleDBClient(name,pwd,connStr);
	odbc->CreateConnectionPool(6,2,2,strError);
	string strProcSql = "SELECT_ORDER_INFO 10001437 5 1 1";
	if(!odbc->ExecProcedure(strProcSql,m_v,strError)){
		cout <<"Error :"<<strError<<endl;
	}else{
		for(int i=0;i < m_v.size(); ++i){
			std::vector<string> vTemp = m_v[i];
			std::vector<string>::iterator it = vTemp.begin();
			for(;it != vTemp.end();++it){
				cout <<*it<<"  ";
			}
			cout<<endl;
		}
	}
	//cout << strError<<endl;

	//string insertSql="insert into TBL_USER_INFO values('0002',NULL,NULL,NULL,sysdate)";
	//odbc->UpdateData(insertSql,strError);
	/*	
	string sql = "select * from TBL_USER_INFO";
	string sql2 = "select * from TBL_CONDITION_ORDER ";
	vector<string> vSql;
	vSql.push_back(sql);
	vSql.push_back(sql2);
	odbc->QueryData(sql,m_v,strError);
	map<int,vector<string> >::iterator it = m_v.begin();
	for(;it != m_v.end(); ++it){
		vector<string> v_temp = it->second;
		vector<string>::iterator it_v = v_temp.begin();
		for(;it_v != v_temp.end(); ++it_v){
			cout <<*it_v<<" ";
		}
		cout <<endl;
	}	
	*/
	//odbc->close();

	//cout << strError<<endl;
	return 0;

}

