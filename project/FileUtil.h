#ifndef __FILEUTIL__
#define __FILEUTIL__
#include "linuxheader.h"
#include <string>
#include <openssl/sha.h>     // sudo apt install libssl-dev
// 用来计算文件hash值
class FileUtil{
public:
    static std::string sha1File(const char *path){
        int fd = open(path, O_RDONLY);
        char buf[4096] = {0};
        SHA_CTX sha_ctx;
        SHA1_Init(&sha_ctx);
        // 循环update
        while(1){
            bzero(buf, sizeof(buf));
            int ret = read(fd, buf, sizeof(buf));
            if(ret == 0){
                break;
            }
            // 把某一个内存中的内容和长度 加入到sha1的计算中
            SHA1_Update(&sha_ctx, buf, ret);
        }
        // 最后再final
        unsigned char md[20];   // 这不是一个可打印字符 40个16进制数组成
        SHA1_Final(md, &sha_ctx);  // 将结果存储起来
        std::string sha1Res;
        char frag[3];
        for(int i = 0; i < 20; ++i){
            sprintf(frag, "%02x", md[i]);
            sha1Res.append(frag);
        }
        // 把文件的hash值计算出来了
        return sha1Res;
    }
};
#endif