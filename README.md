# Winsock2 TCP File Transfer

C++와 Winsock2로 파일 메타데이터와 본문을 TCP 스트림으로 전송하고, 서버 콘솔에 수신 진행률을 표시하는 네트워크 프로그래밍 개인 과제입니다.

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
- 메타데이터와 본문을 순서대로 받아 현재 작업 디렉터리에 저장합니다.
- 누적 수신 바이트를 이용해 콘솔 한 줄에서 진행률을 갱신합니다.

현재 프로토콜은 `fileName[256]`과 `fileSize`를 담은 네이티브 구조체를 그대로 전송한 뒤 파일 본문을 이어 보내는 학습용 구성입니다.

## 빌드

Developer PowerShell for VS 2022에서 서버와 클라이언트를 각각 빌드합니다.

~~~powershell
msbuild .\hw03_file-transfer-progress\server\server.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild .\hw03_file-transfer-progress\client\client.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
~~~

공용 헤더 경로가 설정된 `Release | x64` 구성을 사용합니다.

## 실행

서버가 받은 파일명을 현재 작업 디렉터리에 생성하므로 서버와 클라이언트의 빌드 출력 폴더를 분리해 사용합니다. 먼저 권리를 보유한 작은 테스트 파일을 클라이언트 출력 폴더에 복사합니다.

~~~powershell
Copy-Item C:\path\to\sample.bin .\hw03_file-transfer-progress\client\client\x64\Release\sample.bin
~~~

터미널 A에서 서버를 먼저 실행합니다.

~~~powershell
Set-Location .\hw03_file-transfer-progress\server\server\x64\Release
.\server.exe
~~~

터미널 B에서 클라이언트와 파일명을 전달합니다.

~~~powershell
Set-Location .\hw03_file-transfer-progress\client\client\x64\Release
.\client.exe sample.bin
~~~

## 빌드 및 실행 확인

Windows와 Visual Studio 2022/MSVC v143에서 `Release | x64` 빌드를 확인했습니다. 로컬 루프백으로 824,516바이트 파일을 전송한 뒤 외부 SHA-256 비교에서 송신본과 수신본이 일치했습니다. 애플리케이션 자체에 체크섬 검증 기능이 구현된 것은 아닙니다.

## 현재 상태와 주의

이 저장소는 로컬 학습용 프로토타입입니다. 부분 송수신 반복 처리, 명시적 헤더 직렬화, 수신 파일명의 경로 검증과 기존 파일 덮어쓰기 방지가 아직 구현되어 있지 않으므로 신뢰할 수 없는 클라이언트나 외부 네트워크에서 사용하면 안 됩니다.

저장소의 대용량 테스트 영상은 출처와 사용 권한이 문서화되어 있지 않아, 공개 포트폴리오에서는 권리가 확인된 작은 샘플로 교체하는 편이 적절합니다.

