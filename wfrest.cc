#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/MySQLMessage.h>
static WFFacilities::WaitGroup waitGroup(1);
// wfrest框架
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
    wfrest::HttpResp *resp2client = static_cast<wfrest::HttpResp *>(series_of(mysqlTask)->get_context());
    resp2client->String("Mysql OK");
}
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
int main(){
    signal(SIGINT, sigHandler);
    wfrest::HttpServer server;
    server.GET("/test", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->String("hello");
    });
    server.POST("/test", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->String("hello");
    });
    // 实现重定向
    server.GET("/redirect", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // 把状态码设置为302
        resp->set_status_code("302");
        // 加一个location字段
        // resp->add_header_pair("Location", "/test");
        resp->headers["Location"] = "/test";  // 跟上面的效果一样
    });
    // 解析请求
    server.GET("/test1", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        std::map<std::string, std::string> queryMap = req->query_list();
        for(auto it : queryMap){
            fprintf(stderr, "first = %s, second = %s\n", it.first.c_str(), it.second.c_str());
        }
    });
    // 登录操作 & 提取参数
    server.POST("/login", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        const std::string &username = req->query("username");
        const std::string &password = req->query("password");
        fprintf(stderr, "username = %s, password = %s\n", username.c_str(), password.c_str());
    });
    // 以body的形式 传递信息给服务端
    server.POST("/login1", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        if(req->content_type() != wfrest::APPLICATION_URLENCODED){
            resp->set_status_code("500");
            return;
        }
        std::map<std::string, std::string> formMap = req->form_kv();
        for(auto it : formMap){
            fprintf(stderr, "first = %s, second = %s\n", it.first.c_str(), it.second.c_str());
        }
    });
    // formdata一般用来传递大信息 urlencoded一般传递小信息
    server.POST("/formdata", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        if(req->content_type() != wfrest::MULTIPART_FORM_DATA){
            resp->set_status_code("500");
            return;
        }
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        const Form &form = req->form();
        for(auto it : form){
            fprintf(stderr, "it.first = %s, it.second.first = %s, it.second.second = %s\n", it.first.c_str(),
                                                                                            it.second.first.c_str(),
                                                                                            it.second.second.c_str());
        }
    });
    // html   GET请求 返回网页
    server.GET("/post1.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        int fd = open("post1.html", O_RDONLY);
        std::unique_ptr<char []> buf(new char[3922]);
        read(fd, buf.get(), 3922);
        resp->append_output_body(buf.get(), 3922);
        // 这样就可以显示出网页了
        resp->headers["Content-Type"] = "text/html";
    });
    // 通过post进行上传文件 
    server.POST("/post1.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        if(req->content_type() != wfrest::MULTIPART_FORM_DATA){
            resp->set_status_code("500");
            return;
        }
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        const Form &form = req->form();
        for(auto it : form){
            fprintf(stderr, "it.first = %s, it.second.first = %s, it.second.second = %s\n", it.first.c_str(),
                                                                                            it.second.first.c_str(),
                                                                                            it.second.second.c_str());
        }
    });
    // wfrest中的序列
    server.GET("/series", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // series就是服务端任务所在的序列
        auto timerTask = WFTaskFactory::create_timer_task(5 * 1000000, [resp](WFTimerTask *timerTask){
            resp->String("time is over");
        });
        // 将timerTask加入序列
        series->push_back(timerTask);
        // timerTask->start();  // 不行的
    });
    // mysql
    server.GET("/mysql0", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@127.0.0.1:3306", 0, callback);
        std::string sql = "insert into cloudisk.tbl_user_token (user_name, user_token) values ('test7', 'adwqefwefgwegwgwg');"
        "select * from cloudisk.tbl_user_token;";
        auto mysqlReq = mysqlTask->get_req();
        mysqlReq->set_query(sql);
        series->push_back(mysqlTask);
        // 把resp传递过去 在回调中使用
        series->set_context(resp);
    });
    // .track() 打印出运行的状态
    if(server.track().start(1234) == 0){
        // 当前服务端支持哪些方法 和 哪些路径
        server.list_routes();
        waitGroup.wait();
        server.stop();
    }
    else{
        fprintf(stderr, "can not start server!\n");
        return -1;
    }
}