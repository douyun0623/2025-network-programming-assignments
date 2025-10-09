#include "Common.h"
#include <iostream>

#define SERVER_PORT 9000
#define BUFFER_SIZE 4096

typedef struct {
	char fileName[256];
	long long fileSize;
} FileInfo;

int main() {

	int retval;

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
	SOCKADDR_IN serveraddr = {};
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVER_PORT);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
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

	std::cout << "파일 수신 서버가 시작되었습니다. 클라이언트의 접속을 기다립니다..." << std::endl;

	SOCKET client_sock;
	SOCKADDR_IN clientAddr;
	int addrlen;

	// accept()
	addrlen = sizeof(clientAddr);
	client_sock = accept(listen_sock, (SOCKADDR*)&clientAddr, &addrlen);
	if (client_sock == INVALID_SOCKET) {
		std::cerr << "accept() 실패" << std::endl;
		closesocket(listen_sock);
		WSACleanup();
		return 1;
	}

	// 접속한 클라이언트 정보 출력
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, addr, sizeof(addr));
	std::cout << "\n[TCP 서버] 클라이언트 접속: IP 주소=" << addr << ", 포트 번호=" << ntohs(clientAddr.sin_port) << std::endl;

	// 데이터 통신   
	FileInfo fileInfo = {};

	// 고정 크기 데이터 받기
	retval = recv(client_sock, (char*)&fileInfo, sizeof(FileInfo), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		std::cerr << "고정 데이터 수신에 실패" << std::endl;
	}
	else {
		std::cout << "수신할 파일명: " << fileInfo.fileName	<< ", 파일 크기: " << fileInfo.fileSize << " bytes" << std::endl;

		// 가변 데이터 받기
		FILE* fp;
		fp = fopen(fileInfo.fileName, "wb");
		if (fp == NULL) {
			std::cerr << "파일을 생성 실패 " << std::endl;
		}
		else {
			char buffer[BUFFER_SIZE];
			long long totalReceived = 0;
			while (totalReceived < fileInfo.fileSize) {
				int recvCount = recv(client_sock, buffer, BUFFER_SIZE, 0);
				if (recvCount <= 0) {
					std::cerr << "\n연결이 끊어짐" << std::endl;
					break;
				}
				fwrite(buffer, 1, recvCount, fp);
				totalReceived += recvCount;

				int percentage = static_cast<int>((static_cast<double>(totalReceived) / fileInfo.fileSize) * 100.0);
				std::cout << "\r수신 진행률: " << percentage << "% (" << totalReceived << " / " << fileInfo.fileSize << " bytes) " << std::flush;
			}

			if (totalReceived >= fileInfo.fileSize) {
				std::cout << "\n파일 수신을 완료했습니다." << std::endl;
			}
			fclose(fp);
		}
	}

	closesocket(client_sock);
	closesocket(listen_sock);
	WSACleanup();
	return 0;
}