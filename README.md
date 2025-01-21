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
