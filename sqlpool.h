#ifndef __SQLPOOL_H_
#define __SQLPOOL_H_

#define IP_LEN      15
#define DBNAME_LEN  64
#define DBUSER_LEN  64
#define PASSWD_LEN  64
#define POOL_MAX_NUMBER 20
#include <mysql/mysql.h> 
#include <vector>
#include <string>
using namespace std;
typedef struct _SQL_NODE SQL_NODE;                /*���ӽڵ�*/
typedef struct _SQL_CONN_POOL SQL_CONN_POOL;      /*���ӳ�*/

/*���ӽڵ�*/
typedef struct _SQL_NODE{
    MYSQL            fd;                  /* mysql�����ļ������� */
    MYSQL            *mysql_sock;         /* ָ���Ѿ����ӵ�MYSQL��ָ�� */
    pthread_mutex_t  lock;                /* ������*/
    int              used;                /* ʹ�ñ�־ */
    int              index;               /* �±� */
    enum{
        DB_DISCONN, DB_CONN		  
    }sql_state;

}SQL_NODE;

/*���ӳ�*/
typedef struct _SQL_CONN_POOL{
    int        shutdown;                   /*�Ƿ�ر�*/
    SQL_NODE   sql_pool[POOL_MAX_NUMBER];  /* һ������ */
    int        pool_number;                /* �������� */
    int        busy_number;                /*����ȡ�˵���������*/
    char       ip[IP_LEN+1];               /* ���ݿ��ip */
    int        port;                       /* ���ݿ��port,һ����3306 */
    char       db_name[DBNAME_LEN+1];      /* ���ݿ������ */
    char       user[DBUSER_LEN+1];         /* �û��� */
    char       passwd[PASSWD_LEN+1];       /* ���� */
}SQL_CONN_POOL;

/*�������ӳ�*/
SQL_CONN_POOL *sql_pool_create(int connect_pool_number, const char* ip, int port, 
        const char* db_name, const char* user, const char* passwd);
/*�ڵ㴴������*/
int create_db_connect(SQL_CONN_POOL *sp, SQL_NODE *node);
/*�������ӳ�*/
void sql_pool_destroy(SQL_CONN_POOL *sp);
/*ȡ��һ��δʹ�õ�����*/
SQL_NODE *get_db_connect(SQL_CONN_POOL *sp);
/*�������*/
void release_node(SQL_CONN_POOL *sp, SQL_NODE *node);
/*���ӻ�ɾ������*/
SQL_CONN_POOL *changeNodeNum(SQL_CONN_POOL *sp, int op);

/*ִ������Ӧ��SQL���*/
bool execute_sql(SQL_NODE* node,const char* sql);
/*ִ���з��ص�SQL���*/
bool execute_sql_rtn(SQL_NODE * node,const char * sql,
				std::vector<std::vector<std::string> >& vResult);
/*ִ�д洢���̲���Ӧһ�����*/
/*bool execute_sql_procedure(SQL_NODE* node,const char* sql,
				std::vector<std::vector<std::string> >& vResult	);*/
bool PingMysql(SQL_CONN_POOL* sp);
std::string get_execute_errmsg(SQL_NODE* node);
#endif

