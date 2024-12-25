#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
static WFFacilities::WaitGroup waitGroup(1);
// wfrest���
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
    // ʵ���ض���
    server.GET("/redirect", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // ��״̬������Ϊ302
        resp->set_status_code("302");
        // ��һ��location�ֶ�
        // resp->add_header_pair("Location", "/test");
        resp->headers["Location"] = "/test";  // �������Ч��һ��
    });
    // ��������
    server.GET("/test1", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        std::map<std::string, std::string> queryMap = req->query_list();
        for(auto it : queryMap){
            fprintf(stderr, "first = %s, second = %s\n", it.first.c_str(), it.second.c_str());
        }
    });
    // ��¼���� & ��ȡ����
    server.POST("/login", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        const std::string &username = req->query("username");
        const std::string &password = req->query("password");
        fprintf(stderr, "username = %s, password = %s\n", username.c_str(), password.c_str());
    });
    // ��body����ʽ ������Ϣ�������
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
    // formdataһ���������ݴ���Ϣ urlencodedһ�㴫��С��Ϣ
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
    // html   GET���� ������ҳ
    server.GET("/post1.html", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        int fd = open("post1.html", O_RDONLY);
        std::unique_ptr<char []> buf(new char[3922]);
        read(fd, buf.get(), 3922);
        resp->append_output_body(buf.get(), 3922);
        // �����Ϳ�����ʾ����ҳ��
        resp->headers["Content-Type"] = "text/html";
    });
    // ͨ��post�����ϴ��ļ� 
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
    // .track() ��ӡ�����е�״̬
    if(server.track().start(1234) == 0){
        // ��ǰ�����֧����Щ���� �� ��Щ·��
        server.list_routes();
        waitGroup.wait();
        server.stop();
    }
    else{
        fprintf(stderr, "can not start server!\n");
        return -1;
    }
}