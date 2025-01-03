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
    // ��ʾ�ϴ�����
    server.GET("/file/upload", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->File("./static/view/index.html");
    });
    // �ϴ��ļ�
    server.POST("/file/upload",[](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // ��ȡ�ļ����� ����form-data���͵�������
        using Form = std::map<std::string, std::pair<std::string, std::string>>;
        Form &form = req->form();
        auto fileInfo = form["file"];
        // fileInfo.first = �ļ���  fileInfo.second = �ļ�����
        // tmpĿ¼��������ļ�  �Լ����������ļ�·��
        std::string filepath = "./tmp/" + fileInfo.first;
        // ͨ��openϵͳ���ý��д����ļ�
        // ����O_EXCL �ϴ��ظ��ļ��ᱨ��
        int fd = open(filepath.c_str(), O_RDWR | O_CREAT | O_EXCL, 0666);
        if(fd < 0){
            // ����
            resp->set_status_code("500");
        }
        // �����Բ����쳣����
        // try {} catch ()
        // ���������������� д��ȥ
        // ����1 ͨ��writeϵͳ���� ������ͬ���� Ч�ʽϵ�
        int ret = write(fd, fileInfo.second.c_str(), fileInfo.second.size());
        close(fd);
        // ����2 pwrite������ �첽���
        // auto pwriteTask = WFTaskFactory::create_pwrite_task(fd, fileInfo.second.c_str(), fileInfo.second.size(), 0, callback);
        // series->push_back(pwriteTask);
        // ���ϴ��ļ�֮����߼�д��callback����

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
        // ����ϴ��ɹ���  ����302���ض���
        resp->set_status_code("302");
        // ��location����Ӧ�ļ�ֵ��Ϊ /file/upload/success
        resp->headers["Location"] = "/file/upload/success";
    });
    server.GET("/file/upload/success", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        resp->String("Upload success");
    });
    // ʵ���ļ����ع���
    server.GET("/file/download", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        // /file/download?filehash=aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d&filename=1.txt&filesize=5
        // �����û��������
        auto fileInfo = req->query_list();
        std::string filesha1 = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        // ���ļ�
        std::string filepath = "tmp/" + filename;
        int fd = open(filepath.c_str(), O_RDONLY);
        // ����һƬ�ڴ� ���ļ����ݷŽ�ȥ �Ƶ���Ӧ���ĵı�������
        // �п������ݺܴ� �����ڶ�������
        // ��ȡ�ļ���С lseek �ƶ����ļ�ĩβ�ľ���
        int size = lseek(fd, 0, SEEK_END);
        // ����֮����ƫ�ƻ�ȥ
        lseek(fd, 0, SEEK_SET);
        std::unique_ptr<char []> buf(new char[size]);
        // ������ һ���Խ��ļ�����ȫ����ȡ����
        read(fd, buf.get(), size);
        // ������Ӧ�� ���ͻؿͻ���
        resp->append_output_body(buf.get(), size);
        // Ĭ������
        resp->headers["Content-Type"] = "application/octect-stream";
        // ָ��Ĭ�����ص��ļ���
        resp->headers["content-disposition"] = "attachment;filename="+filename;
    });
    
   // ʹ��nginx������
    /*server.GET("/file/download", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp){
        auto fileInfo = req->query_list();
        std::string filesha1 = fileInfo["filehash"];
        std::string filename = fileInfo["filename"];
        int filesize = std::stoi(fileInfo["filesize"]);
        resp->set_status_code("302");
        resp->headers["Location"] = "http://1.94.50.145:1235/" + filename;
    });*/
    // �������ݿ� ���ݿ���Ҫ��Щ���أ�
    // 1.ȫ���ļ���   �����е��ļ���Ϣ��¼����  �ļ��� �ļ���ϣֵ �ļ���С �ļ�λ��... tbl_file
    /*
    �ֿ�ֱ�����Ӹ���Ӧ�ó���
    �ֿ�ֱ���Ϊ��Ӧ���������������ս
    ���Ӹ�����Ϊ����߶����ܣ��ṩ�ݴ��ܣ���߿�����
    */
    // ���԰ѱ�ֱ���� ���õ���Ϣ����tb1 �����õķ���tb2
    /*
    ��ֱ��Ƭ���ŵ㣺
    1.��߲�ѯ���ܣ�ֻ��ѯ������ֶΣ����������ݵĶ�ȡ�ʹ�����������˲�ѯЧ�ʡ� 
    2.�����������ݣ�ÿ�����ݿ���ֻ������ص����ݣ������˴洢�����ֶεĿ����� 3.
    �������ݿ⸺�أ�����ͬҵ������Բ�ǿ�ı��ɢ����ͬ�����ݿ��У����Խ��͵�һ���ݿ�ĸ���ѹ����������ݿ�Ĳ�������������
    ��ֱ��Ƭ��ȱ�㣺
    1.����ʱ���ѯЧ�ʵ� ��Ҫjoin
    */
    /*
    ˮƽ��Ƭ��������ṹ��һ����  ����������tb1  ����������tb2
    ���Խ�����������������
    */
    // 2.�û���  tbl_user
    // 3.�û��ļ���  tbl_user_file
    // 4.token��   ÿһ�ε�¼�Ժ�  ���ͻ���һ��ƾ֤  ���ڷ����У������û������ʸ�  token��Чʱ�䣺1������-1���� ��÷���redis EXPIREָ��


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