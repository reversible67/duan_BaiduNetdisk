#include <string>
#include <wfrest/HttpServer.h>
// serverTask �� mysqlTask��������
struct UserInfo{
    std::string username;
    std::string password;
    std::string token;
    wfrest::HttpResp *resp;
};