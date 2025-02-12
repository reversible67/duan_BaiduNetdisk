#include <signal.h>

#include <iostream>

#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>
#include <workflow/HttpUtil.h>

using std::cout;
using std::endl;
using std::string;

//初始化时，计数器的值为1
static WFFacilities::WaitGroup waitGroup(1);

void handler(int signum)
{
    printf("signum is %d\n", signum);
    //每调用一次done，计数器的值就会减1
    waitGroup.done();
    printf("waitGroup done\n");
}


void process(WFHttpTask * serverTask)
{
    printf("process is running\n");
    //1.错误的检测
    int state = serverTask->get_state();
    //int error = serverTask->get_error();
    printf("state: %d\n", state);
    if(state != WFT_STATE_TOREPLY) {
        return;
    }
    
    //2. 获取请求信息
    auto req = serverTask->get_req();
    //2.1 起始行
    printf("%s %s %s\n", req->get_method(),
           req->get_request_uri(),
           req->get_http_version());
    //2.2首部字段
    protocol::HttpHeaderCursor cursor(req);
    string key, val;
    while(cursor.next(key, val)) {
        cout << key << ": " << val << endl;
    }
    cout << endl;
    
    //3. 生成响应信息
    auto resp = serverTask->get_resp();
    //3.1 起始行
    resp->set_http_version("HTTP/1.1");
    resp->set_status_code("200");
    resp->set_reason_phrase("OK");
    //3.2 首部字段
    resp->add_header_pair("Server", "Workflow HttpServer");
    resp->add_header_pair("Contet-Type", "text/html");

    string body = "<html><body>hello,client</body></html>";

    resp->add_header_pair("Content-Length", std::to_string(body.size()));
    //3.3 添加消息体
    resp->append_output_body(body.c_str(), body.size());

}


void test0()
{
    signal(SIGINT, handler);

    WFHttpServer httpserver(process);

    if(!httpserver.start(8888)) {
        waitGroup.wait();
        httpserver.stop();
    } else {
        printf("server start failed\n");
    }
}


int main()
{
    test0();
    return 0;
}

