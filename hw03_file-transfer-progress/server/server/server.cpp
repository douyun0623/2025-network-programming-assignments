#include "Common.h"
#include <iostream>
#include <cctype>
#include <string>

#define SERVER_PORT 9000
#define BUFFER_SIZE 4096
#define RECEIVE_DIRECTORY "received_files"

typedef struct {
	char fileName[256];
	long long fileSize;
} FileInfo;

bool IsReservedWindowsFileName(const std::string& fileName) {
	size_t extensionPosition = fileName.find('.');
	std::string baseName = fileName.substr(0, extensionPosition);
	while (!baseName.empty() && baseName.back() == ' ') {
		baseName.pop_back();
	}
	for (char& character : baseName) {
		character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
	}

	if (baseName == "CON" || baseName == "PRN" || baseName == "AUX" || baseName == "NUL") {
		return true;
	}

	return baseName.length() == 4
		&& (baseName.compare(0, 3, "COM") == 0 || baseName.compare(0, 3, "LPT") == 0)
		&& baseName[3] >= '1' && baseName[3] <= '9';
}

bool IsSafeFileName(const std::string& fileName) {
	if (fileName.empty() || fileName == "." || fileName == "..") {
		return false;
	}

	if (fileName.find_first_of("\\/:*?\"<>|") != std::string::npos
		|| fileName.back() == '.' || fileName.back() == ' ') {
		return false;
	}

	for (unsigned char character : fileName) {
		if (character < 32) {
			return false;
		}
	}

	return !IsReservedWindowsFileName(fileName);
}

bool EnsureReceiveDirectory() {
	if (CreateDirectoryA(RECEIVE_DIRECTORY, NULL)) {
		return true;
	}

	if (GetLastError() != ERROR_ALREADY_EXISTS) {
		return false;
	}

	DWORD attributes = GetFileAttributesA(RECEIVE_DIRECTORY);
	return attributes != INVALID_FILE_ATTRIBUTES
		&& (attributes & FILE_ATTRIBUTE_DIRECTORY)
		&& !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

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
	if (retval != static_cast<int>(sizeof(FileInfo))) {
		std::cerr << "고정 데이터 수신에 실패" << std::endl;
	}
	else if (fileInfo.fileSize < 0) {
		std::cerr << "잘못된 파일 크기를 수신했습니다." << std::endl;
	}
	else {
		const char* nullTerminator = static_cast<const char*>(memchr(fileInfo.fileName, '\0', sizeof(fileInfo.fileName)));
		if (nullTerminator == NULL) {
			std::cerr << "종료 문자가 없는 파일명을 거부했습니다." << std::endl;
		}
		else {
			std::string receivedFileName(fileInfo.fileName, nullTerminator - fileInfo.fileName);
			if (!IsSafeFileName(receivedFileName)) {
				std::cerr << "디렉터리 요소 또는 Windows에서 허용되지 않는 파일명을 거부했습니다." << std::endl;
			}
			else if (!EnsureReceiveDirectory()) {
				std::cerr << "수신 디렉터리를 준비하지 못했습니다." << std::endl;
			}
			else {
				std::string outputPath = std::string(RECEIVE_DIRECTORY) + "\\" + receivedFileName;
				std::cout << "수신할 파일명: " << receivedFileName << ", 파일 크기: " << fileInfo.fileSize << " bytes" << std::endl;

				HANDLE fileHandle = CreateFileA(
					outputPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
				if (fileHandle == INVALID_HANDLE_VALUE) {
					DWORD createError = GetLastError();
					if (createError == ERROR_FILE_EXISTS || createError == ERROR_ALREADY_EXISTS) {
						std::cerr << "같은 이름의 수신 파일이 이미 있어 덮어쓰지 않습니다." << std::endl;
					}
					else {
						std::cerr << "수신 파일 생성에 실패했습니다." << std::endl;
					}
				}
				else {
					char buffer[BUFFER_SIZE];
					long long totalReceived = 0;
					bool receiveSucceeded = true;
					while (totalReceived < fileInfo.fileSize) {
						long long remainingBytes = fileInfo.fileSize - totalReceived;
						int receiveSize = remainingBytes < BUFFER_SIZE
							? static_cast<int>(remainingBytes) : BUFFER_SIZE;
						int recvCount = recv(client_sock, buffer, receiveSize, 0);
						if (recvCount <= 0) {
							std::cerr << "\n연결이 끊어짐" << std::endl;
							receiveSucceeded = false;
							break;
						}

						DWORD totalWritten = 0;
						while (totalWritten < static_cast<DWORD>(recvCount)) {
							DWORD writtenBytes = 0;
							if (!WriteFile(fileHandle, buffer + totalWritten,
								static_cast<DWORD>(recvCount) - totalWritten, &writtenBytes, NULL)
								|| writtenBytes == 0) {
								std::cerr << "\n파일 저장에 실패했습니다." << std::endl;
								receiveSucceeded = false;
								break;
							}
							totalWritten += writtenBytes;
						}

						if (!receiveSucceeded) {
							break;
						}

						totalReceived += recvCount;
						int percentage = static_cast<int>((static_cast<double>(totalReceived) / fileInfo.fileSize) * 100.0);
						std::cout << "\r수신 진행률: " << percentage << "% (" << totalReceived << " / " << fileInfo.fileSize << " bytes) " << std::flush;
					}

					CloseHandle(fileHandle);
					if (receiveSucceeded && totalReceived == fileInfo.fileSize) {
						std::cout << "\n파일 수신을 완료했습니다: " << outputPath << std::endl;
					}
					else {
						if (DeleteFileA(outputPath.c_str())) {
							std::cerr << "불완전한 수신 파일을 제거했습니다." << std::endl;
						}
						else {
							std::cerr << "불완전한 수신 파일을 제거하지 못했습니다." << std::endl;
						}
					}
				}
			}
		}
	}

	closesocket(client_sock);
	closesocket(listen_sock);
	WSACleanup();
	return 0;
}
