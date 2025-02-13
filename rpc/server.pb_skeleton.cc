#include "signup.srpc.h"
#include "workflow/WFFacilities.h"
#include "linuxheader.h"
#include <ppconsul/agent.h>

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
		// 提取用户名和密码
		std::string username = request->username();
		std::string password = request->password();
		std::string salt = "12345678";
		// 创建一个密文
		char *encryptPassword = crypt(password.c_str(), salt.c_str());
		// sql语句
		std::string sql = "INSERT INTO cloudisk.tbl_user (user_name, user_pwd) VALUES ('"+ username + "', '" + encryptPassword + "');";
		// mysql任务
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
		// 通过ctx找到rpcTask所在的序列，当rpcTask所在的序列执行结束以后，回复响应给客户端
		ctx->get_series()->push_back(mysqlTask);
	}
};

void timerCallback(WFTimerTask *timerTask){
	fprintf(stderr, "time = %ld\n", time(NULL));
	ppconsul::agent::Agent *pagent = static_cast<ppconsul::agent::Agent *>(timerTask->user_data);
	pagent->servicePass("SignupService1");
	auto nextTask = WFTaskFactory::create_timer_task(5000000, timerCallback);
	nextTask->user_data = pagent;
	series_of(timerTask)->push_back(nextTask);
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1234;
	SRPCServer server;

	UserServiceServiceImpl userservice_impl;
	server.add_service(&userservice_impl);

	server.start(port);

	// 将本服务注册到consul之中
	// 找到dc1注册中心  consul是客户端对象
	ppconsul::Consul consul("127.0.0.1:8500", ppconsul::kw::dc = "dc1");
	// 创建一个用来访问注册中心的agent
	ppconsul::agent::Agent agent(consul);
	// 开启一个服务
	agent.registerService(
		ppconsul::agent::kw::name = "SignupService1",
		ppconsul::agent::kw::address = "127.0.0.1",
		ppconsul::agent::kw::id = "SignupService1",
		ppconsul::agent::kw::port = 1234,
		ppconsul::agent::kw::check = ppconsul::agent::TtlCheck{std::chrono::seconds(10)}
	);
	// 把自己活着的消息告诉consul
	// 只要活着 就要永久的执行下去
	// agent.servicePass("SignupService1");
	// 创建一个定时任务 每隔一段时间pass一次
	auto timerTask = WFTaskFactory::create_timer_task(5000000, timerCallback);
	timerTask->user_data = &agent;
	timerTask->start();
	// 取消服务
	// agent.deregisterService("SignupService1");
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
