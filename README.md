# 2025 네트워크 프로그래밍 과제

C++와 Winsock2로 작성한 네트워크 프로그래밍 개인 과제 모음입니다. Visual Studio의 임시 파일과 빌드 산출물은 제외하고, 소스·프로젝트 설정·필요한 리소스만 보관합니다.

## 과제 목록

| 과제 | 주제 | 핵심 내용 |
| --- | --- | --- |
| [HW01](./hw01_host-byte-order/) | 호스트 바이트 순서 | 메모리의 첫 바이트를 확인해 Little/Big Endian 판별 |
| [HW02](./hw02_hostname-lookup/) | 호스트 이름 조회 | 도메인의 별칭과 IPv4 주소 출력 |
| [HW03](./hw03_file-transfer-progress/) | TCP 파일 전송 | 파일 메타데이터·본문 전송과 수신 진행률 표시 |
| [HW04](./hw04_file-transfer-client/) | 콘솔 파일 송신 클라이언트 | 서버 IP와 파일 경로를 인수로 받아 송신 |
| [HW05](./hw05_gui-multiclient-file-transfer/) | GUI·다중 클라이언트 파일 전송 | GUI 송신 진행률과 스레드 기반 서버 |

## 공통 환경

- Windows 10/11
- Visual Studio 2022, MSVC v143
- Windows SDK 10.0
- Winsock2 (`ws2_32.lib`)

HW03은 `FileInfo` 구조체를 사용하고, HW04와 HW05는 `파일 크기 → 256바이트 파일명 → 파일 본문` 순서를 사용합니다. 따라서 HW04 클라이언트는 HW05 서버와 호환되지만 HW03 서버와는 호환되지 않습니다.

## 검증

2026-08-13에 Visual Studio 2022로 HW01~HW05의 7개 솔루션을 `Release | x64`로 빌드했습니다. HW01은 Little Endian 판별, HW02는 `localhost` 조회를 실행했고, HW04 클라이언트에서 HW05 서버로 8,192바이트 파일을 전송해 송수신본의 SHA-256 일치를 확인했습니다.

## HW03: Winsock2 TCP 파일 전송

C++와 Winsock2로 파일 메타데이터와 본문을 TCP 스트림으로 전송하고, 서버 콘솔에 수신 진행률을 표시합니다.

| 항목 | 내용 |
| --- | --- |
| 구현 범위 | TCP 클라이언트·서버, 파일 메타데이터·본문 전송, 수신 진행률 |
| 기술 | C++, Winsock2, TCP/IP, Visual Studio 2022, MSVC v143 |
| 기본 연결 | 클라이언트 `127.0.0.1:9000` → 서버 `0.0.0.0:9000` |
| 현재 상태 | `Release x64` 빌드 및 로컬 루프백 전송 확인 |

## 핵심 구현

### 클라이언트

- 명령행 인수로 전송할 파일을 선택합니다.
- 파일명과 크기를 `FileInfo` 구조체로 먼저 전송합니다.
- 본문을 최대 4,096바이트 단위로 읽어 TCP 소켓으로 전송합니다.

### 서버

- TCP 9000번 포트에서 한 클라이언트의 연결을 수락합니다.
- 메타데이터와 본문을 순서대로 받아 서버 작업 디렉터리의 `received_files` 폴더에 저장합니다.
- 디렉터리 요소와 Windows 예약 이름이 포함된 파일명은 거부하고, 같은 이름의 파일이 있으면 덮어쓰지 않습니다.
- 누적 수신 바이트를 이용해 콘솔 한 줄에서 진행률을 갱신합니다.

현재 프로토콜은 `fileName[256]`과 `fileSize`를 담은 네이티브 구조체를 그대로 전송한 뒤 파일 본문을 이어 보내는 학습용 구성입니다.

## HW03 빌드

Developer PowerShell for VS 2022에서 서버와 클라이언트를 각각 빌드합니다.

~~~powershell
msbuild .\hw03_file-transfer-progress\server\server.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild .\hw03_file-transfer-progress\client\client.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
~~~

공용 헤더 경로가 설정된 `Release | x64` 구성을 사용합니다.

## HW03 실행

서버와 클라이언트의 실행 파일이 서로 다른 출력 폴더에 생성되므로 두 터미널의 작업 폴더를 아래처럼 분리합니다. 먼저 권리를 보유한 작은 테스트 파일을 클라이언트 출력 폴더에 복사합니다. 클라이언트에는 경로가 아닌 파일명만 전달합니다.

~~~powershell
Copy-Item C:\path\to\sample.bin .\hw03_file-transfer-progress\client\x64\Release\sample.bin
~~~

터미널 A에서 서버를 먼저 실행합니다.

~~~powershell
Set-Location .\hw03_file-transfer-progress\server\x64\Release
.\server.exe
~~~

터미널 B에서 클라이언트와 파일명을 전달합니다.

~~~powershell
Set-Location .\hw03_file-transfer-progress\client\x64\Release
.\client.exe sample.bin
~~~

수신이 완료되면 파일은 서버 출력 폴더 아래의 `received_files\sample.bin`에 생성됩니다. 같은 이름의 파일이 이미 있으면 서버가 새 전송을 거부하므로, 다시 테스트하려면 기존 수신 파일의 이름을 바꾸거나 직접 확인 후 제거합니다. 수신 도중 연결 또는 저장이 실패해 생긴 부분 파일은 서버가 제거합니다.

## HW03 빌드 및 실행 확인

Windows와 Visual Studio 2022/MSVC v143에서 서버와 클라이언트의 `Release | x64` 빌드를 확인했습니다. 변경 후 로컬 루프백으로 1,474바이트 파일을 전송해 송신본과 수신본의 SHA-256이 일치했고, 경로가 포함된 파일명 거부와 기존 파일 무덮어쓰기, 중단된 전송의 부분 파일 제거도 확인했습니다. 애플리케이션 자체에 체크섬 검증 기능이 구현된 것은 아닙니다.

## HW03 현재 상태와 주의

이 저장소는 로컬 학습용 프로토타입입니다. 서버는 단순 파일명만 허용하고 `received_files` 밖의 경로 접근과 기존 파일 덮어쓰기를 막습니다. 다만 부분 `send()` 반복 처리, 명시적 헤더 직렬화, 전송 크기 상한, 인증, 애플리케이션 수준 완료 응답은 구현되어 있지 않으므로 신뢰할 수 없는 클라이언트나 외부 네트워크에서 사용하면 안 됩니다.

저장소의 대용량 테스트 영상은 출처와 사용 권한이 문서화되어 있지 않아, 공개 포트폴리오에서는 권리가 확인된 작은 샘플로 교체하는 편이 적절합니다.


