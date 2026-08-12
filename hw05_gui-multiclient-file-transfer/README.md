# HW05: GUI·다중 클라이언트 파일 전송

Windows 대화상자 GUI에서 파일을 선택하고 송신 진행률을 표시하는 클라이언트와, 클라이언트마다 작업 스레드를 생성하는 TCP 파일 수신 서버입니다.

## 구성

- `hw05_client`: 파일 선택 대화상자, 백그라운드 송신 스레드, 프로그레스 바
- `hw05_server`: 반복 `accept()`, 클라이언트별 스레드, 콘솔 행별 수신 진행률
- `Common/Common.h`: 저장소 루트의 Winsock 공용 헤더

## 프로토콜

HW04와 동일하게 `long long` 파일 크기, 256바이트 파일명, 파일 본문 순서로 송수신합니다. 기본 주소는 `127.0.0.1:9000`입니다.

## 빌드

Developer PowerShell for VS 2022에서 두 솔루션을 각각 빌드합니다.

```powershell
msbuild .\hw05_server\hw05_server.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
msbuild .\hw05_client\hw05_client.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
```

서버를 먼저 실행하고 GUI 클라이언트에서 파일을 선택한 뒤 전송을 시작합니다.

## 제한

현재 구현은 로컬 학습용입니다. 명시적 헤더 직렬화, 인증, 전송 크기 상한, 파일명·저장 경로 검증, 완료 응답을 구현하지 않았으므로 외부 네트워크에 노출하지 않습니다.
