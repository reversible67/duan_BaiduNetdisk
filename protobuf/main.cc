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
