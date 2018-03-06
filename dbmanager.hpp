#ifndef __KDS_DBMANAGER_FTDS_H__
#define __KDS_DBMANAGER_FTDS_H__

#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <sybfront.h> //freetds头文件  
#include <sybdb.h>    //freetds 
#include <json/json.h>

using namespace std;


//连接超时默认时间
const int I_DB_CONN_DEFAULT_TIME     = 2;

//连接最大超时时间
const int I_DB_CONN_MAX_TIME         = 30;

//读写超时默认时间
const int I_DB_RW_DEFAULT_TIME       = 2;

//读写最大超时时间
const int I_DB_RW_MAX_TIME           = 60;


class CDbManager
{
  public:
    CDbManager();
    ~CDbManager();

    bool init(const string & server, const string & userName, const string & password, const string & DBName);
    int conn();
    int reconn(int retry_times = 2);
    void closeDB();
    bool execSqlCmd(const string & cmd_str,Json::Value& json_ret);

    void setTimeoutParam(const int conn_timeout, const int rw_timeout);

    static CDbManager* getInstance();
  private:
    DBPROCESS * m_dbprocess;
    bool is_free;
    bool is_conn;

    std::string server_name;
    std::string user_name;
    std::string pass_word;
    std::string db_name;

    //连接超时
    unsigned int login_time_out; 

    //读写超时
    unsigned int rw_time_out;   

    static CDbManager* pInstance;
};


#endif
