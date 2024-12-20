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
    // ����url����������
    auto req = serverTask->get_req();
    auto resp = serverTask->get_resp();
    std::string uri = req->get_request_uri();  // ·�� + ��ѯ
    std::string path = uri.substr(0, uri.find("?"));
    std::string query = uri.substr(uri.find("?") + 1);
    std::string method = req->get_method();
    
    // ��ʼ��
    if(method == "POST" && path == "/file/mupload/init"){
        // 1.��ȡ�����ģ���ȡ��������
        const void *body;
        size_t size;
        req->get_parsed_body(&body, &size);
        // 2.�������������json����
        Json fileInfo = Json::parse(static_cast<const char *>(body));
        std::string filename = fileInfo["filename"];
        std::string filehash = fileInfo["filehash"];
        int filesize = fileInfo["filesize"];
        printf("filename = %s\n filehash = %s\n filesize = %d\n", filename.c_str(), filehash.c_str(), filesize);
        // 3.��ʼ���ֿ���Ϣ uploadID�ֿ�
        // uploadID = user + name
        std::string uploadID = "duan";
        time_t now = time(nullptr);
        struct tm *ptm = localtime(&now);
        char timeStamp[20] = {0};
        snprintf(timeStamp, 20, "%02d:%02d", ptm->tm_hour, ptm->tm_min);
        uploadID += timeStamp;
        fprintf(stderr, "uploadID = %s\n", uploadID.c_str());
        // ���ɷֿ���Ϣ
        int chunkcount;
        int chunksize = 2* 1024 * 1024;
        chunkcount = filesize / chunksize + (filesize % chunksize != 0);
        fprintf(stderr, "chunkcount = %d, chunsize = %d\n", chunkcount, chunksize);
        // 4.���ɶԿͻ��˵���Ӧ
        Json uppartInfo;
        uppartInfo["status"] = "OK";
        uppartInfo["uploadID"] = uploadID;
        uppartInfo["chunkcount"] = chunkcount;
        uppartInfo["filesize"] = filesize;
        uppartInfo["chunksize"] = chunksize;
        uppartInfo["filehash"] = filehash;
        // ���ظ��ͻ���
        resp->append_output_body(uppartInfo.dump());
        // 5.��һЩ��Ϣд�뻺��
        std::vector<std::vector<std::string>> argsVec = {
            {"MP_" + uploadID, "chunkcount", std::to_string(chunkcount)},
            {"MP_" + uploadID, "filehash", filehash},
            {"MP_" + uploadID, "filesize", std::to_string(filesize)}
        };
        // ����redisTask�ǲ���ִ�е�
        for(int i = 0; i < 3; ++i){
            auto redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 2, nullptr);
            redisTask->get_req()->set_request("HSET", argsVec[i]);
            redisTask->start();
        }
    }
    // �ϴ������ֿ�
    else if(method == "POST" && path == "/file/mupload/uppart"){
        // �ϴ������ֿ�
        // 1.�����û����� ��ȡ�� uploadID��chkidx
        // uploadID = MP_duan15:55&chkidx = 1
        std::string uploadIDKV = query.substr(0, query.find("&"));
        std::string chkidxKV = query.substr(query.find("&") + 1);
        std::string uploadID = uploadIDKV.substr(uploadIDKV.find("=") + 1);
        std::string chkidx = chkidxKV.substr(chkidx.find("=") + 1);
        // 2.��ȡ�ļ���hash������Ŀ¼��д��ֿ�
        // HGET uploadID filehash
        // ������Ҫ�õ�callback ��ΪҪ�õ�filehash
        auto redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 2,[](WFRedisTask *redisTask){
            protocol::RedisRequest *req = redisTask->get_req();
            protocol::RedisResponse *resp = redisTask->get_resp();
            int state = redisTask->get_state();
            int error = redisTask->get_error();
            protocol::RedisValue value;
            switch (state){
            case WFT_STATE_SYS_ERROR:
                fprintf(stderr, "system error : %s\n", strerror(error));
                break;
            case WFT_STATE_DNS_ERROR:
                fprintf(stderr, "dns error: %s\n", gai_strerror(error));
                break;
            case WFT_STATE_SUCCESS:
                resp->get_result(value);
                if(value.is_error()){
                    fprintf(stderr, "redis error\n");
                    state = WFT_STATE_TASK_ERROR;
                }
                break;
            }
            if(state != WFT_STATE_SUCCESS){
                fprintf(stderr, "Failed\n");
                return;
            }
            else{
                fprintf(stderr, "Success!\n");
            }
        });
        redisTask->get_req()->set_request("HGET", {uploadID, "filehash"});
        // 3.д��ֿ����֮�󣬽��ϴ��Ľ��ȴ��뻺����
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