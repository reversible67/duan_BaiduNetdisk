#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/MySQLMessage.h>
#include <nlohmann/json.hpp>
#include "FileUtil.h"
#include <wfrest/HttpMsg.h>
#include "UserInfo.h"
#include "Token.h"
static WFFacilities::WaitGroup waitGroup(1);
using Json = nlohmann::json;
void callback(WFMySQLTask *mysqlTask){
}
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
int main(){
    signal(SIGINT, sigHandler);
    wfrest::HttpServer server;
    // 显示上传界面
    server.GET("/file/upload", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/index.html");
    });
    // 上传文件
    server.POST("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // 从URL中提取用户名的信息
        auto userInfo = req->query_list();
        std::string username = userInfo["username"];
        // 读取文件内容 解析form-data类型的请求报文
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        Form &form = req->form();
        auto fileInfo = form["file"];
        // fileInfo.first = 文件名  fileInfo.second = 文件内容
        // tmp目录存放所有文件  以及用于生成文件路径
        std::string filepath = "./tmp/" + fileInfo.first;
        // 通过open系统调用进行创建文件
        // 加上O_EXCL 上传重复文件会报错
        int fd = open(filepath.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
        if(fd < 0){
            // 报错
            resp->set_status_code("500");
        }
        // 还可以采用异常机制
        // try {} catch ()
        // 将读到的所有内容 写进去
        // 方案1 通过write系统调用 但是是同步的 效率较低
        int ret = write(fd, fileInfo.second.c_str(), fileInfo.second.size());
        close(fd);
        // 方案2 pwrite的任务 异步框架
        // auto pwriteTask = WFTaskFactory::create_pwrite_task(fd, fileInfo.second.c_str(), fileInfo.second.size(), 0, callback);
        // series->push_back(pwriteTask);
        // 把上传文件之后的逻辑写到callback当中

        // for(auto it : form){
        //     fprintf(stderr, "it.first = %s, it.second.first = %s, it.second.second = %s\n", it.first.c_str(),
        //                                                                                     it.second.first.c_str(),
        //                                                                                     it.second.second.c_str());
        // }


        // fprintf(stderr, "sha1 = %s\n", FileUtil::sha1File(filepath.c_str()).c_str());

        std::string sql = "INSERT INTO cloudisk.tbl_file (file_sha1,file_name, file_size, file_addr, status) VALUES('" 
                + FileUtil::sha1File(filepath.c_str()) + "',' "
                + fileInfo.first + "',"
                + std::to_string(fileInfo.second.size()) + ",'"
                + filepath + "', 0)";
        sql += "INSERT INTO cloudisk.tbl_user_file (user_name, file_sha1, file_name, file_size) VALUES ('"
                + username + "','"
                + FileUtil::sha1File(filepath.c_str()) + "','"
                + fileInfo.first + "',"
                + std::to_string(fileInfo.second.size()) + ");";
        fprintf(stderr, "sql = %s\n", sql.c_str());
        // resp->MySQL("mysql://root:123456@localhost", sql, [](Json *pjson){
        //     fprintf(stderr, "out = %s\n", pjson->dump().c_str());
        // });
        // 如果上传成功啦  设置302（重定向）
        resp->set_status_code("302");
        // 把location所对应的键值改为 /file/upload/success
        resp->headers["Location"] = "/file/upload/success";
    });
    server.GET("/file/upload/success", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->String("Upload success");
    });
    // 实现文件下载功能
    server.GET("/file/download", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // /file/download?filehash=aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d&filename=1.txt&filesize=5
        // 解析用户请求操作
        auto fileInfo = req->query_list();
        std::string filesha1 = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        // 打开文件
        std::string filepath = "tmp/" + filename;
        int fd = open(filepath.c_str(), O_RDONLY);
        // 申请一片内存 把文件内容放进去 移到响应报文的报文体中
        // 有可能内容很大 所以在堆上申请
        // 获取文件大小 lseek 移动到文件末尾的距离
        int size = lseek(fd, 0, SEEK_END);
        // 用完之后再偏移回去
        lseek(fd, 0, SEEK_SET);
        std::unique_ptr<char []> buf(new char[size]);
        // 读进来 一次性将文件内容全部读取进来
        read(fd, buf.get(), size);
        // 设置响应体 发送回客户端
        resp->append_output_body(buf.get(), size);
        // 默认下载
        resp->headers["Content-Type"] = "application/octect-stream";
        // 指定默认下载的文件名
        resp->headers["content-disposition"] = "attachment;filename="+filename;
    });
    
   // 使用nginx服务器
    /*server.GET("/file/download", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        auto fileInfo = req->query_list();
        std::string filesha1 = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        resp->set_status_code("302");
        resp->headers["Location"] = "http://1.94.50.145:1235/" + filename;
    });*/
    // 引入数据库 数据库需要哪些表呢？
    // 1.全局文件表   把所有的文件信息记录下来  文件名 文件哈希值 文件大小 文件位置... tbl_file
    /*
    分库分表和主从复制应用场景
    分库分表是为了应对数据量过大的挑战
    主从复制是为了提高读性能，提供容错功能，提高可用性
    */
    // 可以把表垂直分裂 常用的信息放在tb1 不常用的放在tb2
    /*
    垂直分片的优点：
    1.提高查询性能：只查询所需的字段，减少了数据的读取和传输量，提高了查询效率。 
    2.减少冗余数据：每个数据库或表只包含相关的数据，避免了存储冗余字段的开销。 3.
    降低数据库负载：将不同业务关联性不强的表分散到不同的数据库中，可以降低单一数据库的负载压力，提高数据库的并发处理能力。
    垂直分片的缺点：
    1.部分时候查询效率低 需要join
    */
    /*
    水平分片：两个表结构是一样的  若干行属于tb1  若干行属于tb2
    可以解决数据量过大的问题
    */
    // 2.用户表  tbl_user
    // 3.用户文件表  tbl_user_file
    // 4.token表   每一次登录以后  给客户端一个凭证  用于服务端校正你有没有这个资格  token有效时间：1个星期-1个月 最好放入redis EXPIRE指令
    // 注册界面
    server.GET("/user/signup", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("static/view/signup.html");
    });
    server.POST("/user/signup", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // 1.按urlencoded的形式去解析post报文体
        std::map<std::string, std::string> &form_kv = req->form_kv();
        std::string username = form_kv["username"];
        std::string password = form_kv["password"];

        // 2 得到SignupService的IP:PORT
        std::string url = "http://1.94.50.145:8500/v1/agent/services";
        auto httpTask = WFTaskFactory::create_http_task(url, 0, 0, [username, password, resp](WFHttpTask *httpTask){
            // 解析consul的响应内容
            auto consulResp = httpTask->get_resp();
            const void *body;
            size_t size;
            consulResp->get_parsed_body(&body, &size);
            Json services = Json::parse(static_cast<const char *>(body));
            std::string ip = services["SignupService1"]["Address"];
            unsigned short port = services["SignupService1"]["Port"];
            // 3 发起RPC调用
            // 创建一个代理对象
            UserService::SRPCClient client(ip.c_str(), port);
            // 创建一个请求
            ReqSignup reqSignup;
            reqSignup.set_username(username);
            reqSignup.set_password(password);
            // 通过代理对象的方法，调用rpc
            // 使用任务的形式来使用客户端
            auto rpcTask = client.create_Signup_task([resp](RespSignup *response, srpc::RPCContext *ctx){
                if(ctx->success() && response->code() == 0){
                    resp->String("SUCCESS");
                }
                else{
                    fprintf(stderr, "status = %d, error = &d, errmsg = %s\n",
                    ctx->get_status_code(), ctx->get_error(), ctx->get_errmsg());
                    resp->String("FAIL");
                }
            });
            // 修改任务的属性以设置rpc传入参数
            rpcTask->serialize_input(&reqSignup);
            // 启动任务
            series_of(httpTask)->push_back(rpcTask);
        });
        // 这个httpTask去访问consul
        series->push_back(httpTask);
        
        /* // 2.把密码进行加密
        std::string salt = "12345678";
        char *encryptPassword = crypt(password.c_str(), salt.c_str());
        fprintf(stderr, "encryptPassword = %s\n", encryptPassword);
        // 3.把用户信息插入到数据库
        std::string sql = "INSERT INTO cloudisk.tbl_user (user_name, user_pwd) VALUES ('"+ username + "', '" + encryptPassword + "');";
        fprintf(stderr, "sql = %s\n", sql.c_str());
        // create_mysql_task
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [](WFMySQLTask *mysqlTask){
            // 4.回复一个SUCCESS给前端
            wfrest::HttpResp *resp2client = static_cast<wfrest::HttpResp *>(mysqlTask->user_data);
            if(mysqlTask->get_state() != WFT_STATE_SUCCESS){
                fprintf(stderr, "error msg:%s\n", WFGlobal::get_error_string(mysqlTask->get_state(), mysqlTask->get_error()));
                
                resp2client->append_output_body("FAIL", 4);
                return;
            }
            // 查看指令是否执行成功  拿到响应
            protocol::MySQLResponse *resp = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(resp);
            // 获取报错类型
            // 检查语法错误
            if(resp->get_packet_type() == MYSQL_PACKET_ERROR){
                fprintf(stderr, "error_code = %d msg = %s\n", resp->get_error_code(), resp->get_error_msg().c_str());
                resp2client->append_output_body("FAIL", 4);
                return;
            }

            if(cursor.get_cursor_status() == MYSQL_STATUS_OK){
                // 写指令，执行成功
                fprintf(stderr, "OK. %llu rows affected. %d warnings. insert_id = %llu.\n", cursor.get_affected_rows(), cursor.get_warnings(), cursor.get_insert_id());
                if(cursor.get_affected_rows() == 1){
                    resp2client->append_output_body("SUCCESS", 7);
                    return;
                }
            }
        });
        mysqlTask->get_req()->set_query(sql);
        mysqlTask->user_data = resp;
        // push_back
        series->push_back(mysqlTask); */
    });
    server.GET("/static/view/signin.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("static/view/signin.html");
    });
    server.GET("/static/view/home.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("static/view/home.html");
    });
    server.GET("/static/js/auth.js", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("static/js/auth.js");
    });
    server.GET("/static/img/avatar.jepg", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("static/img/avatar.jepg");
    });
    server.POST("/user/signin", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // 1.解析用户请求
        std::map<std::string, std::string> &form_kv = req->form_kv();
        std::string username = form_kv["username"];
        std::string password = form_kv["password"];
        // 2.查询数据库
        std::string url = "mysql://root:123456@localhost";
        std::string sql = "SELECT user_pwd FROM cloudisk.tbl_user WHERE user_name = '" + username + "' LIMIT 1;";
        auto readTask = WFTaskFactory::create_mysql_task(url, 0, [](WFMySQLTask *readTask){
            // 提取readTask的结果
            auto resp = readTask->get_resp();
            // 作为迭代器 获取结果
            protocol::MySQLResultCursor cursor(resp);

            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);

            // 数据库中的密码
            std::string nowPassword = rows[0][0].as_string();
            fprintf(stderr, "nowPassword = %s\n", nowPassword.c_str());

            UserInfo *userinfo = static_cast<UserInfo *>(series_of(readTask)->get_context());
            // 用户输入的密码
            char * inPassword = crypt(userinfo->password.c_str(), "12345678");
            fprintf(stderr, "inPassword = %s\n", inPassword);
            if(strcmp(nowPassword.c_str(), inPassword) != 0){
                userinfo->resp->append_output_body("FAIL", 4);
                return;
            }
            // 3.生成一个token，存入数据库中
            // 根据用户的信息 经过加密得到密文  拼接上登陆时间 合在一起就是token
            // 盐值固定 为12345678
            Token usertoken(userinfo->username, "12345678");
            fprintf(stderr, "token = %s\n", usertoken.token.c_str());
            userinfo->token = usertoken.token;
            // 存入数据库当中
            std::string url = "mysql://root:123456@localhost";
            // replace 相当于 insert 和 update的结合
            std::string sql = "REPLACE INTO cloudisk.tbl_user_token (user_name, user_token) VALUES ('" 
                + userinfo->username + "', '" 
                + usertoken.token + "');";
            auto writeTask = WFTaskFactory::create_mysql_task(url, 0, [readTask](WFMySQLTask *writeTask){
                // 这里没有做检查  假设成功
                // // 4.将信息包装成Json 返回给客户端
                UserInfo *userinfo = static_cast<UserInfo *>(series_of(readTask)->get_context());
                Json uinfo;
                uinfo["Username"] = userinfo->username;
                uinfo["Token"] = userinfo->token;
                uinfo["Location"] = "/static/view/home.html";
                Json respInfo;
                respInfo["code"] = 0;
                respInfo["msg"] = "OK";
                respInfo["data"] = uinfo;
                userinfo->resp->String(respInfo.dump());
            });
            writeTask->get_req()->set_query(sql);
            series_of(readTask)->push_back(writeTask);
        });
        readTask->get_req()->set_query(sql);
        series->push_back(readTask);
        UserInfo *userinfo = new UserInfo;
        userinfo->username = username;
        userinfo->password = password;
        userinfo->resp = resp;
        series->set_context(userinfo);
        // 在序列的回调函数中释放
        series->set_callback([series](const SeriesWork *series){
            // delete;
            UserInfo *userinfo = static_cast<UserInfo *> (series_of()->get_context());
            delete userinfo;
            fprintf(stderr, "userinfo is deleted\n");
        });
        
    });
    // 用来刷新 查询用户信息
    server.POST("/user/info", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // 查询用户信息需要用到数据库 所以需要mysql任务
        // 1 解析用户请求
        auto userInfo = req->query_list();
        // 2 校验token是否合法 ---> 拦截器
        // 3 根据用户信息，查询sql
        std::string sql = "SELECT user_name, signup_at FROM cloudisk.tbl_user WHERE user_name='" 
                          + userInfo["username"] + "' LIMIT 1;";
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [resp](WFMySQLTask *mysqlTask){
            auto respMysql = mysqlTask->get_resp();
            // 作为迭代器 获取结果
            protocol::MySQLResultCursor cursor(respMysql);
            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);
            fprintf(stderr, "username = %s, signupat = %s\n", rows[0][0].as_string().c_str(), rows[0][1].as_datetime().c_str());
            Json uInfo;
            uInfo["Usernam"] = rows[0][0].as_string();
            uInfo["SignupAt"] = rows[0][1].as_datetime();
            Json respInfo;
            respInfo["data"] = uInfo;
            respInfo["code"] = 0;
            respInfo["msg"] = "OK";
            // 返回给用户  这样页面就能弹出用户的信息
            resp->String(respInfo.dump());
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    });
    server.POST("/file/query", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // 解析用户请求
        auto userInfo = req->query_list();
        std::string username = userInfo["username"];
        auto form_kv = req->form_kv();
        std::string limit = form_kv["limit"];
        // 根据用户名查tbl_user_file
        std::string sql = "SELECT file_sha1,file_name,file_size,upload_at,last_update FROM cloudisk.tbl_user_file WHERE user_name = '"
                          + username + "' LIMIT " + limit + ";";
        fprintf(stderr, "sql = %s\n", sql.c_str());
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [resp](WFMySQLTask *mysqlTask){
            auto respMysql = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(respMysql);
            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);
            Json respArr;
            for(auto &row : rows){
                Json fileJson;
                // row[0] file_sha1
                fileJson["FileHash"] = row[0].as_string();
                // row[1] file_name
                fileJson["FileName"] = row[1].as_string();
                // row[2] file_size
                fileJson["FileSize"] = row[2].as_ulonglong();
                // row[3] upload_at
                fileJson["UploadAt"] = row[3].as_datetime();
                // row[4] lastupdate
                fileJson["LastUpdated"] = row[4].as_datetime();
                respArr.push_back(fileJson);
            }
            fprintf(stderr, "out = %s\n", respArr.dump().c_str());
            resp->String(respArr.dump());
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    });
    if(server.track().start(1234) == 0){
        server.list_routes();
        waitGroup.wait();
        server.stop();
    }
    else{
        fprintf(stderr, "can not start server!\n");
        return -1;
    }
}
