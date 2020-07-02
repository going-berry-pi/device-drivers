# Offline Class Management System

> 아주대학교 대면 수업 관리 시스템

![system structure](/images/system-structure.png)

## Contributors

- [경주현](https://github.com/kkjhh0207)
- [박병준](https://github.com/nike0717)
- [박효성](https://github.com/HyoSungP)
- [추연호](https://github.com/younho9)

## Device Drivers

| 디바이스 드라이버           | 사용 디바이스 / 모델명                                       | 관련 시나리오         | 기능                                | 작동 방식                   |
| --------------------------- | ------------------------------------------------------------ | --------------------- | ----------------------------------- | --------------------------- |
| `rfid_dev.c`                | RFID / [RC522](http://mechasolution.com/shop/goods/goods_view.php?goodsno=866&category=) | 교실 입실 (학생 인증) | 학생증을 활용한 간편한 학생 인증    | 카드 터치 / SPI             |
| `keypad_dev.c`              | 4x3 키패드 / [COM-08653](http://mechasolution.com/shop/goods/goods_view.php?goodsno=2323&category=) | 교실 입실 (학생 인증) | 학생증이 없는 경우에 대한 학생 인증 | 버튼 클릭 / GPIO            |
| `infrared_thermopile_dev.c` | 적외선 온도 / [TMP007](http://vctec.co.kr/front/php/product.php?product_no=5154&NaPm=ct%3Dkc4tb77d%7Cci%3Dcheckout%7Ctr%3Dppc%7Ctrx%3D%7Chk%3D4b518817bcd276091c7a8abe5ee8965178db27ce) | 교실 입실 (온도 측정) | 학생의 체온 측정                    | 일정 거리 접근 / I2C        |
| `ultrasonic_dev.c`          | 초음파 센서 / [HC-SR04](http://mechasolution.com/shop/goods/goods_view.php?goodsno=539636&category=) | 교실 입실 (온도 측정) | 체온 측정 시 적정 거리 유지         | 일정 거리 접근 / GPIO       |
| `led_dev.c`                 | LED                                                          | 교실 입실 (온도 측정) | 온도 측정 완료에 대한 피드백        | On, Off 조절 / GPIO         |
| `pressure_dev.c`            | 압력 센서 / [FSR18](http://mechasolution.com/shop/goods/goods_view.php?goodsno=1300&category=) | 좌석 확인             | 착석 여부 확인                      | 압력에 따른 가변 저항 / SPI |
| 없음                        | ADC / [MCP3008](http://mechasolution.com/shop/goods/goods_view.php?goodsno=8067&category=) | 좌석 확인             | 아날로그 가변 저항 값 처리          | SPI                         |
| `motor_dev.c`               | 모터 / [SERVO SG90](http://mechasolution.com/shop/goods/goods_view.php?goodsno=587413&category=) | 원격 창문 제어        | 환기용 창문 여닫기                  | Soft PWM                    |

> 각 폴더의 쉘 스크립트로 디바이스 드라이버를 설치하는 것과  각 디바이스의 앱 코드로 테스트 가능

## Application

| 응용 프로그램       | 설명                | 사용 디바이스                      | 사용 소켓                      |
| ------------------- | ------------------- | ---------------------------------- | ------------------------------ |
| `admin_app.c`       | 관리자 서버 응용    | 없음                               | 클라이언트 소켓 1, 서버 소켓 3 |
| `motor_server.c`    | 모터 서버 응용      | 모터                               | 서버 소켓                      |
| `pressure_client.c` | 좌석 확인 파이 응용 | 압력 센서, ADC                     | 클라이언트 소켓 1              |
| `rfid_server.c`     | 학생 인증 파이 응용 | RFID                               | 클라이언트 소켓 1, 서버 소켓 1 |
| `temperature_app.c` | 온도 측정 파이 응용 | 적외선 온도 센서, 초음파 센서, LED | 클라이언트 소켓 1, 서버 소켓 1 |

> 각 응용의 쉘 스크립트로 필요한 디바이스 드라이버 설치와 응용 프로그램 컴파일 가능

## How to Contribute

### 초기 세팅

1. 팀 repo 를 Fork 하여 개인 repo 로 가져옴

2. 개인 repo 를 clone 하여 로컬로 가져옴

3. 팀 repo 를 remote로 추가

```
git remote add upstream https://github.com/going-berry-pi/f2f-class-management-system.git
```

### 작업 방법

1. 팀 repo 의 변경 사항 확인

```
git fetch upstream
```

2. 변경사항이 있으면 로컬 master 에 변경사항 pull 하기

```
git pull upstream master
```

3. 변경사항 업데이트 후 / 또는 변경사항이 없으면 브랜치 만들어서 작업하기

```
git checkout -b 브랜치-이름
... work ...
git add 작업한-파일
git commit -m "커밋 메시지"
... work ...
git add 작업한-파일
git commit -m "커밋 메시지"
```

4. 작업 내용 푸시하기

```
git push -u origin 브랜치-이름
```

5. Github 에서 PR 작성하기

`New Pull Request` 사용

6. 다른 사람이 코드 리뷰하고 `Squash and Merge` 하기

7. merge 완료되면 팀 repo, 로컬 master 브랜치, Forked repo 동기화

```
git checkout master
git pull upstream master
git push origin master
```

8. 3번부터 다시 작업 시작!

## 폴더 구조

```
# 예시
f2f-class-management-system
  ├── README.md
  ├── led
  │  ├── led_dev.c
  │  ├── led_app.c
  │  ├── Makefile
  │  ├── led_script.sh
  │  └── ...
  ├── button
  │  ├── button_dev.c
  │  ├── button_app.c
  │  ├── Makefile
  │  ├── button_script.sh
  │  └── ...
  ├── ...
  ...
```
