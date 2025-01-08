#include <string>
#include <wfrest/HttpServer.h>
// serverTask 向 mysqlTask传递任务
struct UserInfo{
    std::string username;
    std::string password;
    std::string token;
    wfrest::HttpResp *resp;
};