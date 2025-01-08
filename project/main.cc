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
    // ��ʾ�ϴ�����
    server.GET("/file/upload", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/index.html");
    });
    // �ϴ��ļ�
    server.POST("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // ��URL����ȡ�û�������Ϣ
        auto userInfo = req->query_list();
        std::string username = userInfo["username"];
        // ��ȡ�ļ����� ����form-data���͵�������
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        Form &form = req->form();
        auto fileInfo = form["file"];
        // fileInfo.first = �ļ���  fileInfo.second = �ļ�����
        // tmpĿ¼��������ļ�  �Լ����������ļ�·��
        std::string filepath = "./tmp/" + fileInfo.first;
        // ͨ��openϵͳ���ý��д����ļ�
        // ����O_EXCL �ϴ��ظ��ļ��ᱨ��
        int fd = open(filepath.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
        if(fd < 0){
            // ����
            resp->set_status_code("500");
        }
        // �����Բ����쳣����
        // try {} catch ()
        // ���������������� д��ȥ
        // ����1 ͨ��writeϵͳ���� ������ͬ���� Ч�ʽϵ�
        int ret = write(fd, fileInfo.second.c_str(), fileInfo.second.size());
        close(fd);
        // ����2 pwrite������ �첽���
        // auto pwriteTask = WFTaskFactory::create_pwrite_task(fd, fileInfo.second.c_str(), fileInfo.second.size(), 0, callback);
        // series->push_back(pwriteTask);
        // ���ϴ��ļ�֮����߼�д��callback����

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
        resp->MySQL("mysql://root:123456@localhost", sql, [](Json *pjson){
            fprintf(stderr, "out = %s\n", pjson->dump().c_str());
        });
        // ����ϴ��ɹ���  ����302���ض���
        resp->set_status_code("302");
        // ��location����Ӧ�ļ�ֵ��Ϊ /file/upload/success
        resp->headers["Location"] = "/file/upload/success";
    });
    server.GET("/file/upload/success", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->String("Upload success");
    });
    // ʵ���ļ����ع���
    server.GET("/file/download", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // /file/download?filehash=aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d&filename=1.txt&filesize=5
        // �����û��������
        auto fileInfo = req->query_list();
        std::string filesha1 = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        // ���ļ�
        std::string filepath = "tmp/" + filename;
        int fd = open(filepath.c_str(), O_RDONLY);
        // ����һƬ�ڴ� ���ļ����ݷŽ�ȥ �Ƶ���Ӧ���ĵı�������
        // �п������ݺܴ� �����ڶ�������
        // ��ȡ�ļ���С lseek �ƶ����ļ�ĩβ�ľ���
        int size = lseek(fd, 0, SEEK_END);
        // ����֮����ƫ�ƻ�ȥ
        lseek(fd, 0, SEEK_SET);
        std::unique_ptr<char []> buf(new char[size]);
        // ������ һ���Խ��ļ�����ȫ����ȡ����
        read(fd, buf.get(), size);
        // ������Ӧ�� ���ͻؿͻ���
        resp->append_output_body(buf.get(), size);
        // Ĭ������
        resp->headers["Content-Type"] = "application/octect-stream";
        // ָ��Ĭ�����ص��ļ���
        resp->headers["content-disposition"] = "attachment;filename="+filename;
    });
    
   // ʹ��nginx������
    /*server.GET("/file/download", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        auto fileInfo = req->query_list();
        std::string filesha1 = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        resp->set_status_code("302");
        resp->headers["Location"] = "http://1.94.50.145:1235/" + filename;
    });*/
    // �������ݿ� ���ݿ���Ҫ��Щ���أ�
    // 1.ȫ���ļ���   �����е��ļ���Ϣ��¼����  �ļ��� �ļ���ϣֵ �ļ���С �ļ�λ��... tbl_file
    /*
    �ֿ�ֱ�����Ӹ���Ӧ�ó���
    �ֿ�ֱ���Ϊ��Ӧ���������������ս
    ���Ӹ�����Ϊ����߶����ܣ��ṩ�ݴ��ܣ���߿�����
    */
    // ���԰ѱ�ֱ���� ���õ���Ϣ����tb1 �����õķ���tb2
    /*
    ��ֱ��Ƭ���ŵ㣺
    1.��߲�ѯ���ܣ�ֻ��ѯ������ֶΣ����������ݵĶ�ȡ�ʹ�����������˲�ѯЧ�ʡ� 
    2.�����������ݣ�ÿ�����ݿ���ֻ������ص����ݣ������˴洢�����ֶεĿ����� 3.
    �������ݿ⸺�أ�����ͬҵ������Բ�ǿ�ı��ɢ����ͬ�����ݿ��У����Խ��͵�һ���ݿ�ĸ���ѹ����������ݿ�Ĳ�������������
    ��ֱ��Ƭ��ȱ�㣺
    1.����ʱ���ѯЧ�ʵ� ��Ҫjoin
    */
    /*
    ˮƽ��Ƭ��������ṹ��һ����  ����������tb1  ����������tb2
    ���Խ�����������������
    */
    // 2.�û���  tbl_user
    // 3.�û��ļ���  tbl_user_file
    // 4.token��   ÿһ�ε�¼�Ժ�  ���ͻ���һ��ƾ֤  ���ڷ����У������û������ʸ�  token��Чʱ�䣺1������-1���� ��÷���redis EXPIREָ��
    // ע�����
    server.GET("/user/signup", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("static/view/signup.html");
    });
    server.POST("/user/signup", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // 1.��urlencoded����ʽȥ����post������
        std::map<std::string, std::string> &form_kv = req->form_kv();
        std::string username = form_kv["username"];
        std::string password = form_kv["password"];
        // 2.��������м���
        std::string salt = "12345678";
        char *encryptPassword = crypt(password.c_str(), salt.c_str());
        fprintf(stderr, "encryptPassword = %s\n", encryptPassword);
        // 3.���û���Ϣ���뵽���ݿ�
        std::string sql = "INSERT INTO cloudisk.tbl_user (user_name, user_pwd) VALUES ('"+ username + "', '" + encryptPassword + "');";
        fprintf(stderr, "sql = %s\n", sql.c_str());
        // create_mysql_task
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [](WFMySQLTask *mysqlTask){
            // 4.�ظ�һ��SUCCESS��ǰ��
            wfrest::HttpResp *resp2client = static_cast<wfrest::HttpResp *>(mysqlTask->user_data);
            if(mysqlTask->get_state() != WFT_STATE_SUCCESS){
                fprintf(stderr, "error msg:%s\n", WFGlobal::get_error_string(mysqlTask->get_state(), mysqlTask->get_error()));
                
                resp2client->append_output_body("FAIL", 4);
                return;
            }
            // �鿴ָ���Ƿ�ִ�гɹ�  �õ���Ӧ
            protocol::MySQLResponse *resp = mysqlTask->get_resp();
            protocol::MySQLResultCursor cursor(resp);
            // ��ȡ��������
            // ����﷨����
            if(resp->get_packet_type() == MYSQL_PACKET_ERROR){
                fprintf(stderr, "error_code = %d msg = %s\n", resp->get_error_code(), resp->get_error_msg().c_str());
                resp2client->append_output_body("FAIL", 4);
                return;
            }

            if(cursor.get_cursor_status() == MYSQL_STATUS_OK){
                // дָ�ִ�гɹ�
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
        series->push_back(mysqlTask);
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
        // 1.�����û�����
        std::map<std::string, std::string> &form_kv = req->form_kv();
        std::string username = form_kv["username"];
        std::string password = form_kv["password"];
        // 2.��ѯ���ݿ�
        std::string url = "mysql://root:123456@localhost";
        std::string sql = "SELECT user_pwd FROM cloudisk.tbl_user WHERE user_name = '" + username + "' LIMIT 1;";
        auto readTask = WFTaskFactory::create_mysql_task(url, 0, [](WFMySQLTask *readTask){
            // ��ȡreadTask�Ľ��
            auto resp = readTask->get_resp();
            // ��Ϊ������ ��ȡ���
            protocol::MySQLResultCursor cursor(resp);

            std::vector<std::vector<protocol::MySQLCell>> rows;
            cursor.fetch_all(rows);

            // ���ݿ��е�����
            std::string nowPassword = rows[0][0].as_string();
            fprintf(stderr, "nowPassword = %s\n", nowPassword.c_str());

            UserInfo *userinfo = static_cast<UserInfo *>(series_of(readTask)->get_context());
            // �û����������
            char * inPassword = crypt(userinfo->password.c_str(), "12345678");
            fprintf(stderr, "inPassword = %s\n", inPassword);
            if(strcmp(nowPassword.c_str(), inPassword) != 0){
                userinfo->resp->append_output_body("FAIL", 4);
                return;
            }
            // 3.����һ��token���������ݿ���
            // �����û�����Ϣ �������ܵõ�����  ƴ���ϵ�½ʱ�� ����һ�����token
            // ��ֵ�̶� Ϊ12345678
            Token usertoken(userinfo->username, "12345678");
            fprintf(stderr, "token = %s\n", usertoken.token.c_str());
            userinfo->token = usertoken.token;
            // �������ݿ⵱��
            std::string url = "mysql://root:123456@localhost";
            // replace �൱�� insert �� update�Ľ��
            std::string sql = "REPLACE INTO cloudisk.tbl_user_token (user_name, user_token) VALUES ('" 
                + userinfo->username + "', '" 
                + usertoken.token + "');";
            auto writeTask = WFTaskFactory::create_mysql_task(url, 0, [readTask](WFMySQLTask *writeTask){
                // ����û�������  ����ɹ�
                // // 4.����Ϣ��װ��Json ���ظ��ͻ���
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
        // �����еĻص��������ͷ�
        series->set_callback([](const SeriesWork *series){
            // delete;
            UserInfo *userinfo = static_cast<UserInfo *> (series_of(series)->get_context());
            delete userinfo;
            fprintf(stderr, "userinfo is deleted\n");
        });
        
    });
    // ����ˢ�� ��ѯ�û���Ϣ
    server.POST("/user/info", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // ��ѯ�û���Ϣ��Ҫ�õ����ݿ� ������Ҫmysql����
        // 1 �����û�����
        auto userInfo = req->query_list();
        // 2 У��token�Ƿ�Ϸ� ---> ������
        // 3 �����û���Ϣ����ѯsql
        std::string sql = "SELECT user_name, signup_at FROM cloudisk.tbl_user WHERE user_name='" 
                          + userInfo["username"] + "' LIMIT 1;";
        auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [resp](WFMySQLTask *mysqlTask){
            auto respMysql = mysqlTask->get_resp();
            // ��Ϊ������ ��ȡ���
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
            // ���ظ��û�  ����ҳ����ܵ����û�����Ϣ
            resp->String(respInfo.dump());
        });
        mysqlTask->get_req()->set_query(sql);
        series->push_back(mysqlTask);
    });
    server.POST("/file/query", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
        // �����û�����
        auto userInfo = req->query_list();
        std::string username = userInfo["username"];
        auto form_kv = req->form_kv();
        std::string limit = form_kv["limit"];
        // �����û�����tbl_user_file
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