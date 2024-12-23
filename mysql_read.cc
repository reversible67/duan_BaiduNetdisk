#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/MySQLMessage.h>
// 读类型的mysql任务
static WFFacilities::WaitGroup waitGroup(1);
void callback(WFMySQLTask *mysqlTask){
    // 检查连接错误
    if(mysqlTask->get_state() != WFT_STATE_SUCCESS){
        fprintf(stderr, "error msg:%s\n", WFGlobal::get_error_string(mysqlTask->get_state(), mysqlTask->get_error()));
        return;
    }
    // 查看指令是否执行成功  拿到响应
    protocol::MySQLResponse *resp = mysqlTask->get_resp();
    // cursor迭代器 遍历resp
    protocol::MySQLResultCursor cursor(resp);

    // 获取报错类型
    // 检查语法错误
    if(resp->get_packet_type() == MYSQL_PACKET_ERROR){
        fprintf(stderr, "error_code = %d msg = %s\n", resp->get_error_code(), resp->get_error_msg().c_str());
    }
    do{
        if(cursor.get_cursor_status() == MYSQL_STATUS_OK){
        // 写指令，执行成功
        fprintf(stderr, "OK. %llu rows affected. %d warnings. insert_id = %llu.\n", cursor.get_affected_rows(), cursor.get_warnings(), cursor.get_insert_id());
        }
        else if(cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT){
            // 读指令，执行成功
            // 1.域的数量，名字，类型
            // 2.若干个cell ---> 一行     若干行 ---> 表
            // vector<vector<cells>>
            // 把所有域信息构成一个数组
            const protocol::MySQLField *const *fields = cursor.fetch_fields();
            // 访问所有列数
            for(int i = 0; i < cursor.get_field_count(); ++i){
                // db table name type
                fprintf(stderr, "db = %s, table = %s, name = %s, type = %s\n", fields[i]->get_db().c_str(), 
                                                                               fields[i]->get_table().c_str(),
                                                                               fields[i]->get_name().c_str(),
                                                                               datatype2str(fields[i]->get_data_type()));                                    
            }

            // 全部取出来
            // cursor.fetch_all();
            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);
            // 遍历二维数组
            for(auto &row : rows){
                for(auto &cell : row){
                    if(cell.is_int()){
                        printf("[%d]", cell.as_int());
                    }
                    else if(cell.is_ulonglong()){
                        printf("[%llu]", cell.as_ulonglong());
                    }
                    else if(cell.is_string()){
                        printf("[%s]", cell.as_string().c_str());
                    }
                    else if(cell.is_datetime()){
                        printf("[%s]", cell.as_datetime().c_str());
                    }
                }
                printf("\n");
            }
            // 把每一行取出来
            // cursor.fetch_row();
        }
    } while(cursor.next_result_set());
}
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
int main(){
    signal(SIGINT, sigHandler);
    // 创建mysql任务
    auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@127.0.0.1:3306", 0, callback);
    // std::string sql = "show databases;";
    // 假如此时我想要先update 再select  可以考虑创建两个任务 一个用来update(写) 一个用来select(读) 然后加入序列 串行执行
    // 也可以采用下面这种写法 ---> 两条语句
    // std::string sql = "select * from cloudisk.tbl_user_token;";
    std::string sql = "insert into cloudisk.tbl_user_token (user_name, user_token) values('testxxx', 'wqrwqefefgweg72392dhf37fw');"
    "select * from cloudisk.tbl_user_token;";
    // mysql请求报文
    auto req = mysqlTask->get_req();
    req->set_query(sql);
    // 启动mysqlTask
    mysqlTask->start();
    waitGroup.wait();
}