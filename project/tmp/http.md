### 工程能力的提升

- 会大量引入第三方框架和产品

- 不要求掌握产品的所有细节，把精力放在和业务需求相关的功能



### 学习上的要求：

- 不要求全  会用、懂原理就好
- 注重实践  充分利用时间 马上练习
- 内容逻辑性不强的时候  了解问题场景&解决方案
- 一定要记录笔记
- 广度优先



### HTTP协议

- 自己写一个简单的http服务端（了解协议的内容 如何运作）
- `nginx`
- 利用`nginx` 反向代理 缓存 负载均衡

### HTTP的库

- `workflow` 搜狗公司的一个开源库 异步回调
- 分块上传功能 `redis`实现缓存
- 网盘的一般功能

### 分布式架构的网盘

- 使用`ceph`或者`OSS`持久存储数据
- 使用消息队列`rabbitmq`实现异步转移
- 远程过程调用`rpc` 把本地函数的压栈弹栈 转为通过网络通信的函数调用
- 序列化和反序列化 `protobuf`
- 使用现有的`rpc`框架 `srpc`搜狗公司
- 服务注册中心 consul 分布式系统

### 分布式理论

- 底层所使用的共识协议 raft
- ACID vs BASE  强一致性 vs 最终一致性

### Lua课程

- `Lua`语法
- `Lua`和`openresty`









### HTTP协议的特点

超文本传输协议

1.网络协议        应用层协议 ISO/OSI 7   TCP/IP 4  基于可靠的传输层协议  一般是TCP

2.传输模型        C/S

(发送请求  回复一个响应)    (请求，响应) -> 事务

HTTP：即超文本传输协议，用来传输超文本

资源的情况

网页的组成

HTML          网页的骨架 + 内容(超链接)

CSS              层叠样式文件

JavaScript   动态行为

### HTTP协议的特点

1.应用层协议

2.C/S模型

3.可靠  （传输层保证）

4.文本协议

5.无状态协议      stateless      事务结束后不占用任何内存

#### 无状态的好处

1.一个事务对另一个事务没有影响

### 把状态转移到客户端

浏览器 本地可以存储数据 cookie

session

token



### 浏览器

`IE`   `Chrome`   `Edge` `firefox`

浏览器就是`http`客户端(发请求)







### 一个简单的http服务的

基于`tcp`     

单线程服务器(一次只能处理一个请求)





### 客户端选择

1.浏览器

2.`curl`

3.`postman`

4. `apifox`



### 方法

![image-20241112111949917](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241112111949917.png)

GET 读   POST 写

GET  获取  输入`url`

POST  提交表单

​	`1. x-www-form-urlencoded   key1 = value1 & key2 = value2  适合短数据`

​	`2.form-data   a.指定一个boundary  b.把请求的元信息和内容放入若干个boundary之间`





### 响应报文

![image-20241113103526935](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241113103526935.png)

### 状态码和原因字符串

1XX      中间状态

2XX      成功

3XX      重定向

4XX      客户端错误

5XX      服务端错误

### 接口设计的问题

![image-20241113110929037](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241113110929037.png)

### REST

REST（表示状态转换 `Representational state transfer`）

怎么样设计`restful`的接口

1.行为一般有  增 删 查 改  `CRUD`

2.使用`url`的`path`定位资源

3.将参数放在请求体里面  （用JSON/XML组织）

4.得到的响应体只有关键结果 是`json/xml 不是html文档` 



### 幂等性

加法  不幂等

并集   幂等                      ------------>   唯一id



HTTPS

http      明文传递   风险：信息会泄漏

​                    |

​	      密文传递

### 对称加密 和 非对称加密

![image-20241114180358694](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241114180358694.png)

![image-20241115101448869](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241115101448869.png)

### 利用https

握手阶段：把对称加密的密钥传送过去

传输阶段：对称加密

![image-20241115104655169](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241115104655169.png)

![image-20241115151859055](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241115151859055.png)





Apache是基于进程的模型

Apache成为了一个重量级的Web服务端 它支持简易用 特性丰富 但是如果需要用它来实现高并发 就需要占用大量的资源(甚至是不可接受的)  并且它的可扩展性很差 如果用户提高为原来的两倍 那么所需求的资源远远超过两倍



作为C10K问题的解决者 Nginx最重要的特征包括高性能、高并发和低资源占用。除此之外，Nginx还有其他功能，比如负载均衡、缓存、访问控制等等，Nginx也可作为复杂的Web应用的基础架构。

Nginx采用了事件驱动模型

![image-20241119102650070](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241119102650070.png)

### Nginx的优点

1.高并发 高性能 <---- 架构优秀

2.可扩展性  <------ 模块化

3.可靠性好  用9    4个9      99.99%  最好5个9~6个9

4.热部署（运行中可以更换配置）

### 安装和配置`nginx`

![image-20241119104537780](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241119104537780.png)

1.安装依赖的库

```bash
sudo yum install libz-dev # 处理压缩
sudo yum install libpcre3-dev # 处理正则表达式
sudo yum install libssl-dev # 处理SSL连接
```

### `sbin/nginx`

-v/-V   显示版本信息

-S 发信号

-C  指定配置文件

-t   检查配置文件的语法错误

### 配置文件的特点

配置文件 = 指令 + 指令块

指令名 参数；

指令块名 参数 {}

include 包含另一个文件

#单行注释

$引用变量

支持正则表达式

### `nginx`的日志系统

登录日志

错误日志

![image-20241120111015517](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241120111015517.png)





### 4个重要的模块

`http`模块

`server`模块   ----------> 虚拟服务端

`location`模块  ------------> 根据`url`匹配配置功能

`npstream`模块 ------------> 支持反向代理



### server模块 虚拟服务端

![image-20241120115437315](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241120115437315.png)

### 域名要解析为`ip`地址

1.查缓存

2.检查本地的host文件 `/etc/hosts`

![image-20241120120935166](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241120120935166.png)

3.查询DNS服务





抓包

```bash
tcpdump -i any port 80 -w 路径
```

调试网络的基本命令

`netstat        netstat -t     netstat -a`

`tcpdump    -w保存下来 拖到windows 用 wireshark看`



### `tcpdump配合wireshark`

![image-20241120152458327](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241120152458327.png)





### `location模块`

指定一个匹配规则 让不同的path的请求 对于到不同location

![image-20241121102141992](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241121102141992.png)



###  使用`nginx`搭建静态资源服务器

`file`配置块





### 启用压缩

![image-20241121154945607](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241121154945607.png)



### 引入第三方模块

![image-20241122102517682](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241122102517682.png)

1.下载模块源代码到本地

2.修改`nginx`的`makefile`

```bash
./configure --add-dynamic-module=home/liao/echo-nginx-module
```

3.make modules

4.引入模块

```bash
cp ngx_http_echo_module.so  /usr/local/nginx/modules/
```

### 使用第三方模块

![image-20241122110716349](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241122110716349.png)

![image-20241122110755480](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241122110755480.png)



### 反向代理

![image-20241122151552532](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241122151552532.png)

### 采用反向代理设计的原因

![image-20241122153628098](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241122153628098.png)

### 同时开启两个`nginx`服务端 

不能同时启动两个`nginx`的原因 `pid_file`路径相同

```bash
pid logs/5_business.pid;
```

### 缓存

![image-20241125112054352](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241125112054352.png)

### 反向代理的时候启用缓存

### 负载均衡

![image-20241125161926168](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241125161926168.png)

### 加权轮询

![image-20241125164152725](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241125164152725.png)

### 直接哈希没有办法处理结点数量变化

![image-20241125171350304](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241125171350304.png)

引起大面积的缓存失效  ---------> 雪崩

### 一致性哈希

![image-20241125172823582](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241125172823582.png)

### Nginx的架构设计

![image-20241126111117368](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241126111117368.png)

### `accept`惊群

### `epoll`惊群

### 引入框架 库

常见的框架和库

TCP  reactor  -----> `muduo` (事件驱动)  

`proactor`  -------> `asio`  `boost::asio`



Http    `libhv` (reactor)

异步框架  `workflow`

协程框架  `brpc`

### 安装和配置workflow

1.获取源码

2.安装依赖  `cmake`

3.生成`makefile`

```bash
make -> make install -> ldconfig
在链接时加上 -lworkflow
```

### workflow

C++服务端框架

可以作为：

1.客户端  http

2.redis客户端

3.`Mysql`客户端

4.http的服务端  很容易实现功能

5.基于workflow 可以构建分布式应用   `rpc` -> `srpc`

### workflow 是异步框架

异步框架

“任务” 一段将要异步执行的代码

![image-20241126164028596](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241126164028596.png)

### 任务的“串并联”

workflow 提供了将任务串联和并联     串联--->先后    并联--->并发

![image-20241126164925276](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241126164925276.png)

### 使用异步任务

http任务

1.创建任务     任务本身就是一个对象     工厂模式   写好回调函数

2.设置任务的属性

3.启动任务   异步执行

### `redis`任务

`redis`客户端

SET 43KEY 100

### 序列任务

创建序列：task->start

```C++
#include "linuxheader.h"
#include <workflow/WFFacilities.h>
static WFFacilities::WaitGroup waitGroup(1);
void callback(WFRedisTask *redisTask){
    protocol::RedisRequest *req = redisTask->get_req();
    protocol::RedisResponse *resp = redisTask->get_resp();
    int state = redisTask->get_state();
    int error = redisTask->get_error();
    protocol::RedisValue value;  // value对象专门用来存储redis任务的结果
    switch (state)
    {
    case WFT_STATE_SYS_ERROR:
        fprintf(stderr, "system error: %s\n", strerror(error));
        break;
    case WFT_STATE_DNS_ERROR:
        fprintf(stderr, "dns error: %s\n", gai_strerror(error));
        break;
    case WFT_STATE_SUCCESS:
        resp->get_result(value);
        if(value.is_error()){
            fprintf(stderr, "redis error\n");
            state = WFT_STATE_TASK_ERROR;
        }
        break;
    }

    if(state != WFT_STATE_SUCCESS){
        fprintf(stderr, "Failed\n");
        return;
    }
    else{
        fprintf(stderr, "Success\n");
    }

    std::string cmd;
    req->get_command(cmd);
    if(cmd == "SET"){
        // firstTask的基本工作做完了
        // 创建新任务 把新任务加入到本序列的末尾
        fprintf(stderr, "first task callback begins\n");
        WFRedisTask *secondTask = WFTaskFactory::create_redis_task("redis://127.0.0.0:6379", 0, callback);
        protocol::RedisRequest *req = secondTask->get_req();
        req->set_request("GET", {"43key"});
        SeriesWork *series = series_of(redisTask);
        // 前面任务的回调执行完了 开始执行它
        series->push_back(secondTask);
        fprintf(stderr, "first task callback ends\n");
    }
    else{
        // secondTask的基本工作做完了
        fprintf(stderr, "second task callback begins\n");
        fprintf(stderr, "redis request, cmd = %s\n", cmd.c_str());
        if(value.is_string()){
            fprintf(stderr, "value is a string, value = %s\n", value.string_value().c_str());
        }
        else if(value.is_array()){
            fprintf(stderr, "value is string array\n");
            for(size_t i = 0; i < value.arr_size(); ++i){
                fprintf(stderr, "value at %lu = %s\n", i, value.arr_at(i).string_value().c_str());
            }
        }
        fprintf(stderr, "second task callback ends\n");
    }
    // fprintf(stderr, "redis request, cmd = %s\n", cmd.c_str());
    // if(value.is_string()){
    //     fprintf(stderr, "value is a string, value = %s\n", value.string_value().c_str());
    // }
    // else if(value.is_array()){
    //     fprint(stderr, "value is string array\n");
    //     for(size_t i = 0; i < value.arr_size(); ++i){
    //         fprintf(stderr, "value at %lu = %s\n", i, value.arr_at(i).string_value().c_str());
    //     }
    // }
}
void sigHandler(int num){
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}

int main(){
    signal(SIGINT, sigHandler);
    // 创建redis任务
    WFRedisTask *firstTask = WFTaskFactory::create_redis_task("redis://127.0.0.0:6379", 0, callback);
    // 设置redis任务的属性
    protocol::RedisRequest *req = firstTask->get_req();
    req->set_request("SET", {"43key", "100"});
    // 启动redis任务
    firstTask->start();
    waitGroup.wait();
}
```

### 在序列中使用context来共享数据

### 在序列的回调函数中delete

![image-20241129114227990](C:\Users\wd\AppData\Roaming\Typora\typora-user-images\image-20241129114227990.png)

### workflow

异步框架：异步执行基本工作（框架） -------> 回调函数



### 现实中的业务场景

买炸鸡

​                       -----------------------------> 吃

买啤酒

### 并行任务

由若干个并行的序列组成的任务

```C++
#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/Workflow.h>
#include <vector>
#include <workflow/HttpUtil.h>
static WFFacilities::WaitGroup waitGroup(1);
struct SeriesContext{
    std::string url;
    int state;
    int error;
    protocol::HttpResponse resp;  // 响应报文的完整内容
};
void sigHandler(int num){
    // 调用 done() 会使 WaitGroup 的计数减一。在这里，它的作用是让主线程知道信号处理函数已经完成。
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
void parallelCallback(const ParallelWork *pwork){
    fprintf(stderr, "pwork callback!\n");
    SeriesContext *context;
    for(size_t i = 0; i < pwork->size(); ++i){
        context = static_cast<SeriesContext *>(pwork->series_at(i)->get_context());
        fprintf(stderr, "url = %s\n", context->url.c_str());
        if(context->state == WFT_STATE_SUCCESS){
            const void *body;
            size_t size;
            context->resp.get_parsed_body(&body, &size);
            fwrite(body, 1, size, stderr);
            fprintf(stderr, "\n");
        }
        else{
            fprintf(stderr, "Error, state = %d, error = %d\n", context->state, context->error);
        }
        delete context;
    }
}
void httpCallback(WFHttpTask *httpTask){
    // auto req = httpTask->get_req();
    // std::string name;
    // std::string value;
    // protocol::HttpHeaderCursor reqCursor(req);
    // while(reqCursor.next(name, value)){
    //     fprintf(stderr, "%s:%s\r\n", name.c_str(), value.c_str());
    // }
    SeriesContext *context = static_cast<SeriesContext *>(series_of(httpTask)->get_context());
    fprintf(stderr, "httpTask callback, url = %s\n", context->url.c_str());
    context->state = httpTask->get_state();
    context->error = httpTask->get_error();
    context->resp = std::move(*httpTask->get_resp());   // 左值是拷贝行为   用move强转为右值
}
int main(){
    // 将 SIGINT 信号（通常由用户按下 Ctrl+C 触发）与 sigHandler 信号处理函数绑定。当接收到 SIGINT 信号时，程序会调用 sigHandler。
    signal(SIGINT, sigHandler);
    // 使用工厂函数，创建一个并行任务
    ParallelWork *pwork = Workflow::create_parallel_work(parallelCallback);   // Workflow::create_parellel_work

    std::vector<std::string> urlVec = {"http://www.jd.com", "http://1.94.50.145", "http://1.94.50.145/duan"};
    for(size_t i = 0; i != urlVec.size(); ++i){
        // 创建若干个任务
        // WFTaskFactory::create_http_task
        std::string url = urlVec[i];
        auto httpTask = WFTaskFactory::create_http_task(url, 0, 0, httpCallback);
        // 修改任务属性
        auto req = httpTask->get_req();
        req->add_header_pair("Accept", "*/*");
        req->add_header_pair("User-Agent", "myHttpTask");
        req->set_header_pair("Connection", "Close");
        // 为响应的内容申请一片堆空间
        SeriesContext *context = new SeriesContext;
        context->url = std::move(url);
        // 为每个任务创建一个序列
        // create_series_work
        auto series = Workflow::create_series_work(httpTask, nullptr);
        // 把存储响应内容的指针 拷贝到序列的context当中
        series->set_context(context);
        // 把序列加入到并行任务中
        // add_series
        pwork->add_series(series);
    }
    // 启动并行任务 这三个任务会异步的去执行
    // 也可以在这里的回调中释放内存
    Workflow::start_series_work(pwork, nullptr);
    // 主线程调用 waitGroup.wait() 来等待 WaitGroup 的计数变为零。
    waitGroup.wait();
}
/*
workflow的设计特点
1.任何任务放入序列中才能执行（需要包装为序列）
2.序列之间是并行的
3.若某事件需要等待序列都完成    需要并行执行
4.并行任务也是任务 也可以放在序列中
*/
```

### 用workflow实现http服务端

服务端任务（服务端接收到客户端的请求）（服务端框架生成的任务）

### process函数对象  (用户设计)

作用：去找到`serverTask`

每当客户端接入 框架解析完请求时，process就调用

`void（WFHttpTask *httpTask）`  `httpTask`是服务端任务

```C++
#include "linuxheader.h"
#include <workflow/WFFacilities.h>
#include <workflow/WFHttpServer.h>
static WFFacilities::WaitGroup waitGroup(1);
void sigHandler(int num){
    // 调用 done() 会使 WaitGroup 的计数减一。在这里，它的作用是让主线程知道信号处理函数已经完成。
    waitGroup.done();
    fprintf(stderr, "wait group is done\n");
}
void process(WFHttpTask *serverTask){
    auto resp = serverTask->get_resp();
    resp->set_http_version("HTTP/1.1");
    resp->set_status_code("200");
    resp->set_reason_phrase("OK");
    resp->set_header_pair("Content_Type", "text/plain");
    // resp->append_output_body("hello");
    resp->append_output_body("就像是那灰色天空中的小雨,\n下下停停不动声色淋湿土地,\n尽管总是阴晴不定,\n但偶尔也会闪出星星,\n这都是形容你的眼睛~");
}
int main(){
    signal(SIGINT, sigHandler);
    WFHttpServer server(process);
    // 监听服务端
    // == 0 代表启动成功
    if(server.start(1234) == 0){
        waitGroup.wait();
        server.stop();
    } 
    else{
        perror("server start failed\n");
        return -1;
    }
    return 0;
}
```

### 登录业务

client ----->test 123-------> server
