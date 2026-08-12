#include <stdio.h>

// 호스트의 바이트 정렬 방식(Endianness)을 확인하고 결과를 출력하는 함수
void check_host_byte_order() {
    // 2바이트 데이터 0x0102를 선언 (상위 바이트: 0x01, 하위 바이트: 0x02)
    unsigned short test_num = 0x0102;

    // test_num의 메모리를 1바이트 단위로 읽기 위해
    // unsigned char 포인터로 해석한다.
    const unsigned char* byte_pointer =
        reinterpret_cast<const unsigned char*>(&test_num);

    printf("호스트의 바이트 정렬 방식: ");

    // 메모리의 첫 바이트 값이 0x01인지 0x02인지 확인
    if (byte_pointer[0] == 0x02) {
        // 첫 바이트에 하위 바이트(0x02)가 저장되어 있으면 리틀 엔디언
        printf("Little Endian\n");
    }
    else if (byte_pointer[0] == 0x01) {
        // 첫 바이트에 상위 바이트(0x01)가 저장되어 있으면 빅 엔디언
        printf("Big Endian\n");
    }
    else {
        printf("알 수 없음\n");
    }
}

int main() {
    // 정의된 함수를 호출하여 결과를 확인
    check_host_byte_order();
    return 0;
}
