#include <signal.h>

#include <iostream>

#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>

using std::cout;
using std::endl;

//初始化时，计数器的值为1
static WFFacilities::WaitGroup waitGroup(1);

void handler(int signum)
{
    printf("signum is %d\n", signum);
    //每调用一次done，计数器的值就会减1
    waitGroup.done();
    printf("waitGroup done\n");
}


void process(WFHttpTask * serverTask)
{
    printf("process is running\n");
}


void test0()
{
    signal(SIGINT, handler);

    WFHttpServer httpserver(process);

    if(!httpserver.start(8888)) {
        waitGroup.wait();
        httpserver.stop();
    } else {
        printf("server start failed\n");
    }
}


int main()
{
    test0();
    return 0;
}

