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
	// ���Է���resp
	fprintf(stderr, "code = %d, message = %s\n", response->code(), response->message().c_str());
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	const char *ip = "127.0.0.1";
	unsigned short port = 1234;

	// ����һ���������
	UserService::SRPCClient client(ip, port);
	// ׼��rpc�Ĵ������
	ReqSignup signup_req;
	signup_req.set_username("admin");
	signup_req.set_password("1234");
	//signup_req.set_message("Hello, srpc!");
	// ͨ���������ķ���������rpc
	// ������˻ظ���Ӧ��ʱ�򣬵���signup_done����
	// client.Signup(&signup_req, signup_done);

	// ʹ���������ʽ��ʹ�ÿͻ���
	auto rpcTask = client.create_Signup_task(signup_done);
	// �޸���������� ������rpc�Ĳ���
	rpcTask->serialize_input(&signup_req);
	// ��������
	rpcTask->start();
	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
