#include "signup.srpc.h"
#include "workflow/WFFacilities.h"
#include "linuxheader.h"

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

class UserServiceServiceImpl : public UserService::Service
{
public:

	void Signup(ReqSignup *request, RespSignup *response, srpc::RPCContext *ctx) override
	{
		// ��ȡ�û���������
		std::string username = request->username();
		std::string password = request->password();
		std::string salt = "12345678";
		// ����һ������
		char *encryptPassword = crypt(password.c_str(), salt.c_str());
		// sql���
		std::string sql = "INSERT INTO cloudisk.tbl_user (user_name, user_pwd) VALUES ('"+ username + "', '" + encryptPassword + "');";
		// mysql����
		auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [response](WFMySQLTask *mysqlTask){
			if(mysqlTask->get_state() == WFT_STATE_SUCCESS){
				response->set_message("SIGNUP SUCCESS");
				response->set_code(0);
			}
			else{
				response->set_message("SIGNUP FAIL");
				response->set_code(0);
			}
		});
		// ͨ��ctx�ҵ�rpcTask���ڵ����У���rpcTask���ڵ�����ִ�н����Ժ󣬻ظ���Ӧ���ͻ���
		ctx->get_series()->push_back(mysqlTask);
	}
};

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1234;
	SRPCServer server;

	UserServiceServiceImpl userservice_impl;
	server.add_service(&userservice_impl);

	server.start(port);
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
