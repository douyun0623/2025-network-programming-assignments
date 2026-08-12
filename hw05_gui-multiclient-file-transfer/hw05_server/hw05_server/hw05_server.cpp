#include "Common.h"

#include <iostream>



#define SERVER_PORT 9000

#define BUFFER_SIZE 4096





typedef struct {

    SOCKET sock;

    int client_id;

    SOCKADDR_IN client_addr;

} THREAD_ARGS;



HANDLE hConsole;

CRITICAL_SECTION cs;



DWORD WINAPI ProcessClient(LPVOID arg) {

    THREAD_ARGS* args = (THREAD_ARGS*)arg;

    SOCKET client_sock = args->sock;

    int client_id = args->client_id;

    SOCKADDR_IN client_addr = args->client_addr;



    char client_ip[16];

    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));



    // 1. 파일 크기 수신

    long long file_size;

    recv(client_sock, (char*)&file_size, sizeof(file_size), 0);



    // 2. 파일 이름 수신

    char filename[256] = { 0 };

    recv(client_sock, filename, sizeof(filename), 0);



    FILE* fp = fopen(filename, "wb");

    if (fp == NULL) {

        printf("파일 생성 실패: %s\n", filename);

        closesocket(client_sock);

        delete args;

        return 1;

    }



    // 3. 파일 데이터 수신 및 저장

    char buffer[BUFFER_SIZE];

    long long total_received = 0;

    int bytes_read;



    while (total_received < file_size) {

        bytes_read = recv(client_sock, buffer, BUFFER_SIZE, 0);

        if (bytes_read <= 0) {

            break;

        }

        fwrite(buffer, 1, bytes_read, fp);

        total_received += bytes_read;



        // 전송률 표시

        EnterCriticalSection(&cs);



        COORD pos = { 0, static_cast<SHORT>((client_id % 30000) + 8) };

        SetConsoleCursorPosition(hConsole, pos);



        int percent = (int)((double)total_received * 100 / file_size);

        printf("[%-15s] %-20s ( %3d%% )", client_ip, filename, percent);



        LeaveCriticalSection(&cs);

    }



    // --- 최종 완료 메시지 표시 ---

    EnterCriticalSection(&cs);

    COORD pos = { 0, static_cast<SHORT>((client_id % 30000) + 8) };

    SetConsoleCursorPosition(hConsole, pos);

    printf("[%-15s] %-20s ( 100%% ) - 완료\n", client_ip, filename);

    LeaveCriticalSection(&cs);



    fclose(fp);

    closesocket(client_sock);

    delete args;

    return 0;

}



int main(int argc, char* argv[]) {



    int retval;



    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    InitializeCriticalSection(&cs);



    // 윈속 초기화

    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {

        std::cerr << "WSAStartup() 실패" << std::endl;

        return 1;

    }



    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_sock == INVALID_SOCKET) {

        std::cerr << "socket() 생성 실패" << std::endl;

        WSACleanup();

        return 1;

    }



    // bind()

    SOCKADDR_IN server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    server_addr.sin_port = htons(SERVER_PORT);

    retval = bind(listen_sock, (SOCKADDR*)&server_addr, sizeof(server_addr));

    if (retval == SOCKET_ERROR) {

        std::cerr << "bind() 실패" << std::endl;

        closesocket(listen_sock);

        WSACleanup();

        return 1;

    }



    // listen()

    retval = listen(listen_sock, SOMAXCONN);

    if (retval == SOCKET_ERROR) {

        std::cerr << "listen() 실패" << std::endl;

        closesocket(listen_sock);

        WSACleanup();

        return 1;

    }



    printf("서버 시작... 클라이언트 접속 대기 중...");



    int client_count = 0;

    while (1) {

        SOCKADDR_IN client_addr;

        int addr_len = sizeof(client_addr);

        SOCKET client_sock = accept(listen_sock, (SOCKADDR*)&client_addr, &addr_len);

        if (client_sock == INVALID_SOCKET) continue;



        // 스레드에 전달할 인자 동적 할당

        THREAD_ARGS* args = new THREAD_ARGS;

        args->sock = client_sock;

        args->client_id = client_count++;

        args->client_addr = client_addr;



        // 클라이언트 처리를 위한 스레드 생성

        HANDLE hThread = CreateThread(NULL, 0, ProcessClient, (LPVOID)args, 0, NULL);

        CloseHandle(hThread);

    }



    closesocket(listen_sock);

    DeleteCriticalSection(&cs);

    WSACleanup();

    return 0;

}
