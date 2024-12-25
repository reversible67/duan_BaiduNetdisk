#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
static WFFacilities::WaitGroup waitGroup(1);
// wfrest框架
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