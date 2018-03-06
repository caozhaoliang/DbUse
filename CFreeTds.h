#ifndef _DATASVR_FREETDS_H_
#define _DATASVR_FREETDS_H_


#include <string>
#include <sybfront.h> //freetdsÍ·ÎÄ¼þ  
#include <sybdb.h>    //freetds 

using namespace std;


class CDataSvrFreeTDS
{
public:
    CDataSvrFreeTDS();
    ~CDataSvrFreeTDS();

    bool init(const std::string & db_host, 
              const unsigned short db_port, 
              const std::string & db_name, 
              const std::string & db_user, 
              const std::string & db_pwd);
    bool connect();
    void disconnect();
    bool execSQL(const char *strSQL);
    int  getResult();
    bool bindColumn(int column, int varLen, char *varAddr);
    bool bindColumn(int column, int varLen, int *varAddr);
    bool moveNextRow();
    int  countRow();
    bool isCount();
	bool cancel();

private:
    DBPROCESS *m_dbprocess;
    bool is_free;

    std::string     conn_server;
    std::string     host;
    unsigned short  port;
    std::string     database_name;
    std::string     user_name;
    std::string     user_passwd;
};

class TdsException : public std::exception
{
public:
    enum ETdsExceptionType
    {
        TDS_EXCEPT_NONE_ERR = 0,
        TDS_EXCEPT_INIT_ERR,
        TDS_EXCEPT_LOGIN_ERR,
        TDS_EXCEPT_OPEN_ERR,
        TDS_EXCEPT_DBUSE_ERR,
        TDS_EXCEPT_DBCMD_ERR,
        TDS_EXCEPT_DBEXEC_ERR,
        TDS_EXCEPT_GETRESULT_ERR,
        TDS_EXCEPT_BIND_ERR,
        TDS_EXCEPT_BUFFULL_ERR,
        TDS_EXCEPT_NEXTROW_ERR,
        TDS_EXCEPT_UNKOWN_ERR
    };
public:
	//explicit TdsException(const char* msg) {
    explicit TdsException(ETdsExceptionType err_type, const char* msg) {
        except_type = err_type;
		errMsg.assign(msg);
	}
	virtual const char* what() const throw() {
		return errMsg.c_str();
	}
	
    virtual ETdsExceptionType exceptType() const throw() {
		return except_type;
	}

	virtual ~TdsException() throw()
	{
		
	}

private:
	std::string errMsg;
    ETdsExceptionType except_type;
};


#endif
