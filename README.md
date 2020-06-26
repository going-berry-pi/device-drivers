# Device Drivers

각자 작성한 디바이스 드라이버 코드를 모아놓는 공간입니다.

## PR 규칙

### 초기 세팅

1. 팀 repo 를 Fork 하여 개인 repo 로 가져옴

2. 개인 repo 를 clone 하여 로컬로 가져옴

3. 팀 repo 를 remote로 추가

```
git remote add upstream https://github.com/going-berry-pi/device-drivers.git
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

7. merge 완료되면 팀 repo,  로컬 master 브랜치, Forked repo 동기화

```
git checkout master
git pull upstream master
git push origin master
```

8. 3번부터 다시 작업 시작!

## 폴더 구조

```
# 예시
device-drivers
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

