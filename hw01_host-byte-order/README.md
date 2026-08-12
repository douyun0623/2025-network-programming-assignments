# HW01: 호스트 바이트 순서 확인

2바이트 값 `0x0102`가 메모리에 저장된 순서를 조사해 호스트가 Little Endian인지 Big Endian인지 출력하는 프로그램입니다.

## 구현

- `unsigned short` 값의 주소를 `unsigned char*`로 해석합니다.
- 첫 바이트가 `0x02`면 Little Endian, `0x01`이면 Big Endian으로 판별합니다.

## 빌드와 실행

Developer PowerShell for VS 2022에서 실행합니다.

```powershell
msbuild .\InitSocket.sln /m /t:Build /p:Configuration=Release /p:Platform=x64
.\x64\Release\InitSocket.exe
```

Windows x64 환경에서는 일반적으로 `Little Endian`이 출력됩니다.
