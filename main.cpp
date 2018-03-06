#include "dbmanager.hpp"
#include <iostream>
#include "CFreeTds.h"


using namespace std;
#if 0
int main()
{
	string ip = "";
	unsigned short port=1433;
	string dbname = "YDZQ_LOG";
	string dbuser = "sa";
	string passwd = "";

	string procedure = "SELECT USER_MBLPHONE,USER_PASSWORD from TBL_USER_SMS";

	CDataSvrFreeTDS tds;
	if( ! tds.init(ip,port,dbname,dbuser,passwd) )
	{
		cout <<"init error"<<endl;
	}
	if( !tds.connect() )
	{
		cout << "connect error" <<endl;
	}
	try{
		tds.execSQL(procedure.c_str());
		
	}catch(std::exception& ex){
		cout <<"ex.what:"<<ex.what() <<endl;
	}
	int iRet = -1;
	while((iRet = tds.getResult()) != 0 ){
		char buf[32] = {0};
		char buf2[32] = {0};
		int i = -1,j = -1;
		if(iRet == 1){
			// cout <<"iRet == 1"<<endl;
			if(!tds.bindColumn(1,0,buf) )
			{
				cout <<"bindColumn"<<endl;
			}

			if(!tds.bindColumn(2,0,buf2) )
			{
				cout <<"bindColumn2"<<endl;
			}
		}
		while(tds.moveNextRow()){
			// cout <<"moveNextRow"<<endl;
			cout <<"buf"<<buf<<endl<<"buf2"<<buf2<<endl;
		}
	}

	tds.disconnect();
	return 0;

}
#endif 
#if 1
int main()
{
	string ip = "";
	string port="1433";
	string dbname = "YDZQ_LOG";
	string dbuser = "sa";
	string passwd = "";

	string procedure = "YDZQ_LOG..USP_MOB_INSERTTEST";

	CDbManager::getInstance()->init(ip+":"+port,dbuser,
								passwd,dbname);

	CDbManager::getInstance()->setTimeoutParam(2,2);

	CDbManager::getInstance()->reconn(2);

	string cmd_str = "SELECT USER_MBLPHONE,USER_PASSWORD from TBL_USER_SMS where USER_MBLPHONE = 18219205857";

	//return json 
	Json::Value json_ret;

	CDbManager::getInstance()->execSqlCmd(cmd_str,json_ret);
	// for(int i =0; i < (int)json_ret[0].size(); ++i){
	// 	cout <<"1："<<json_ret[i]["USER_MBLPHONE"].asString()<<"\n2:"<<json_ret[i]["USER_PASSWORD"].asString()<<endl;
	// }
	Json::FastWriter write;
	string sRet = write.write(json_ret);
	cout << sRet<<endl;
	return 0;
}
#endif 




