// ----------------------------------------------------
//          hw05_client.cpp (전체 코드)
// ----------------------------------------------------

#include "Common.h"       // 5단계에서 추가할 파일
#include "resource.h"     // 3단계에서 자동 생성된 파일
#include <commdlg.h>      // GetOpenFileName()
#include <commctrl.h>     // Progress Bar

// 라이브러리 링크 (중요)
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFFER_SIZE 4096

// 사용자 정의 메시지 (스레드 -> GUI)
#define WM_APP_UPDATE_PROGRESS (WM_APP + 1)
#define WM_APP_TRANSFER_COMPLETE (WM_APP + 2)

// 스레드에 전달할 인자 구조체
typedef struct {
    HWND hDlg;
    char szFilepath[MAX_PATH];
} THREAD_ARGS;

// 함수 프로토타입
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI FileSenderThread(LPVOID arg);
void DisplayText(const char* fmt, ...);
void DisplayError(const char* msg);

// GUI 컨트롤 핸들 (전역 변수)
HWND hStatusEdit;
HWND hProgress;
HWND hBtnSelect;
HWND hBtnSend;
HWND hEditFile;

// WinMain : 프로그램 시작점
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    // 1. 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 2. 공용 컨트롤(프로그레스 바) 초기화
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    // 3. 대화상자 생성
    //    이 ID가 2단계에서 만든 ID(IDD_DIALOG1)와 일치해야 합니다!
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // 4. 윈속 종료
    WSACleanup();
    return 0;
}

// DlgProc : 대화상자 메시지 처리기
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        // 컨트롤 핸들 가져오기
        hStatusEdit = GetDlgItem(hDlg, IDC_EDIT_STATUS);
        hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);
        hBtnSelect = GetDlgItem(hDlg, IDC_BTN_SELECT);
        hBtnSend = GetDlgItem(hDlg, IDC_BTN_SEND);
        hEditFile = GetDlgItem(hDlg, IDC_EDIT_FILEPATH);

        // 프로그레스 바 범위 설정 (0 ~ 100%)
        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        SendMessage(hProgress, PBM_SETPOS, 0, 0);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_SELECT: // [파일 선택] 버튼
        {
            OPENFILENAMEA ofn;
            char szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameA(&ofn) == TRUE) {
                SetWindowTextA(hEditFile, ofn.lpstrFile);
            }
        }
        return TRUE;

        case IDC_BTN_SEND: // [전송 시작] 버튼
        {
            THREAD_ARGS* args = new THREAD_ARGS;
            args->hDlg = hDlg;
            GetWindowTextA(hEditFile, args->szFilepath, MAX_PATH);

            if (strlen(args->szFilepath) == 0) {
                DisplayText("[오류] 전송할 파일을 먼저 선택하세요.\r\n");
                delete args;
                return TRUE;
            }

            EnableWindow(hBtnSelect, FALSE);
            EnableWindow(hBtnSend, FALSE);
            SendMessage(hProgress, PBM_SETPOS, 0, 0);
            DisplayText("[알림] 파일 전송을 시작합니다...\r\n");

            // 파일 전송 스레드 생성 (경고 수정된 버전)
            HANDLE hThread = CreateThread(NULL, 0, FileSenderThread, (LPVOID)args, 0, NULL);
            if (hThread) {
                CloseHandle(hThread); // 성공 시 핸들 닫기
            }
            else {
                DisplayText("[오류] 스레드 생성에 실패했습니다.\r\n");
                EnableWindow(hBtnSelect, TRUE);
                EnableWindow(hBtnSend, TRUE);
                delete args; // 스레드 시작 못했으니 args 해제
            }
        }
        return TRUE;

        case IDCANCEL: // '취소' 버튼 또는 'X' 버튼
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        return FALSE;

        // 스레드로부터 받은 '진행률' 메시지
    case WM_APP_UPDATE_PROGRESS:
    {
        int percent = (int)wParam;
        SendMessage(hProgress, PBM_SETPOS, percent, 0); // 프로그레스 바 업데이트
    }
    return TRUE;

    // 스레드로부터 받은 '전송 완료' 메시지
    case WM_APP_TRANSFER_COMPLETE:
    {
        DisplayText("[알림] 파일 전송이 완료되었습니다.\r\n");
        EnableWindow(hBtnSelect, TRUE);
        EnableWindow(hBtnSend, TRUE);
    }
    return TRUE;

    case WM_CLOSE: // 창 닫기
        EndDialog(hDlg, 0);
        return TRUE;
    }
    return FALSE;
}

// FileSenderThread : 파일 전송 스레드
DWORD WINAPI FileSenderThread(LPVOID arg)
{
    THREAD_ARGS* args = (THREAD_ARGS*)arg;
    HWND hDlg = args->hDlg;
    char* filename = args->szFilepath;

    // 1. 파일 열기
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        DisplayText("[오류] 파일을 열 수 없습니다: %s\r\n", filename);
        PostMessage(hDlg, WM_APP_TRANSFER_COMPLETE, 0, 0); // 버튼 활성화
        delete args;
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    long long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 2. 소켓 생성 및 서버 접속
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        DisplayError("socket()");
        fclose(fp);
        PostMessage(hDlg, WM_APP_TRANSFER_COMPLETE, 0, 0);
        delete args;
        return 1;
    }

    SOCKADDR_IN server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &server_addr.sin_addr);
    server_addr.sin_port = htons(SERVERPORT);

    if (connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        DisplayError("connect()");
        fclose(fp);
        closesocket(sock);
        PostMessage(hDlg, WM_APP_TRANSFER_COMPLETE, 0, 0);
        delete args;
        return 1;
    }
    DisplayText("[알림] 서버에 접속했습니다.\r\n");

    // 3. 파일 크기 전송
    send(sock, (char*)&file_size, sizeof(file_size), 0);

    // 4. 파일 이름 전송 (경로 제외 순수 파일명)
    char send_filename[256] = { 0 };
    char* p = strrchr(filename, '\\');
    if (p) strncpy_s(send_filename, sizeof(send_filename), p + 1, _TRUNCATE);
    else strncpy_s(send_filename, sizeof(send_filename), filename, _TRUNCATE);

    send(sock, send_filename, sizeof(send_filename), 0);
    DisplayText("[알림] 파일명 전송: %s (%lld bytes)\r\n", send_filename, file_size);

    // 5. 파일 데이터 전송 (프로그레스 바 업데이트)
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long long total_sent = 0;
    int current_percent = 0;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        int retval = send(sock, buffer, (int)bytes_read, 0);
        if (retval == SOCKET_ERROR) {
            DisplayError("send()");
            break;
        }
        total_sent += retval;

        // 진행률 계산
        int percent = (int)((double)total_sent * 100 / file_size);
        if (percent > current_percent) {
            current_percent = percent;
            PostMessage(hDlg, WM_APP_UPDATE_PROGRESS, (WPARAM)current_percent, 0);
        }
    }

    // 6. 정리
    fclose(fp);
    closesocket(sock);
    PostMessage(hDlg, WM_APP_UPDATE_PROGRESS, 100, 0); // 100%
    PostMessage(hDlg, WM_APP_TRANSFER_COMPLETE, 0, 0); // 완료
    delete args;
    return 0;
}

// DisplayText : 상태창(에디트 컨트롤) 출력
void DisplayText(const char* fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char cbuf[BUFFER_SIZE * 2];
    vsprintf_s(cbuf, sizeof(cbuf), fmt, arg);
    va_end(arg);

    int nLength = GetWindowTextLength(hStatusEdit);
    SendMessage(hStatusEdit, EM_SETSEL, nLength, nLength);
    SendMessageA(hStatusEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}

// DisplayError : 소켓 오류 출력
void DisplayError(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&lpMsgBuf, 0, NULL);
    DisplayText("[%s] %s\r\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}
