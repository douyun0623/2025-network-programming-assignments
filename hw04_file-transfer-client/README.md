# HW04: 콘솔 파일 송신 클라이언트

서버 IPv4 주소와 전송할 파일 경로를 명령행 인수로 받아 TCP 9000번 포트로 파일을 송신하는 클라이언트입니다.

## 프로토콜

클라이언트는 하나의 TCP 연결에서 다음 순서로 데이터를 보냅니다.

1. `long long` 파일 크기
2. 256바이트 파일명 버퍼
3. 파일 본문

이 프로토콜은 HW05 서버와 호환됩니다. `FileInfo` 구조체를 사용하는 HW03 서버와는 호환되지 않습니다.

## 빌드와 실행

```powershell
msbuild .\client.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
.\x64\Release\client.exe 127.0.0.1 C:\path\to\sample.bin
```

서버를 먼저 실행한 뒤 클라이언트를 실행합니다. 현재 구현은 학습용 네이티브 프로토콜이며 인증, 전송 크기 상한, 완료 응답을 포함하지 않습니다.
