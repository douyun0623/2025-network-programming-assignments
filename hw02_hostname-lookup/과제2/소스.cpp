#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <cstring>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "사용법: " << argv[0] << " <hostname>" << std::endl;
        return 1;
    }

    WSADATA wsa_data;
    const int startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (startup_result != 0) {
        std::cerr << "WSAStartup() 실패: " << startup_result << std::endl;
        return 1;
    }

    const char* hostname = argv[1];
    std::cout << "입력한 호스트: " << hostname << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // 과제 요건인 hostent 구조 학습을 위해 IPv4 전용 구형 API를 사용한다.
    hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        std::cerr << "gethostbyname() 실패: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    if (host->h_addrtype != AF_INET || host->h_length != sizeof(in_addr)) {
        std::cerr << "IPv4 호스트 정보가 아닙니다." << std::endl;
        WSACleanup();
        return 1;
    }

    std::cout << "[별칭 목록]" << std::endl;
    if (host->h_aliases[0] == nullptr) {
        std::cout << "  (별칭 없음)" << std::endl;
    }
    else {
        for (int i = 0; host->h_aliases[i] != nullptr; ++i) {
            std::cout << "  " << host->h_aliases[i] << std::endl;
        }
    }

    std::cout << "\n[IPv4 주소 목록]" << std::endl;
    if (host->h_addr_list[0] == nullptr) {
        std::cout << "  (IPv4 주소 없음)" << std::endl;
    }
    else {
        for (int i = 0; host->h_addr_list[i] != nullptr; ++i) {
            in_addr address;
            std::memcpy(&address, host->h_addr_list[i], sizeof(address));

            char ip_buffer[INET_ADDRSTRLEN] = {};
            if (inet_ntop(AF_INET, &address, ip_buffer, sizeof(ip_buffer)) != nullptr) {
                std::cout << "  " << ip_buffer << std::endl;
            }
            else {
                std::cerr << "inet_ntop() 실패: " << WSAGetLastError() << std::endl;
            }
        }
    }

    WSACleanup();
    return 0;
}
