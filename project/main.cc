#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/MySQLMessage.h>
#include <nlohmann/json.hpp>
static WFFacilities::WaitGroup waitGroup(1);
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
        resp->File("static/view/index.html");
    });
    // 上传文件
    server.POST("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // 读取文件内容 解析form-data类型的请求报文
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        Form &form = req->form();
        auto fileInfo = form["file"];
        // fileInfo.first = 文件名  fileInfo.second = 文件内容
        // tmp目录存放所有文件  以及用于生成文件路径
        std::string filepath = "tmp/" + fileInfo.first;
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