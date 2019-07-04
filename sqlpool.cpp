#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include "sqlpool.h"

//sql_pool_create(POOL_MAX_NUMBER, "localhost", 3306, "ceshi", "root", "qc123456");  
/*�������ӳ�*/
SQL_CONN_POOL *sql_pool_create(int connect_pool_number, const char* ip, int port, 
        const char* db_name, const char* user, const char* passwd){
    SQL_CONN_POOL *sp = NULL;
    if (connect_pool_number < 1){
        //printf("connect_pool_number < 1. defalut 1 \n");
        connect_pool_number = 1;
    }
    if ((sp=(SQL_CONN_POOL *)malloc(sizeof(SQL_CONN_POOL))) == NULL){
        //printf("malloc SQL_CONN_POOL error.\n");
        return NULL;
    }

    sp->shutdown    = 0; //�������ӳ�
    sp->pool_number = 0;
    sp->busy_number = 0;
    strcpy(sp->ip, ip);
    sp->port = port;
    strcpy(sp->db_name, db_name);
    strcpy(sp->user, user);
    strcpy(sp->passwd, passwd);

    /*��������*/
    if (connect_pool_number > POOL_MAX_NUMBER)
        connect_pool_number = POOL_MAX_NUMBER;

    for (int index=0; index < connect_pool_number; index++){
        //����ʧ��
        if (0 != create_db_connect(sp, &sp->sql_pool[index])){
            //�������ӳ�
            sql_pool_destroy(sp);
            return NULL;
        }
        //�����ɹ�
        sp->sql_pool[index].index = index;
        sp->pool_number++;
        //printf("create database pool connect:-%d-.\n",sp->sql_pool[index].index); 
    }

    return sp;
}
/*�ڵ㴴������*/
int create_db_connect(SQL_CONN_POOL *sp, SQL_NODE *node){
    int opt=1;
    int res=0; //0���� -1��ʼ��ʧ�� 1 ����ʧ��

    do {
        if (sp->shutdown == 1)
            return -1;
        /*����*/
        pthread_mutex_init(&node->lock, NULL);

        /*��ʼ��mysql����*/
        if (NULL == mysql_init(&node->fd)){
            //printf("mysql init error. \n");
            res = -1;
            break;
        }
        if (!(node->mysql_sock = mysql_real_connect(&node->fd, sp->ip, sp->user, sp->passwd, sp->db_name, sp->port, NULL, 0))){
            //printf("can not connect to mysql.\n");
            node->sql_state = SQL_NODE::DB_DISCONN;
            res = 1;
            break;
        }
        node->used = 0;
        node->sql_state = SQL_NODE::DB_CONN;
        //�����Զ����ӿ���
        mysql_options(&node->fd, MYSQL_OPT_RECONNECT, &opt);
        opt = 3;
        //�������ӳ�ʱʱ��Ϊ3s��3sδ���ӳɹ���ʱ
        mysql_options(&node->fd, MYSQL_OPT_CONNECT_TIMEOUT, &opt);
        res = 0;

    }while(0);

    return res;
}
/*�������ӳ�*/
void sql_pool_destroy(SQL_CONN_POOL *sp){
    //printf("destroy sql pool ... ... \n");

    sp->shutdown = 1; //�ر����ӳ�
    for (int index=0; index < sp->pool_number; index++){
        if (NULL != sp->sql_pool[index].mysql_sock){
            mysql_close(sp->sql_pool[index].mysql_sock);
            sp->sql_pool[index].mysql_sock = NULL;
        }
        sp->sql_pool[index].sql_state = SQL_NODE::DB_DISCONN; 
        sp->pool_number--;
    }
}
bool PingMysql(SQL_CONN_POOL* sp){
	bool bRet = false;
	SQL_NODE* node=NULL;
	node = get_db_connect(sp);
	if(NULL != node){
		bRet = true;
		release_node(sp,node);
	}
	return bRet;
}
/*ȡ��һ��δʹ�õ�����*/
SQL_NODE *get_db_connect(SQL_CONN_POOL *sp){
    //��ȡһ��δʹ�õ����ӣ������ֵ����index����֤ÿ�η���ÿ���ڵ�ĸ��ʻ�����ͬ
    int start_index = 0, index = 0, i;
    int ping_res;

    if (sp->shutdown == 1)
        return NULL;

    srand((int)time(0)); //���ݵ�ǰʱ�����������
    start_index = rand() % sp->pool_number; //���ʵĿ�ʼ��ַ

    for (i=0; i < sp->pool_number; i++){
        index = (start_index + i) % sp->pool_number;

        if (!pthread_mutex_trylock(&sp->sql_pool[index].lock)){
            if (SQL_NODE::DB_DISCONN == sp->sql_pool[index].sql_state){
                //��������
                if (0 != create_db_connect(sp, &(sp->sql_pool[index]))){
                    //��������ʧ��
                    release_node(sp, &(sp->sql_pool[index]));
                    continue;
                }
            }
            //���������Ƿ�ر�������
            ping_res = mysql_ping(sp->sql_pool[index].mysql_sock);
            if (0 != ping_res){
                //printf("mysql ping error.\n");
                sp->sql_pool[index].sql_state = SQL_NODE::DB_DISCONN;
                release_node(sp, &(sp->sql_pool[index]));
            } else {
                sp->sql_pool[index].used = 1;
                sp->busy_number++;//����ȡ��������1
                break ;  //ֻ��Ҫһ���ڵ�
            }
        }
    }

    if (i == sp->pool_number)
        return NULL;
    else
        return &(sp->sql_pool[index]);
}
/*�������*/
void release_node(SQL_CONN_POOL *sp, SQL_NODE *node){
    node->used = 0;
    sp->busy_number--;
    pthread_mutex_unlock(&node->lock);
}
/*���ӻ�ɾ������*/
SQL_CONN_POOL *changeNodeNum(SQL_CONN_POOL *sp, int op) { //���ӻ����5������
    int Num = 5;
    int index;
    int endindex;

    if (op == 1) { //����    0����
        endindex = sp->pool_number + Num;

        /*��������*/
        for (index=sp->pool_number; index < endindex; index++) {
            //����ʧ��
            if (0 != create_db_connect(sp, &sp->sql_pool[index])){
                //�������ӳ�
                sql_pool_destroy(sp);
                return NULL;
            }
            //�����ɹ�
            sp->sql_pool[index].index = index;                                                           
            sp->pool_number++;
            //printf("create database pool connect:-%d-.\n",sp->sql_pool[index].index); 
        }
    } else if (op == 0) {
        endindex = sp->pool_number - Num -1;
        //��������
        for (index=sp->pool_number-1; index>endindex && index>=0; index--) { 
            if (NULL != sp->sql_pool[index].mysql_sock) {
                mysql_close(sp->sql_pool[index].mysql_sock);
                sp->sql_pool[index].mysql_sock = NULL;
            }
            sp->sql_pool[index].sql_state = SQL_NODE::DB_DISCONN; 
            sp->pool_number--;
            //printf("delete database pool connect:-%d-.\n",sp->sql_pool[index].index);
        }
    }
    return sp;
}
/*ִ������Ӧ��SQL���*/
bool execute_sql(SQL_NODE* node,const char* sql){
	int res = mysql_real_query(node->mysql_sock,sql,strlen(sql));
	return (res == 0);
}
/*
bool execute_sql_procedure(SQL_NODE* node,const char* sql,
				std::vector<std::vector<std::string> >& vResult	){
	if(mysql_query(node->mysql_sock,sql)){
		return false;
	}
	mysql_query(node->mysql_sock,"select @t");
	MYSQL_RES* res = mysql_store_result(node->mysql_sock);
	if(NULL == res){
		mysql_free_result(res);
		return false;
	}
	int numcols = mysql_num_fields(res);
	MYSQL_ROW row;
	while(row = mysql_fetch_row(res)){
		std::vector<std::string> row_vec;
		row_vec.reserve(numcols);
		for(int nIndex = 0; nIndex < numcols;nIndex++){
			printf("%s ",row[nIndex]);
			//row_vec.push_back(row[nIndex]);
		}
		printf("\n");
		//vResult.push_back(row_vec);
	}
	mysql_free_result(res);
	return true;
}*/

/*ִ���з��ص�SQL���*/
bool execute_sql_rtn(SQL_NODE * node,const char * sql,
				std::vector<std::vector<std::string> >& vResult){
	if(!execute_sql(node,sql)){
		return false;
	}	
	MYSQL_RES* res = mysql_store_result(node->mysql_sock);
	if(NULL == res){
		mysql_free_result(res);
		return false;
	}
	int numcols = mysql_num_fields(res);
	MYSQL_ROW row;
	while(row = mysql_fetch_row(res)){
		std::vector<std::string> row_vec;
		row_vec.reserve(numcols);
		for(int nIndex = 0; nIndex < numcols;nIndex++){
			if(NULL == row[nIndex])
				row_vec.push_back("");
			else
				row_vec.push_back(row[nIndex]);
		}
		vResult.push_back(row_vec);
	}
	mysql_free_result(res);
	return true;
}
/*ִ�г�����Ϣ��ȡ*/
std::string get_execute_errmsg(SQL_NODE* node){
	char buf[2048] = {0};
	snprintf(buf,2047,"error_code=%d,error_msg=%s",mysql_errno(node->mysql_sock),mysql_error(node->mysql_sock));
	return buf;
}
/*
int main(){
	SQL_CONN_POOL *sp = 
        sql_pool_create(10, "localhost", 3306, "test", "root", "123456");  
    SQL_NODE *node  = get_db_connect(sp);
	if (NULL == node) {  
        printf("get sql pool node error.\n");  
        return -1;  
    } 
	//const char* sql = "call select_c1(1,@t);";
	char buff[1024] = {0};
    strcpy(buff,"call SelectAll('c1')");
    //sprintf(buff,"call cal_grade(%d,%d,@t,%f,%s,%s)",10,10,0.3,"123","456");
    std::vector<std::vector<std::string> > vResult;
	//if(!execute_sql_procedure(node,buff,vResult)){
	if(!execute_sql_rtn(node,buff,vResult)){
    	printf("error:%s\n",get_execute_errmsg(node).c_str());
		return -1;
	}
	for(int i=0; i< vResult.size();i++){
		for(int j=0; j< vResult[i].size();j++){
			printf("%s ",vResult[i][j].c_str());
		}
		printf("\n");
	}
	sql_pool_destroy(sp);
	return 0;
}
int main1()  
{  
    //MYSQL_FIELD *fd;  
    SQL_CONN_POOL *sp = 
        sql_pool_create(10, "localhost", 3306, "test", "root", "123456");  
    SQL_NODE *node  = get_db_connect(sp);  
    SQL_NODE *node2 = get_db_connect(sp);

    if (NULL == node) {  
        printf("get sql pool node error.\n");  
        return -1;  
    } 
    printf("--%d-- \n", node->index);
    printf("busy--%d--\n", sp->busy_number);

    if (mysql_query(&(node->fd), "select * from c1"))
    {  												     
        printf("query error.\n");  								  
        return -1;  									
    }
    else  
    {  
        printf("succeed!\n");  
    }
    changeNodeNum(sp, 0);//����
    changeNodeNum(sp, 1);//����
    sql_pool_destroy(sp);
    return 0;  
} */
