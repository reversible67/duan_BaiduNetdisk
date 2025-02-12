#include <signal.h>

#include <iostream>

#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>

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
    //1. 解析请求
    auto req = serverTask->get_req();
    auto httpresp = serverTask->get_resp();
    string uri = req->get_request_uri();
    cout << "uri:" << uri << endl;
    // /login?name=liubei&password=123
    string nameKV = uri.substr(0, uri.find("&"));
    string passwdKV = uri.substr(uri.find("&") + 1);
    string name = nameKV.substr(nameKV.find("=") + 1);
    string passwd = passwdKV.substr(passwdKV.find("=") + 1);
    cout << "name:" << name << endl;
    cout << "passwd:" << passwd << endl;

    //2. 构造Redis任务
    string url = "redis://127.0.0.1:6379";
    auto redisTask = WFTaskFactory::create_redis_task(url, 1, 
        [passwd, httpresp](WFRedisTask * redistask){
            int state = redistask->get_state();
            int error = redistask->get_error();
            auto resp = redistask->get_resp();
            protocol::RedisValue result;
            switch(state) {
            case WFT_STATE_SYS_ERROR:
                printf("sys error: %s\n", strerror(error)); break;
            case WFT_STATE_DNS_ERROR:
                printf("dns error: %s\n", gai_strerror(error)); break;
            case WFT_STATE_SUCCESS:
                resp->get_result(result);break;
            }
            if(state != WFT_STATE_SUCCESS) {
                printf("error ocurrs\n");
                return;
            }
            //获取响应信息,进行比对
            if(result.is_string()) {
                cout << "response:" << result.string_value() << endl;
                if(passwd == result.string_value()) {
                    httpresp->append_output_body("Login Success");
                } else {
                    //登录失败
                    httpresp->append_output_body("Login Failed");
                }
            } else {
                //登录失败
                httpresp->append_output_body("Login Failed");
            } 
        });
    redisTask->get_req()->set_request("GET", {name});
    series_of(serverTask)->push_back(redisTask);
    
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

