#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 9000
#define BUFFER_SIZE 8192
#define FILE_NAME_SIZE 256

bool SendAll(SOCKET socket, const char* data, int size) {
    int total_sent = 0;
    while (total_sent < size) {
        const int sent = send(socket, data + total_sent, size - total_sent, 0);
        if (sent == SOCKET_ERROR || sent == 0) {
            return false;
        }
        total_sent += sent;
    }
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("사용법: %s <server_ip> <filename>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    const char* filepath = argv[2];
    FILE* file = fopen(filepath, "rb");
    if (file == nullptr) {
        printf("파일을 열 수 없습니다: %s\n", filepath);
        return 1;
    }

    if (_fseeki64(file, 0, SEEK_END) != 0) {
        printf("파일 크기를 확인하지 못했습니다.\n");
        fclose(file);
        return 1;
    }
    const long long file_size = _ftelli64(file);
    if (file_size < 0 || _fseeki64(file, 0, SEEK_SET) != 0) {
        printf("파일 크기를 확인하지 못했습니다.\n");
        fclose(file);
        return 1;
    }

    const char* filename = filepath;
    const char* slash = strrchr(filepath, '\\');
    const char* forward_slash = strrchr(filepath, '/');
    if (slash != nullptr && (forward_slash == nullptr || slash > forward_slash)) {
        filename = slash + 1;
    }
    else if (forward_slash != nullptr) {
        filename = forward_slash + 1;
    }

    const size_t filename_length = strlen(filename);
    if (filename_length == 0 || filename_length >= FILE_NAME_SIZE) {
        printf("파일명은 1~%d바이트여야 합니다.\n", FILE_NAME_SIZE - 1);
        fclose(file);
        return 1;
    }

    WSADATA wsa_data;
    const int startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (startup_result != 0) {
        printf("WSAStartup() 실패: %d\n", startup_result);
        fclose(file);
        return 1;
    }

    SOCKET socket_handle = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_handle == INVALID_SOCKET) {
        printf("socket() 실패: %d\n", WSAGetLastError());
        fclose(file);
        WSACleanup();
        return 1;
    }

    SOCKADDR_IN server_address = {};
    server_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, server_ip, &server_address.sin_addr) != 1) {
        printf("잘못된 IPv4 주소입니다: %s\n", server_ip);
        closesocket(socket_handle);
        fclose(file);
        WSACleanup();
        return 1;
    }
    server_address.sin_port = htons(SERVER_PORT);

    if (connect(socket_handle, reinterpret_cast<SOCKADDR*>(&server_address),
        sizeof(server_address)) == SOCKET_ERROR) {
        printf("서버 접속 실패: %d\n", WSAGetLastError());
        closesocket(socket_handle);
        fclose(file);
        WSACleanup();
        return 1;
    }
    printf("서버에 접속했습니다.\n");

    char send_filename[FILE_NAME_SIZE] = {};
    memcpy(send_filename, filename, filename_length);

    bool success = SendAll(socket_handle, reinterpret_cast<const char*>(&file_size),
        static_cast<int>(sizeof(file_size)));
    if (success) {
        success = SendAll(socket_handle, send_filename, sizeof(send_filename));
    }

    char buffer[BUFFER_SIZE];
    while (success) {
        const size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if (bytes_read == 0) {
            success = !ferror(file);
            break;
        }
        success = SendAll(socket_handle, buffer, static_cast<int>(bytes_read));
    }

    if (success) {
        printf("클라이언트 송신 완료: %s (%lld bytes)\n", filename, file_size);
    }
    else {
        printf("파일 송신 실패: %d\n", WSAGetLastError());
    }

    fclose(file);
    closesocket(socket_handle);
    WSACleanup();
    return success ? 0 : 1;
}
