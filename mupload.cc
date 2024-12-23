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
        uppartInfo["filename"] = filename;
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
        std::string chkidx = chkidxKV.substr(chkidxKV.find("=") + 1);
        // 2.��ȡ�ļ���hash������Ŀ¼��д��ֿ�
        // HGET uploadID filehash
        // ������Ҫ�õ�callback ��ΪҪ�õ�filehash
        auto redisTaskHGET = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 2,[chkidx, req](WFRedisTask *redisTask){
            protocol::RedisRequest *redisReq = redisTask->get_req();
            protocol::RedisResponse *redisResp = redisTask->get_resp();
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
                redisResp->get_result(value);
                if(value.is_error()){
                    fprintf(stderr, "redis error\n");
                    state = WFT_STATE_TASK_ERROR;
                }
                break;
            }
            if(state != WFT_STATE_SUCCESS){
                // fprintf(stderr, "Failed\n");
                return;
            }
            else{
                // fprintf(stderr, "Success!\n");
            }
            // �õ��ļ���hashֵ
            std::string filehash = value.string_value();
            mkdir(filehash.c_str(), 0777);
            std::string filepath = filehash + "/" + chkidx;
            // ��������������ļ�
            int fd = open(filepath.c_str(), O_RDWR | O_CREAT, 0666);
            // ���ļ����ݽ���д��
            const void *body;
            size_t size;
            req->get_parsed_body(&body, &size);
            fprintf(stderr, "body = %s\nsize = %ld\n", (char *)body, size);
            write(fd, body, size);
            close(fd);
        });
        redisTaskHGET->get_req()->set_request("HGET", {uploadID, "filehash"});
        // ���ﲻ��ʹ��start  ��Ȼ�ڻ�ȡbody��ʱ�� �п���redisTaskû�н��� ����serverTask�Ѿ�������
        // redisTask��serverTask�ǲ��е�
        // redisTask->start();
        series_of(serverTask)->push_back(redisTaskHGET);
        // 3.д��ֿ����֮�󣬽��ϴ��Ľ��ȴ��뻺����
        auto redisTaskHSET = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 2, nullptr);
        redisTaskHSET->get_req()->set_request("HSET", {uploadID, "chkidx_"+chkidx, "1"});
        series_of(serverTask)->push_back(redisTaskHSET);
        // 4.�ظ���Ӧ
        resp->append_output_body("OK");
    }
    // �ϲ��ֿ�
    else if(method == "GET" && path == "/file/mupload/complete"){
        // 1 �����û����� ��ȡ��uploadID
        std::string uploadID = query.substr(query.find("=") + 1);
        // 2 ����uploadID��ѯ����  HGETALL uploadID
        auto redisTask = WFTaskFactory::create_redis_task("redis://127.0.0.1:6379", 2, [resp](WFRedisTask *redisTask){
            protocol::RedisRequest *redisReq = redisTask->get_req();
            protocol::RedisResponse *redisResp = redisTask->get_resp();
            protocol::RedisValue value;
            redisResp->get_result(value);
            // 3 �ҵ�chunkcount��Ӧ��ֵ
            int chunkcount;
            int chunknow = 0;
            // һ����ż��  ��Ϊ��K-V K-V K-V
            for(int i = 0; i < value.arr_size(); i+=2){
                std::string key = value.arr_at(i).string_value();
                std::string val = value.arr_at(i + 1).string_value();
                if(key == "chunkcount"){
                    // atoi(val.c_str());
                    chunkcount = std::stoi(val);
                }
                // 4 �ҵ�chkidx_*��Ӧ������
                // ǰ׺ƥ��
                else if(key.substr(0, 7) == "chkidx_"){
                    // ��ǰ���ϴ�������++
                    chunknow++;
                }
            }
            fprintf(stderr,"chunkcount = %d\nchunknow = %d\n", chunkcount, chunknow);
            // �Ƚϴ�С
            if(chunkcount == chunknow){
                resp->append_output_body("SUCCESS");
            }
            else{
                resp->append_output_body("FAIL");
            }
        });
        redisTask->get_req()->set_request("HGETALL", {uploadID});
        series_of(serverTask)->push_back(redisTask);
        // 5 �Ȳ�д����ϲ�
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