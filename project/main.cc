#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <wfrest/HttpServer.h>
#include <workflow/MySQLUtil.h>
#include <workflow/MySQLResult.h>
#include <workflow/MySQLMessage.h>
#include <nlohmann/json.hpp>
#include "FileUtil.h"
#include <wfrest/HttpMsg.h>
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
        resp->File("./static/view/index.html");
    });
    // 上传文件
    server.POST("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // 读取文件内容 解析form-data类型的请求报文
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        Form &form = req->form();
        auto fileInfo = form["file"];
        // fileInfo.first = 文件名  fileInfo.second = 文件内容
        // tmp目录存放所有文件  以及用于生成文件路径
        std::string filepath = "./tmp/" + fileInfo.first;
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


        // fprintf(stderr, "sha1 = %s\n", FileUtil::sha1File(filepath.c_str()).c_str());

        std::string sql = "INSERT INTO cloudisk.tbl_file (file_sha1,file_name, file_size, file_addr, status) VALUES('" 
                + FileUtil::sha1File(filepath.c_str()) + "',' "
                + fileInfo.first + "',"
                + std::to_string(fileInfo.second.size()) + ",'"
                + filepath + "', 0)";
        // fprintf(stderr, "sql = %s\n", sql.c_str());
        using Json = nlohmann::json;
        resp->MySQL("mysql://root:123456@localhost", sql, [](Json *pjson){
            fprintf(stderr, "out = %s\n", pjson->dump().c_str());
        });
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
    // 引入数据库 数据库需要哪些表呢？
    // 1.全局文件表   把所有的文件信息记录下来  文件名 文件哈希值 文件大小 文件位置... tbl_file
    /*
    分库分表和主从复制应用场景
    分库分表是为了应对数据量过大的挑战
    主从复制是为了提高读性能，提供容错功能，提高可用性
    */
    // 可以把表垂直分裂 常用的信息放在tb1 不常用的放在tb2
    /*
    垂直分片的优点：
    1.提高查询性能：只查询所需的字段，减少了数据的读取和传输量，提高了查询效率。 
    2.减少冗余数据：每个数据库或表只包含相关的数据，避免了存储冗余字段的开销。 3.
    降低数据库负载：将不同业务关联性不强的表分散到不同的数据库中，可以降低单一数据库的负载压力，提高数据库的并发处理能力。
    垂直分片的缺点：
    1.部分时候查询效率低 需要join
    */
    /*
    水平分片：两个表结构是一样的  若干行属于tb1  若干行属于tb2
    可以解决数据量过大的问题
    */
    // 2.用户表  tbl_user
    // 3.用户文件表  tbl_user_file
    // 4.token表   每一次登录以后  给客户端一个凭证  用于服务端校正你有没有这个资格  token有效时间：1个星期-1个月 最好放入redis EXPIRE指令


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