### workflow框架

`workflow`  异步 + 回调

客户端任务：

1.创建任务（http、redis、IO）

2.设置任务属性

3.组织任务的执行（start/series->push_back）

服务端设计：

1.创建 `WFHTTPServer` ,start

2.每当有`client`连接时，框架就会创建任务（`serverTask`）

3.`serverTask` = 基本工作（调用process-->序列中其他任务-->回调-->序列回调） + 回调

### 实现一个登录过程

1.打开网页         `GET`

2.操作网页元素  `POST`

### 分块上传功能

##### 分块上传功能的实现方案

在上传文件的时候，假如文件的体积过大，就会导致单次传输时间过长，传输失败的风险大大增加。为了使传输过程更加灵活，也为了实现断点续传功能，一种解决方案就是分片--将要上传的文件切分成若干小片，每个小片单独传输，服务端会接收某个文件的若干小片，最后会合并这些分片以获取原始文件。

1.初始化上传：为不同的文件上传任务分配不同的ID

2.单个分片：存储某个文件分片

3.分片信息查询：记录分片的信息，有哪些是已经上传完成了的

4.分片合并：如果所有分片都已经上传完成，则合并分片得到原始文件

### 接口1：POST `/file/mupload/init`

##### 上传ID生成

在执行真正的上传操作之前，必须为每个文件上传任务分配一个上传ID，使用ID的原因如下：

* 服务端可能会并发地执行多个上传文件地任务，服务端可以通过ID识别不同的文件任务
* 如果由于各种原因导致本次上传失败，需要重启上传任务，使用ID可以获取缓存的上传进度信息 

在本项目中，上传ID的格式是"用户名+上传时间"

##### 分块信息处理

除了要生成上传ID以外，服务端还需要根据文件的体积和其他信息，生成分块相关的数据返回给客户端，并且存入缓存当中

客户端使用json包装文件信息：

可以用md5 或者 sha1去计算hash值

```json
{
    "filename":"apache-brpc-1.0.0-incubating-src.tar.gz",
    "filehash":"910638d6cbf29bcebac0bb00017e94cb4b13d43d",
    "filesize":18028779
}
```

服务端需要将分块的总数量、上传ID和其他文件相关信息返回给客户端：

```json
{
    "status":"OK",
    "uploadID":"uploadID",
    "chunkcount":"chunkcount",
}
```

### 具体步骤

##### init操作

1.客户端给 /file/mupload/init发请求

2.服务端

* 存入缓存  HSET  uploadID  MP_duan:15:56       chunkCount   4   filename  1.txt  chunkSize  12345
* 响应   uploadID，chunkCount

##### uppart操作

1.访问 /file/mupload/uppart ？ uploadID = xxx & chkidx = 02

报文体中放文件内容

2.服务端解析upload 和 chkidx  存储在tmp/1.txt/01   HGET uploadID filename

3.保存进度到redis     HSET uploadID chkidx_00  1

##### complete操作

1.解析 uploadID

2.判断上传是否完成  HGETALL uploadID     --------->  统计前缀为chkidx-的前缀 找到chunkCount对应的值

3.服务端合并分块（创建一个空的filesize大小的文件 先pread(0)再pwrite(offset)通过id大小 偏移量  并行的pread/pwrite进行完成 文件合并成功）

### MySQL任务

创建任务 ---> 设置任务属性 ---> 启动任务 ---> 执行回调

检查：

* MySQL连接问题
* SQL语句语法问题

MySQL分为读任务和写任务

读任务：show / select

写任务：MYSQL_STATUS_OK(写)   MYSQL_STATUS_GET_RESULT(读)

### 定时任务

### 包装workflow ---> wfrest

把workflow的服务端使用方法封装一下（gin）

#### 安装wfrest

```bash
git clone https://github.com/wfrest/wfrest.git
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
```

#### wfrest的方便之处

* 完全支持workflow
* 支持大量的解析工作（query的解析 post的url encode解析  post的form-data解析）
* 直接把方法和路径  ---> handler    不用再写大量的if else

### 网盘接口

文件模块   /file/upload GET              POST

   				      显示网页     上传文件

​		  /file/download  GET   下载文件

用户模块  /usr/signup  GET      POST

​    						 注册

​                 /usr/signin                 登录

​                 /usr/info                     获取登录用户的信息

​		 /file/query                  查询某个用户的文件

#### 第一版网盘结构

client ---------------------------------------> 主服务端（用户注册、登录   文件上传）

​           <--------------------------------------          |

​								      |

​							       静态资源（下载）



### 下载功能

```C++
server.POST("/file/downloadurl", [](const wfrest::HttpReq *req, wfrest::HttpResp *resp, SeriesWork *series){
    auto fileInfo = req->query_list();
    std::string sha1 = fileInfo["filehash"];
    std::string sql = "SELECT file_name FROM cloudisk.tbl_file WHERE file_sha1 ='" + sha1 + "';";
    auto mysqlTask = WFTaskFactory::create_mysql_task("mysql://root:123456@localhost", 0, [resp](WFMySQLTask *mysqlTask){
        auto respMysql = mysqlTask->get_resp();
        protocol::MySQLResultCursor cursor(respMysql);
        std::vector<std::vector<protocol::MySQLCell>> rows;
        cursor.fetch_all(rows);
        resp->String("http://1.94.50.145:1235/" + rows[0][0].as_string());
    });
    mysqlTask->get_req()->set_query(sql);
    series->push_back(mysqlTask);
});
```

如果哪一天服务器挂了，要花费很昂贵的代价去修复，所以说目前的方案是不保险的

把一个系统从单体的改成分布式的，处理容错和备份，要考虑的事情是很多的！

比如，通信故障。

### 使用workflow构建云盘系统

### 使用公有云存储实现备份

IaaS（infrastructure as a Service），就是云服务厂商把文件系统，数据库，缓存系统，消息队列等等组件剥离出来做成一个单独的产品，用户通过网络调用接口来使用服务，而不需要自行部署文件系统和数据库等组件。这样，数据备份的需求就可以交给云计算厂商的文件系统产品，通常会采用两种方法来使用：一种是使用专业的云厂商的提供的云产品，比如阿里云对象存储OSS；另一种是自己购买硬件，在此基础上搭建私有云产品，比如自行搭建分布式文件系统ceph

### 对象存储OSS

>阿里云对象存储OSS（Object Storage Service）是一款海量、安全、低成本、高可靠的云存储服务，可提供99.9999999999%（12个9）的数据持久性，99.995%的数据可用性。多种存储类型供选择，全面优化存储成本。
>
>OSS具有与平台无关的RESTful API接口，您可以在任何应用、任何时间、任何地点存储和访问任意类型的数据。
>
>您可以使用阿里云提供的API、SDK包或者OSS迁移工具轻松地将海量数据移入或移出阿里云OSS。数据存储到阿里云OSS以后，您可以选择标准存储（Standard）作为移动应用、大型网站、图片分享或热点音视频的主要存储方式，也可以选择成本更低、存储期限更长的低频访问存储（Infrequent Access）、归档存储（Archive）、冷归档存储（Cold Archive）或者深度冷归档（Deep Cold Archive）作为不经常访问数据的存储方式。
>
>OSS作为云上数据湖可提供高带宽的下载能力。在部分地域，可为单个阿里云账号提供高达100 Gbps的内外网总下载带宽，旨在满足AI和大规模数据分析的需求。

### OSS的基本概念

>想要顺利使用OSS，必须先知道以下几个组件的基本概念：
>
>- 存储空间
>
>  存储空间是用户用于存储对象（Object）的容器，所有的对象都必须隶属于某个存储空间。存储空间具有各种配置属性，包括地域、访问权限、存储类型等。用户可以根据实际需求，创建不同类型的存储空间来存储不同的数据。
>
>- 对象
>
>  对象是OSS存储数据的基本单元，也被称为OSS的文件。和传统的文件系统不同，对象没有文件目录层级结构的关系。对象由元数据（Object Meta）、用户数据（Data）和文件名（Key）组成，并且由存储空间内部唯一的Key来标识。对象元数据是一组键值对，表示了对象的一些属性，例如文件类型、编码方式等信息，同时用户也可以在元数据中存储一些自定义的信息。
>
>- 对象名称
>
>  在各语言SDK中，ObjectKey、Key以及ObjectName是同一概念，均表示对Object执行相关操作时需要填写的Object名称。例如向某一存储空间上传Object时，ObjectKey表示上传的Object所在存储空间的完整名称，即包含文件后缀在内的完整路径，如填写为`abc/efg/123.jpg`。
>
>- 地域
>
>  Region表示OSS的数据中心所在物理位置。用户可以根据费用、请求来源等选择合适的地域创建Bucket。一般来说，距离用户更近的Region访问速度更快。
>
>- 访问域名
>
>  Endpoint表示OSS对外服务的访问域名。OSS以HTTP RESTful API的形式对外提供服务，当访问不同的Region的时候，需要不同的域名。通过内网和外网访问同一个Region所需要的Endpoint也是不同的。例如杭州Region的外网Endpoint是oss-cn-hangzhou.aliyuncs.com，内网Endpoint是oss-cn-hangzhou-internal.aliyuncs.com。https://help.aliyun.com/zh/oss/user-guide/regions-and-endpoints#concept-zt4-cvy-5db)。
>
>- 访问密钥
>
>  AccessKey简称AK，指的是访问身份验证中用到的AccessKey ID和AccessKey Secret。OSS通过使用AccessKey ID和AccessKey Secret对称加密的方法来验证某个请求的发送者身份。AccessKey ID用于标识用户；AccessKey Secret是用户用于加密签名字符串和OSS用来验证签名字符串的密钥，必须保密。对于OSS来说，AccessKey的来源有：
>
>  - Bucket的拥有者申请的AccessKey。
>  - 被Bucket的拥有者通过RAM授权给第三方请求者的AccessKey。
>  - 被Bucket的拥有者通过STS授权给第三方请求者的AccessKey。
>
>- 原子性和强一致性
>
>  Object操作在OSS上具有原子性，操作要么成功要么失败，不存在中间状态的Object。当Object上传完成时，OSS即可保证读到的Object是完整的，OSS不会返回给用户一个部分上传成功的Object。
>
>  Object操作在OSS同样具有强一致性，当用户收到了上传（PUT）成功的响应时，该上传的Object进入立即可读状态，并且Object的冗余数据已经写入成功。不存在上传的中间状态，即执行read-after-write，却无法读取到数据。对于删除操作，用户删除指定的Object成功之后，该Object立即不存在。

#### 在代码中访问阿里云OSS

API 和 SDK

`API`

请求语法

```C++
PUT /ObjectName HTTP/1.1
Content-Length: ContentLength
Content-Type: ContentType
Host: BucketName.oss-cn-hangzhou.aliyuncs.com
Data: GTM Data
Authorization: SignatureValue
```

API：代码用起来难受一些，但是性能好

`SDK`

阿里云厂商根据不同的开发语言，开发了不同的库文件，只需要在本地安装SDK软件包，以动态库的形式链接到项目中，就不需要发送Http发送请求了，只需要调用API。

SDK：性能稍微差一些，但是用起来简单一些

### C++安装

如果您需要管理OSS存储空间、上传下载文件、管理数据、进行图片处理等，可以先安装OSS C++ SDK。

#### 前提条件

- C++11及以上版本的编译器
- Visual Studio 2013及以上版本
- GCC 4.8及以上版本
- Clang 3.3及以上版本

#### 下载SDK

- [下载SDK安装包](https://gosspublic.alicdn.com/doc/c-sdk/aliyun-oss-cpp-sdk-master.zip)
- [通过GitHub下载](https://github.com/aliyun/aliyun-oss-cpp-sdk.git)

#### 安装SDK

这里以Linux为例

1. 安装CMake并通过CMake生成目标平台的构建脚本。

   1. 下载CMake安装包。

      以下以安装CMake 3.21.1版本为例，如需下载其他版本，请对应替换版本号。

       

      ```shell
      wget https://cmake.org/files/v3.21/cmake-3.21.1.tar.gz
      ```

   2. 解压CMake安装包文件。

       

      ```shell
      tar xvf cmake-3.21.1.tar.gz
      ```

   3. 进入CMake安装目录。

       

      ```shell
      cd cmake-3.21.1/
      ```

   4. 执行自动化构建和配置过程，用于检查系统环境、生成Makefile文件、配置编译选项等。

       

      ```shell
      ./bootstrap
      ```

   5. 根据Makefile文件中的规则，逐个编译源代码文件，生成目标可执行文件或库文件。

       

      ```shell
      make
      ```

   6. 安装编译生成的文件。

       

      ```shell
      sudo make install
      ```

2. 添加环境变量。

   1. 编辑bash配置文件。

       

      ```shell
      vim .bashrc
      ```

   2. 配置环境变量。

       

      ```shell
      CMAKE_PATH=/usr/local/cmake
      export PATH=$CMAKE_PATH/bin:$PATH
      ```

   3. 使bash配置文件更改生效。

       

      ```shell
      source .bashrc
      ```

   4. 查找并显示在当前系统中可执行`cmake`命令的路径。

       

      ```shell
      which cmake
      ```

3. 安装依赖。

    

   ```shell
   yum -y install libcurl-devel openssl-devel unzip
   ```

4. 安装C++ SDK。

   1. 下载C++ SDK安装包。

       

      ```shell
      wget -O aliyun-oss-cpp-sdk-master.zip "https://gosspublic.alicdn.com/doc/c-sdk/aliyun-oss-cpp-sdk-master.zip"
      ```

   2. 解压安装包文件。

       

      ```shell
      unzip aliyun-oss-cpp-sdk-master.zip
      ```

   3. 进入安装目录。

       

      ```shell
      cd aliyun-oss-cpp-sdk-master
      ```

   4. 安装C++ SDK。

      1. 创建用于编译和构建项目的目标目录。

          

         ```shell
         mkdir build
         ```

      2. 切换至目标目录。

          

         ```shell
         cd build
         ```

      3. 通过CMake工具生成项目的构建系统。

          

         ```shell
         cmake ..
         ```

      4. 执行项目的编译过程。

          

         ```shell
         make
         ```

      5. 将编译生成的文件安装到系统指定的目录中。

          

         ```shell
         sudo make install
         sudo ldconfig
         ```

5. 编译示例文件

   ```shell
   g++ test.cpp -std=c++11 -fno-rtti -lalibabacloud-oss-cpp-sdk -lcurl -lcrypto -lpthread -o test.bin
   ```

#### 单体应用的问题

1.开发的角度：需要关心各个模块的耦合

2.性能不够好

3.运维，一崩全崩

#### 微服务架构

(每一个模块称为服务)

用户                            文件

​                数据库

开发人员只需要考虑：1.服务的内部逻辑是什么样子的   2.对外的接口是什么样子的

服务之间可以通过网络通信或者进程间通信类似的形式进行信息交互

优势：低耦合 高内聚

将函数调用替换为网络通信（请求和响应）

### 把函数调用改造成网络通信

`RESTful`风格`API`设计

1.使用`Http`

2.对象--------->资源----------->`url`

3.成员函数/方法 ------------> 方法

4.参数的返回值 ----------> 报文体 `Json/XML`

问题：

1.用的`http`协议

2.使用起来复杂，复杂在请求，解析

3.效率还是比较低（大量解析）

诞生了一种专门的技术：A可以调用位于另一台机器的函数B --------> 远程过程调用 `RPC`

`RPC`的优点：

- 更小的协议开销。
- 更高效的连接管理。
- 更直接的数据传输方式。
- 更多的性能优化空间。
- 更低的实时性延迟。

### 代理模式

`RPC`框架实现

Stub 存根

A客户端                                                      B服务端

Client                                                         Server

​    ^

​    |                                                                  ^

​    |                                                                  |

​    v                                                                  |

​                                                                        v

Client Stub（服务端代理）                 Server Stub

​    ^

​    |                                                                   ^

​    |                                                                   |

​    v                                                                    |

​                                                                          v

Socket ---------------------------------------- >   Socket

### 序列化和反序列化

​                       序列化

对象        <------------------->   字节流

​                       反序列化

序列化          `Json.dump`

反序列化       `Json.parse`

`json`的问题：

1.效率

2.向前兼容，向后兼容

### `protobuf`

专用于`RPC`的序列化方案

优势：

1.二进制

2.天生支持向前向后兼容

3.跨语言

4.特别适用于`RPC`    （自动实现代理模式）

### 安装`protobuf`的流程

1.安装`protobuf`

```	c++
1.sudo apt-get install autoconf automake libtool curl make g++ unzip
2.wget https://github.com/protocolbuffers/protobuf/releases/download/v3.19.4/protobuf-all-3.19.4.tar.gz
3.cd protobuf-3.19.4
4. ./autogen.sh   
5. ./configure
6.make
7.make install
8.protoc --version
```

#### 执行完上述后会装两个软件

`1.libprotobuf.so`

`2.protoc   编译器`  

### 使用`protobuf`

1.写`IDL`文件 （Interface Description Language） 接口描述语言

注册请求  需要发送   `username`      	`password`

消息整体叫`ReqSignup`        有两个字段  有且出现一次    除了`required `还有`optional`  `repeated`

```protobuf
// proto2的格式
message ReqSignup{
	// 消息中有两个字段
	required string username = 1;
	// 字段的重复情况  required 必须出现一次  optional 可以出现0-1次 repeated 可以出现任意次 可以实现数组
	// 字段的数据类型  string  int32
	// 字段的名字
	// 字段的编号 
	required string password = 2;
}

// proto3的格式 required需要省略
syntax = "proto3";
message ReqSignup{
	string username = 1;
	string password = 2;
}
```

### 使用`protoc`把`IDL`文件转化成目标语言文件

```bash
protoc --proto_path=. --cpp_out=. test.proto
# .h是接口文件 .cc是对应接口的实现文件
```

`ReqSignup` 对应的类名

字段名字 对应的方法名 `username()  set_username()`

#### 使用`test.pb.h`

```C++
#include <string>
#include <stdio.h>
#include "test.pb.h"
int main(){
    ReqSignup message;
    std::string username = "admin";
    std::string password = "1234";
    message.set_username(username);
    message.set_password(password);
    // 这个内容通过网络发送到远处
    // 序列化 包装成字节流 通过网络传输
    // std::string output;
    // message.SerializeToString(&output);
    // printf("out = %s\n", output.c_str());
    char arr[100] = {0};
    message.SerializeToArray(arr, 100);
    for(int i = 0; i < 100; ++i){
        fprintf(stderr, "%02x", arr[i]);
    }
    // 反序列化为message
    // message.ParseFromArray();
}
```

#### 如何编译这个程序

```bash
g++ main.cc test.pb.cc -o main -lprotobuf
./main
```

#### 怎么样支持向前向后兼容

向前兼容：比如增加了字段  从2->3 （旧代码怎么处理新消息）

1.新字段使用新编号

2.旧代码遇到新编号，直接忽略字段

3.删除字段要保留编号    

4.给字段改名字没有影响  但是不能换类型

向后兼容：新代码怎么处理就消息（比如这个包传递了很久才到）

1.任何新加的字段 属性设置为optional

### `srpc`

### 使用`rpc`重写注册业务

之前：

Client    ----------------------------> Sever

​               <----------------------------    ^

​                                                        |

​                                                        v

​                                                    handler

现在：

Client ------------------------------>         边缘节点(`API`网关) 调用 注册服务并得到返回值  返回给Client

```protobuf
syntax = "proto3";
service UserService{
	// 用户注册
	rpc Signup(ReqSignup) returns (RespSignup) {}
}
message ReqSignup{
	string username = 1;
	string password = 2;
}

message RespSignup{
	int32 code = 1;
	string message = 2;
}
```

### 利用srpc生成代码

安装srpc

```bash
git clone --recursive https://github.com/sogou/srpc.git
cd srpc
make
sudo make install
```

首先使用`protoc`生成序列化相关代码

```bash
protoc signup.proto --cpp_out=. --proto_path=.
# .pb.h  .pb.cc 消息的接口和实现
```

```bash
srpc_generator protobuf signup.proto .
# .sprc.h client_skeleton.cc server_skeleton.cc
# .sprc.h    rpc的接口和设计
# UserService是命名空间  需要继承Service类，重写Signup虚方法
```

```C++
virtual void Signup(ReqSignup *request, RespSignup *response, srpc::RPCContext *ctx) = 0;
// ReqSignup 是入参  
// RespSignup 是返回值
// srpc::RPCContext 和workflow对接
```

#### 客户端逻辑

```C++
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
}

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
    // signup服务器所在的ip:port
	const char *ip = "127.0.0.1";
	unsigned short port = 1412;
	
    // 相当于stub  服务端在客户端的代理
	UserService::SRPCClient client(ip, port);

	// example for RPC method call
	ReqSignup signup_req;
	//signup_req.set_message("Hello, srpc!");
    // 使用rpc的形式和普通的函数调用一样  但是底层是跨越网络的调用
    // signup_done是回调函数
	client.Signup(&signup_req, signup_done);

	wait_group.wait();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
```

#### 服务端逻辑

```C++
#include "signup.srpc.h"
#include "workflow/WFFacilities.h"

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
		// TODO: fill server logic here
	}
};

int main()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	unsigned short port = 1412;
	SRPCServer server;

    // 注册一个服务
	UserServiceServiceImpl userservice_impl;
	server.add_service(&userservice_impl);

	server.start(port);
	wait_group.wait();
	server.stop();
	google::protobuf::ShutdownProtobufLibrary();
	return 0;
}
```

### 使用`srpc`改造注册的过程

补充`client.pb_skeleton.cc` 和 `server.pb_skeleton.cc`

### 网盘项目的微服务化改造

`C`------------------------> `API`网关 --------------------------> ..........

通过`rpc`进行调用

微服务的好处：

1.开发和治理分离

2.高内聚，低耦合

3.独立部署，独立升级

4.容错性很好

5.实现异构系统 (`rpc`支持跨语言，有些业务用`C++`写，有些业务用`java`写，有些业务用`php`写)

### 注册中心

本质上是一个键值对数据库！ 拉起服务的时候做一次写入（`SET`）的操作，访问服务的时候做一次读（`GET`）操作

注册中心是一个大流量的地方，如果业务越来越复杂，注册中心就会成为一个性能瓶颈（可以通过加机器来解决问题）

解决方案：改为数据库集群

本项目的注册中心没有从头开始实现，使用的是现有的

### `consul`

使用`go`语言开发的，基于`raft`共识算法

`Consul`是一个用于服务发现、健康检查和动态配置的工具，使得分布式系统中的服务能够轻松地相互发现和通信。`ppconsul`是一个用于C++的库，为开发者提供了与Consul进行交互的简单而强大的接口

其它产品：

`Java`     `Zookeeper`  基于`Zab`共识算法

`Go`   `Etcd`    基于`raft`共识算法

`C++`  `braft`    基于`raft`共识算法

三个节点的集群

```bash
# consul1 是集群主节点
docker run --name consul1 -d -p 8500:8500 -p 8301:8301 -p 8302:8302 consul agent -server -bootstrap-exepect 2 -ui bind=0.0.0.0 -client=0.0.0.0
# 主节点绑定的网卡
docker inspect --format '{{.NetworkSeetings.IPAddress}}' consul1
# 启动第二个节点
docker run --name consul2 -d -p 8501:8500 consul agent -server -bind=0.0.0.0 -client=0.0.0.0 -join=172.17.0.7
# 启动第三个节点
docker run --name consul2 -d -p 8502:8500 consul agent -server -bind=0.0.0.0 -client=0.0.0.0 -join=172.17.0.7
```

使用三个节点：共识！！！

### consul支持的操作

用户把consul集群看成单独的节点

1.注册  2.心跳（告诉它我还活着）3.获取服务的`ip`和`port`

`consul` 支持 `Restful API`

也可以使用第三方库/`SDK`     因为我们更喜欢函数调用的方式

安装`ppconsul`

涉及的头文件

```C++
#include<ppconsul/agent.h>
// 链接选项 -lppconsul
```

### 项目细节

在服务端`server.pb_skeleton.cc`中

1.将本服务注册到consul之中

2.创建一个用来访问注册中心的agent

3.开启一个服务

在`main.cc`中 将所用的发给服务端的请求，都改为发送给`API`网关

1.首先通过`httpTask`访问`consul`

2.解析`consul`的响应内容   转为`Json`

3.发起`RPC`调用

#### 不再是单体应用

```C++
// 2 得到SignupService的IP:PORT
std::string url = "http://1.94.50.145:8500/v1/agent/services";
auto httpTask = WFTaskFactory::create_http_task(url, 0, 0, [username, password, resp](WFHttpTask *httpTask){
    // 解析consul的响应内容
    auto consulResp = httpTask->get_resp();
    const void *body;
    size_t size;
    consulResp->get_parsed_body(&body, &size);
    Json services = Json::parse(static_cast<const char *>(body));
    std::string ip = services["SignupService1"]["Address"];
    unsigned short port = services["SignupService1"]["Port"];
    // 3 发起RPC调用
    // 创建一个代理对象
    UserService::SRPCClient client(ip.c_str(), port);
    // 创建一个请求
    ReqSignup reqSignup;
    reqSignup.set_username(username);
    reqSignup.set_password(password);
    // 通过代理对象的方法，调用rpc
    // 使用任务的形式来使用客户端
    auto rpcTask = client.create_Signup_task([resp](RespSignup *response, srpc::RPCContext *ctx){
        if(ctx->success() && response->code() == 0){
            resp->String("SUCCESS");
        }
        else{
            fprintf(stderr, "status = %d, error = &d, errmsg = %s\n",
                 ctx->get_status_code(), ctx->get_error(), ctx->get_errmsg());
            resp->String("FAIL");
        }
    });
    // 修改任务的属性以设置rpc传入参数
    rpcTask->serialize_input(&reqSignup);
    // 启动任务
    series_of(httpTask)->push_back(rpcTask);
});
// 这个httpTask去访问consul
series->push_back(httpTask);
```

如果要将网盘彻底改为微服务，把原来一次性做完的东西（查数据库，各种任务）放到微服务进行处理

`main.cc`中的`API网关`只负责接收客户端的请求，发起`RPC`的调用
