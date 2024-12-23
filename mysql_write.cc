#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
// 写类型的mysql任务
static WFFacilities::WaitGroup waitGroup(1);
void callback(WFMySQLTask *mysqlTask){
    // 检查连接错误
    if(mysqlTask->get_state() != WFT_STATE_SUCCESS){
        fprintf(stderr, "error msg:%s\n", WFGlobal::get_error_string(mysqlTask->get_state(), mysqlTask->get_error()));
        return;
    }
    // 查看指令是否执行成功  拿到响应
    protocol::MySQLResponse *resp = mysqlTask->get_resp();
    protocol::MySQLResultCursor cursor(resp);
    // 获取报错类型
    // 检查语法错误
    if(resp->get_packet_type() == MYSQL_PACKET_ERROR){
        fprintf(stderr, "error_code = %d msg = %s\n", resp->get_error_code(), resp->get_error_msg().c_str());
    }

    if(cursor.get_cursor_status() == MYSQL_STATUS_OK){
        // 写指令，执行成功
        fprintf(stderr, "OK. %llu rows affected. %d warnings. insert_id = %llu.\n", cursor.get_affected_rows(), cursor.get_warnings(), cursor.get_insert_id());
    }
}
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
int main(){
    signal(SIGINT, sigHandler);
    // 创建mysql任务
    auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@127.0.0.1:3306", 0, callback);
    // 一定要把数据库的名字写出来
    std::string sql = "insert into cloudisk.tbl_user_token (user_name, user_token) values('test', 'wqrwqefefgweg72392dhf37fw');";
    // mysql请求报文
    auto req = mysqlTask->get_req();
    req->set_query(sql);
    // 启动mysqlTask
    mysqlTask->start();
    waitGroup.wait();
}