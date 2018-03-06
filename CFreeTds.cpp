#include "CFreeTds.h"
#include <stdio.h>
#include <string.h>

CDataSvrFreeTDS::CDataSvrFreeTDS():m_dbprocess(NULL){
	is_free = true;
}

CDataSvrFreeTDS::~CDataSvrFreeTDS(){
	if(!is_free ) {
		dbfreebuf(m_dbprocess);
		dbclose(m_dbprocess);
		dbexit();
	}
}

bool CDataSvrFreeTDS::init(const std::string & db_host, 
              const unsigned short db_port, 
              const std::string & db_name, 
              const std::string & db_user, 
              const std::string & db_pwd)
{
	char pConn_server[64] = {0};
	snprintf(pConn_server,sizeof(pConn_server),"%s:%u",db_host.c_str(),db_port);
	conn_server = pConn_server;
    host 		= db_host;
    port 		= db_port;
    database_name = db_name;
    user_name 	= db_user;
    user_passwd = db_pwd;
    return true;
}

bool CDataSvrFreeTDS::connect()
{
	if(dbinit() ==FAIL ){
		throw TdsException(TdsException::TDS_EXCEPT_INIT_ERR,"CDataSvrFreeTDS::connect(): dbinit => FAIL");
	}
	LOGINREC *dbloginerc = dblogin();
	if(dbloginerc == NULL){
		throw TdsException(TdsException::TDS_EXCEPT_LOGIN_ERR, 
            "CDataSvrFreeTDS::connect(): dblogin => FAIL");
	}
	DBSETLUSER(dbloginerc,user_name.c_str());
	DBSETLPWD(dbloginerc,user_passwd.c_str());
	m_dbprocess = dbopen(dbloginerc,conn_server.c_str());
	if(m_dbprocess == FAIL) {
		dbloginfree(dbloginerc);
		throw TdsException(TdsException::TDS_EXCEPT_OPEN_ERR,"CDataSvrFreeTDS::connect(): dbopen=>FAIL");
	}
	if(dbuse(m_dbprocess,database_name.c_str()) == FAIL ) {
		dbloginfree(dbloginerc);
		dbfreebuf(m_dbprocess);
		throw TdsException(TdsException::TDS_EXCEPT_DBUSE_ERR,"CDataSvrFreeTDS::connect(): dbuse()=>FAIL");
	}
	// dbloginfree(dbloginerc);
	// is_free = false;
	return true;
}

void CDataSvrFreeTDS::disconnect()
{
	dbfreebuf(m_dbprocess);
	dbclose(m_dbprocess);
	is_free = true;
}
bool CDataSvrFreeTDS::execSQL(const char *strSQL)
{
	if(dbcmd(m_dbprocess,strSQL) != SUCCEED ) {
		dbcancel(m_dbprocess);
		throw TdsException(TdsException::TDS_EXCEPT_DBCMD_ERR,"CDataSvrFreeTDS::execSQL(): dbcmd=>FAIL");
	}
	if(dbsqlexec(m_dbprocess) == FAIL ) {
		dbcancel(m_dbprocess);
		throw TdsException(TdsException::TDS_EXCEPT_DBEXEC_ERR,"CDataSvrFreeTDS::execSQL(): dbsqlexec=>FAIL");
	}
	return true;
}
int CDataSvrFreeTDS::getResult()
{
	int iRet = -1;
	while( (iRet = dbresults(m_dbprocess)) != NO_MORE_RESULTS) {
		if(iRet == FAIL) {
			dbcancel(m_dbprocess);
			throw TdsException(TdsException::TDS_EXCEPT_GETRESULT_ERR,"CDataSvrFreeTDS::getResult():dbresults=>FAIL");
		}
		return 1;
	}
	return 0;
}


bool CDataSvrFreeTDS::bindColumn(int column, int varLen, char *varAddr)
{
	if(dbbind(m_dbprocess,column,STRINGBIND,(DBINT)varLen,(BYTE*)varAddr) == SUCCEED ) {
		return true;
	}else {
		throw TdsException(TdsException::TDS_EXCEPT_BIND_ERR,"CDataSvrFreeTDS::bindColumn(): dbbind=>FAIL");
	}
}

bool CDataSvrFreeTDS::bindColumn(int column, int varLen, int *varAddr)
{
	if(dbbind(m_dbprocess,column,INTBIND,(DBINT)varLen,(BYTE*)varAddr) == SUCCEED ) {
		return true;
	}else {
		throw TdsException(TdsException::TDS_EXCEPT_BIND_ERR,"CDataSvrFreeTDS::bindColumn(): dbbind=>FAIL");
	}
}

bool CDataSvrFreeTDS::moveNextRow()
{
	int row_code = dbnextrow(m_dbprocess);
	if(row_code == NO_MORE_ROWS) {
		return false;
	}else if(row_code == REG_ROW) {
		return true;
	}else if(row_code == BUF_FULL ) {
		throw TdsException(TdsException::TDS_EXCEPT_BUFFULL_ERR,"CDataSvrFreeTDS::moveNextRow() : BUF_FULL");
	}else{
		dbcancel(m_dbprocess);
		throw TdsException(TdsException::TDS_EXCEPT_NEXTROW_ERR,"CDataSvrFreeTDS::moveNextRow(): NEXTROW_ERR");
	}

}

int CDataSvrFreeTDS::countRow()
{
	return int(dbcount(m_dbprocess));
}

bool CDataSvrFreeTDS::isCount()
{
	if(dbiscount(m_dbprocess) == TRUE) {
		return true;
	}
	return false;
}

bool CDataSvrFreeTDS::cancel()
{
	dbcancel(m_dbprocess);
	return true;
}
