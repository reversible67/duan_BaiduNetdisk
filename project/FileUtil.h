#ifndef __FILEUTIL__
#define __FILEUTIL__
#include "linuxheader.h"
#include <string>
#include <openssl/sha.h>     // sudo apt install libssl-dev
// ���������ļ�hashֵ
class FileUtil{
public:
    static std::string sha1File(const char *path){
        int fd = open(path, O_RDONLY);
        char buf[4096] = {0};
        SHA_CTX sha_ctx;
        SHA1_Init(&sha_ctx);
        // ѭ��update
        while(1){
            bzero(buf, sizeof(buf));
            int ret = read(fd, buf, sizeof(buf));
            if(ret == 0){
                break;
            }
            // ��ĳһ���ڴ��е����ݺͳ��� ���뵽sha1�ļ�����
            SHA1_Update(&sha_ctx, buf, ret);
        }
        // �����final
        unsigned char md[20];   // �ⲻ��һ���ɴ�ӡ�ַ� 40��16���������
        SHA1_Final(md, &sha_ctx);  // ������洢����
        std::string sha1Res;
        char frag[3];
        for(int i = 0; i < 20; ++i){
            sprintf(frag, "%02x", md[i]);
            sha1Res.append(frag);
        }
        // ���ļ���hashֵ���������
        return sha1Res;
    }
};
#endif