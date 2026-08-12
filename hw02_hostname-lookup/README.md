# HW02: 호스트 이름과 IPv4 주소 조회

명령행 인수로 받은 도메인 이름을 Winsock으로 조회해 호스트 별칭과 IPv4 주소 목록을 출력합니다.

## 구현

- `WSAStartup()`으로 Winsock 2.2를 초기화합니다.
- `gethostbyname()`의 `hostent` 결과에서 별칭과 IPv4 주소를 순회합니다.
- 이진 IPv4 주소를 `inet_ntop()`으로 문자열로 변환합니다.

## 빌드와 실행

```powershell
msbuild .\과제2.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
.\x64\Release\과제2.exe example.com
```

`gethostbyname()`은 IPv4 전용 구형 API입니다. 이 과제는 호스트 이름 해석과 `hostent` 구조를 학습하기 위해 해당 API를 사용합니다.
