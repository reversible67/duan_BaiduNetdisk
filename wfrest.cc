#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
static WFFacilities::WaitGroup waitGroup(1);

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
    // .track() 打印出运行的状态
    if(server.track().start(1234) == 0){
        waitGroup.wait();
        server.stop();
    }
    else{
        fprintf(stderr, "can not start server!\n");
        return -1;
    }
}