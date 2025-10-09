#include "Common.h"
#include <iostream>
#include <string>

std::string server_ip_str = "127.0.0.1";
#define SERVER_PORT 9000
#define BUFFER_SIZE 4096

typedef struct {
    char fileName[256];
    long long fileSize;
} FileInfo;

int main(int argc, char* argv[]) {
    int retval;

    if (argc != 2) return 1;

    // 윈속 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup() 실패" << std::endl;
        return 1;
    }

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() 생성 실패" << std::endl;
        WSACleanup();
        return 1;
    }

    // connect
    SOCKADDR_IN servAddr = {};
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip_str.c_str(), &servAddr.sin_addr);
    servAddr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        std::cerr << "connect() 연결 실패" << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "서버에 연결되었습니다..." << std::endl;

    FILE* fp;
    fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        std::cerr << "파일 열기 실패!\n";
        closesocket(sock);
        WSACleanup();
        return 1; 
    }

    // 파일 정보 저장
    FileInfo fileInfo = {};
    fseek(fp, 0, SEEK_END);
    fileInfo.fileSize = _ftelli64(fp);
    rewind(fp);

    // 파일명 복사
    strcpy_s(fileInfo.fileName, sizeof(fileInfo.fileName), argv[1]);

    // 고정 데이터 전송
    if (send(sock, (char*)&fileInfo, sizeof(FileInfo), 0) == SOCKET_ERROR) {
        std::cerr << "헤더 전송 실패" << std::endl;
    }
    else {
        // 가변 데이터 전송
        std::cout << "파일명: " << fileInfo.fileName << ", 파일 크기: " << fileInfo.fileSize << " bytes 전송 시작..." << std::endl;
        char buf[BUFFER_SIZE];
        size_t bytesRead;
        while ((bytesRead = fread(buf, 1, sizeof(buf), fp)) > 0) {
            if (send(sock,buf, bytesRead, 0) == SOCKET_ERROR) {
                std::cerr << "\n데이터 전송 중 실패" << std::endl;
                break;
            }
        }
        std::cout << "파일 전송을 완료했습니다." << std::endl;
    }

    fclose(fp);
    closesocket(sock);
    WSACleanup();
    return 0;
}