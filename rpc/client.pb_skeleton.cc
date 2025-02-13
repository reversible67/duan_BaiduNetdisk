#include "signup.srpc.h"
#include "workflow/WFFacilities.h"

using namespace srpc;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
	wait_group.done();
}

static void signup_done(RespSignup *response, srpc::RPCContext *context)
{
	// 可以访问resp
	fprintf(stderr, "code = %d, message = %s\n", response->code(), response->message().c_str());
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1234;

	// 创建一个代理对象
	UserService::SRPCClient client(ip, port);
	// 准备rpc的传入参数
	ReqSignup signup_req;
	signup_req.set_username("admin");
	signup_req.set_password("1234");
	//signup_req.set_message("Hello, srpc!");
	// 通过代理对象的方法，调用rpc
	// 当服务端回复响应的时候，调用signup_done函数
	// client.Signup(&signup_req, signup_done);

	// 使用任务的形式来使用客户端
	auto rpcTask = client.create_Signup_task(signup_done);
	// 修改任务的属性 以设置rpc的参数
	rpcTask->serialize_input(&signup_req);
	// 启动任务
	rpcTask->start();
	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
