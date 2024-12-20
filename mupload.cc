#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>
#include <workflow/HttpUtil.h>
#include <nlohmann/json.hpp>
using Json = nlohmann::json;
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}

void process(WFHttpTask *serverTask){
    // 解析url、分派任务
    auto req = serverTask->get_req();
    auto resp = serverTask->get_resp();
    std::string uri = req->get_request_uri();  // 路径 + 查询
    std::string path = uri.substr(0, uri.find("?"));
    std::string query = uri.substr(uri.find("?") + 1);
    std::string method = req->get_method();
    
    // 初始化
    if(method == "POST" && path == "/file/mupload/init"){
        // 1.读取请求报文，获取请求报文体
        const void *body;
        size_t size;
        req->get_parsed_body(&body, &size);
        // 2.将报文体解析成json对象
        Json fileInfo = Json::parse(static_cast<const char *>(body));
        std::string filename = fileInfo["filename"];
        std::string filehash = fileInfo["filehash"];
        int filesize = fileInfo["filesize"];
        printf("filename = %s\n filehash = %s\n filesize = %d\n", filename.c_str(), filehash.c_str(), filesize);
        // 3.初始化分块信息 uploadID分块
        // uploadID = user + name
        std::string uploadID = "duan";
        time_t now = time(nullptr);
        struct tm *ptm = localtime(&now);
        char timeStamp[20] = {0};
        snprintf(timeStamp, 20, "%02d:%02d", ptm->tm_hour, ptm->tm_min);
        uploadID += timeStamp;
        fprintf(stderr, "uploadID = %s\n", uploadID.c_str());
        // 生成分块信息
        int chunkcount;
        int chunksize = 2* 1024 * 1024;
        chunkcount = filesize / chunksize + (filesize % chunksize != 0);
        fprintf(stderr, "chunkcount = %d, chunsize = %d\n", chunkcount, chunksize);
        // 4.生成对客户端的响应
        Json uppartInfo;
        uppartInfo["status"] = "OK";
        uppartInfo["uploadID"] = uploadID;
        uppartInfo["chunkcount"] = chunkcount;
        uppartInfo["filesize"] = filesize;
        uppartInfo["chunksize"] = chunksize;
        uppartInfo["filehash"] = filehash;
        // 传回给客户端
        resp->append_output_body(uppartInfo.dump());
        // 5.将一些信息写入缓存
        std::vector<std::vector<std::string>> argsVec = {
            {"MP_" + uploadID, "chunkcount", std::to_string(chunkcount)},
            {"MP_" + uploadID, "filehash", filehash},
            {"MP_" + uploadID, "filesize", std::to_string(filesize)}
        };
        // 各个redisTask是并行执行的
        for(int i = 0; i < 3; ++i){
            auto redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 2, nullptr);
            redisTask->get_req()->set_request("HSET", argsVec[i]);
            redisTask->start();
        }
    }
    // 上传单个分块
    else if(method == "POST" && path == "/file/mupload/uppart"){
        
    }
    else if(method == "GET" && path == "/file/mupload/complete"){
        
    }
}

int main(){
    signal(SIGINT, sigHandler);
    WFHttpServer server(process);
    if(server.start(1234) == 0){
        waitGroup.wait();
        server.stop();
    }
    else{
        perror("server start failed\n");
        return -1;
    }
    return 0;
}