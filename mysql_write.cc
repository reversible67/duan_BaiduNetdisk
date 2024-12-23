#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
// д���͵�mysql����
static WFFacilities::WaitGroup waitGroup(1);
void callback(WFMySQLTask *mysqlTask){
    // ������Ӵ���
    if(mysqlTask->get_state() != WFT_STATE_SUCCESS){
        fprintf(stderr, "error msg:%s\n", WFGlobal::get_error_string(mysqlTask->get_state(), mysqlTask->get_error()));
        return;
    }
    // �鿴ָ���Ƿ�ִ�гɹ�  �õ���Ӧ
    protocol::MySQLResponse *resp = mysqlTask->get_resp();
    protocol::MySQLResultCursor cursor(resp);
    // ��ȡ��������
    // ����﷨����
    if(resp->get_packet_type() == MYSQL_PACKET_ERROR){
        fprintf(stderr, "error_code = %d msg = %s\n", resp->get_error_code(), resp->get_error_msg().c_str());
    }

    if(cursor.get_cursor_status() == MYSQL_STATUS_OK){
        // дָ�ִ�гɹ�
        fprintf(stderr, "OK. %llu rows affected. %d warnings. insert_id = %llu.\n", cursor.get_affected_rows(), cursor.get_warnings(), cursor.get_insert_id());
    }
}
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
int main(){
    signal(SIGINT, sigHandler);
    // ����mysql����
    auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@127.0.0.1:3306", 0, callback);
    // һ��Ҫ�����ݿ������д����
    std::string sql = "insert into cloudisk.tbl_user_token (user_name, user_token) values('test', 'wqrwqefefgweg72392dhf37fw');";
    // mysql������
    auto req = mysqlTask->get_req();
    req->set_query(sql);
    // ����mysqlTask
    mysqlTask->start();
    waitGroup.wait();
}