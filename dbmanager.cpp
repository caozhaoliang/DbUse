#include <stdio.h>
#include <assert.h>
#include "dbmanager.hpp"
//#include "log.hpp"
#include <string.h>


CDbManager* CDbManager::pInstance = NULL;

int err_handler( DBPROCESS *dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);
int msg_handler( DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line );

CDbManager::CDbManager() : m_dbprocess(FAIL),
    is_free(true),
    is_conn(false),
    server_name(""),
    user_name(""),
    pass_word(""),
    db_name(""),
    login_time_out(I_DB_CONN_DEFAULT_TIME),
    rw_time_out(I_DB_RW_DEFAULT_TIME)
{

}

CDbManager::~CDbManager()
{ 
  if(!is_free)
  {
      dbfreebuf(m_dbprocess);
      dbclose(m_dbprocess); 
      dbexit();
      is_conn = false;
      m_dbprocess = FAIL;
  }
}

CDbManager* CDbManager::getInstance()
{
    if(NULL == pInstance)
    {
        pInstance = new CDbManager();
    }

    return pInstance;
}

bool CDbManager::init(const string & server, 
                      const string & userName, 
                      const string & password, 
                      const string & DBName)
{
    server_name = server;
    user_name   = userName;
    pass_word   = password;
    db_name     = DBName;

    //RUN_LOG("Database: server_name:%s, user_name:%s, pass_word:%s, db_name:%s",
       // server_name.c_str(), user_name.c_str(), pass_word.c_str(), db_name.c_str());

    return true;  
}

int  CDbManager::conn()
{
    if(dbinit() == FAIL)
    {
        return -1;
    }

    // freetds 错误处理
    dberrhandle(err_handler);
    // 服务器错误处理
    dbmsghandle(msg_handler);

    LOGINREC *loginrec = dblogin(); 
    if(loginrec == NULL)
    {
        dbexit();
        return -2;
    }

    DBSETLUSER(loginrec, user_name.c_str());         
    DBSETLPWD(loginrec, pass_word.c_str());  

    dbsetlogintime(login_time_out);
    dbsettime(rw_time_out);

    m_dbprocess = dbopen(loginrec, server_name.c_str());
    if(m_dbprocess == FAIL)  
    {  
        dbloginfree(loginrec);
        dbexit();
        return -3;
    }  
         
    if(dbuse(m_dbprocess, db_name.c_str()) == FAIL)  
    {
        dbloginfree(loginrec);
        dbfreebuf(m_dbprocess);
        dbexit();
        return -4;
    }

    dbloginfree(loginrec);
    is_free = false;

    is_conn = true;

    //RUN_LOG("Connect database successfull !");

    return 0;
}

void CDbManager::setTimeoutParam(const int conn_timeout, const int rw_timeout)
{
    if((conn_timeout > 0) && (conn_timeout <= I_DB_CONN_MAX_TIME))
    {
        login_time_out = conn_timeout;
    }
    else
    {
        login_time_out = I_DB_CONN_DEFAULT_TIME;
    }

    if((rw_timeout > 0) && (rw_timeout <= I_DB_CONN_MAX_TIME))
    {
        rw_time_out = rw_timeout;
    }
    else
    {
        rw_time_out = I_DB_RW_DEFAULT_TIME;
    }

    return;
}

int CDbManager::reconn(int retry_times)
{
    int ret = 0;
    do
    {
        ret = conn();
        if(0 == ret) break;
        
        //ERR_LOG("Connect database failed, reconnect ...");
        usleep(500 * 1000);
    }while(--retry_times);

    return ret;
}

bool CDbManager::execSqlCmd(const string & cmd_str,Json::Value& json_ret)
{
    // Json::Value json_ret;

    if(!is_conn)
    {
        if(0 != reconn())
        {
            json_ret[0][0]["errorcode"] = "-1";
            json_ret[0][0]["errormsg"] = "操作失败";
            json_ret[1] = "error";
            //ERR_LOG("reconn failed.");
            return false;
        }
    }

    dbcmd(m_dbprocess, cmd_str.c_str());    //构造sql语句
    if(dbsqlexec(m_dbprocess) == FAIL) {
        //ERR_LOG("dbsqlexec error.");
        json_ret[0][0]["errorcode"] = "-1";
        json_ret[0][0]["errormsg"] = "操作失败";
        json_ret[1] = "error";
        //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
        //ERR_LOG("dbsqlexec failed.");
        closeDB();
        //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
        return false;
    }

    //RUN_LOG("dbsqlexec success.");
    int erc;

    int num = 0;

    //得到sql语句的执行结果
    while ((erc = dbresults(m_dbprocess)) != NO_MORE_RESULTS) {
        struct COL {
            char *name;
            char *buffer;
            int type, size, status;
        } *columns, *pcol;
        int ncols;
        int row_code;

        if (erc == FAIL) {
      //      ERR_LOG("dbresults failed");
            json_ret[0][0]["errorcode"] = "-1";
            json_ret[0][0]["errormsg"] = "操作失败";
            json_ret[1] = "error";
            //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
            closeDB();
            //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
            return false;
        }

        ncols = dbnumcols(m_dbprocess);    //返回查询的结果集数量
        //DBG_LOG("Procedure total return: %d.", ncols);
        //动态分配内存并自动初始化
        if ((columns = (struct COL *)calloc(ncols, sizeof(struct COL))) == NULL) {
            perror(NULL);
            json_ret[0][0]["errorcode"] = "-1";
            json_ret[0][0]["errormsg"] = "操作失败";
            json_ret[1] = "error";
            //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
            //ERR_LOG("operator-database failed 1.");
            closeDB();
            //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
            return false;
        }

        for (pcol = columns; pcol - columns < ncols; pcol++) {
            int c = pcol - columns + 1;            

            pcol->name = dbcolname(m_dbprocess, c);    //返回结果集列名
            pcol->type = dbcoltype(m_dbprocess, c);    //返回结果集列数据类型
            pcol->size = dbcollen(m_dbprocess, c);    //返回列最大长度

            if (SYBCHAR != pcol->type) {
                pcol->size = dbwillconvert(pcol->type, SYBCHAR);
                if(pcol->size == TRUE) {
                    pcol->size = 255;
                }
            }

         //   DBG_LOG("pcol->size: %d, pcol->name: %s", pcol->size, pcol->name);

            if ((pcol->buffer = (char *)calloc(1, pcol->size + 1)) == NULL) {
                perror(NULL);
                json_ret[0][0]["errorcode"] = "-1";
                json_ret[0][0]["errormsg"] = "操作失败";
                json_ret[1] = "error";
                //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
                free(columns);
                closeDB();
           //     ERR_LOG("operator-database failed 2.");
                //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
                return false;
            }

            //将sql查询到的列数据绑定到一个变量
            erc = dbbind(m_dbprocess, c, NTBSTRINGBIND,
                    pcol->size+1, (BYTE*)pcol->buffer);

            if (erc == FAIL) {
             //   ERR_LOG("%d dbbind failed", __LINE__);
                json_ret[0][0]["errorcode"] = "-1";
                json_ret[0][0]["errormsg"] = "操作失败";
                json_ret[1] = "error";
                //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
                free(columns);
                free(pcol->buffer);
                closeDB();
             //   ERR_LOG("operator-database failed 3.");
                //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
                return false;
            }

            erc = dbnullbind(m_dbprocess, c, &pcol->status);
            if (erc == FAIL) {
             //   ERR_LOG("%d dbnullbind failed", __LINE__);
                json_ret[0][0]["errorcode"] = "-1";
                json_ret[0][0]["errormsg"] = "操作失败";
                json_ret[1] = "error";
                //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
                free(columns);
                free(pcol->buffer);
                closeDB();
               // ERR_LOG("operator-database failed 4.");
                //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
                return false;
            }
        }

        //读取下一列数据
        while ((row_code = dbnextrow(m_dbprocess)) != NO_MORE_ROWS) { 
            Json::Value json_item;
            switch (row_code) {
                case REG_ROW:
                    for (pcol=columns; pcol - columns < ncols; pcol++) {
                        // char *buffer = pcol->status == -1?
                        // (char *)"NULL" : pcol->buffer;
                        // char *name = pcol->status == -1?
                        // (char *)"NULL" : pcol->name;
                        char *buffer = pcol->buffer;
                        char *name = pcol->name;
         //               DBG_LOG("pcol->size: %d, pcol->name: %s, pcol->buffer: %s", pcol->size, name, buffer);
                        json_item[name] = buffer;
                    }
                    json_ret[num].append(json_item);   //json字符串拼接
                    json_ret[1] = "ok";
                    break;

                case BUF_FULL:
                    assert(row_code != BUF_FULL);
                    break;

                case FAIL:
           //         ERR_LOG("operator dbresults failed");
                    json_ret[0][0]["errorcode"] = "-1";
                    json_ret[0][0]["errormsg"] = "操作失败";
                    json_ret[1] = "error";
                    //Add by huihai.shen at 2017-04-12 for solve memory leak bug begin
                    for (pcol=columns; pcol - columns < ncols; pcol++) {
                        free(pcol->buffer);
                    }
                    free(columns);
                    closeDB();
                   //Add by huihai.shen at 2017-04-12 for solve memory leak bug end
                    return false;
                    break;
            }
        }    
        for (pcol=columns; pcol - columns < ncols; pcol++) {
            free(pcol->buffer);
        }
        free(columns);

        if (DBCOUNT(m_dbprocess) > -1) {
         //   ERR_LOG("%d rows affected\n", DBCOUNT(m_dbprocess));
        }

        //确定一个存储过程或远程存储过程是否产生了返回状态号
        if (dbhasretstat(m_dbprocess) == TRUE) {
//            RUN_LOG("Procedure returned %d\n", dbretstatus(m_dbprocess));
        }
        num ++;
    }


    return true;
}

void CDbManager::closeDB()
{
    dbfreebuf(m_dbprocess);
    dbclose(m_dbprocess); 
    dbexit();
    is_free = true;
    is_conn = false;

    m_dbprocess = FAIL;

    return;  
}

int err_handler( DBPROCESS *dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr)
{ 
    if ((dbproc == NULL) || (DBDEAD(dbproc))) 
        return(INT_EXIT); 
    else  
    { 
 //       ERR_LOG("DB-Library error:\n\t%s\n",
   //         dberrstr); 
        if (oserr != DBNOERR) 
     //       ERR_LOG("Operating-system \
    //        error:\n\t%s\n", oserrstr); 
        return(INT_CANCEL); 
    } 
} 

int msg_handler( DBPROCESS *dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line )
{ 
    //ERR_LOG("Msg %ld, Level %d, State %d\n",  msgno, severity, msgstate); 
    if (strlen(srvname) > 0) 
      //  ERR_LOG("Server ’%s’, ", srvname); 
    if (strlen(procname) > 0) 
        //ERR_LOG("Procedure ’%s’, ", procname); 
    if (line > 0) 
        //ERR_LOG("Line %d", line); 

    //ERR_LOG("\n\t%s\n", msgtext); 

    return(0); 
} 









 
