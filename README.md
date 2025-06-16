# Snake Game 🐍

C++와 ncurses 라이브러리를 사용하여 개발한 고급 스네이크 게임입니다.

## 🎮 게임 특징

- **4개의 다양한 스테이지**: BASIC, MAZE, ISLANDS, CROSS
- **아이템 시스템**: 성장(+), 독(-), 시간(T) 아이템
- **게이트 시스템**: 순간이동 기능
- **미션 시스템**: 각 스테이지별 목표 달성
- **점수 시스템**: 성과 기반 점수 계산

## 📋 요구사항

- **C++11** 이상
- **ncurses** 라이브러리
- **CMake** 3.12 이상 또는 **Make**

## 🚀 설치 및 실행

### 1단계: 필요한 라이브러리 설치

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libncurses5-dev
```

#### macOS:
```bash
# Homebrew 사용
brew install cmake ncurses

# 또는 MacPorts 사용
sudo port install ncurses +universal
```

#### CentOS/RHEL:
```bash
sudo yum install gcc-c++ cmake ncurses-devel
```

### 2단계: 프로젝트 클론 및 빌드

```bash
# 저장소 클론
git clone https://github.com/your-username/snake-game.git
cd snake-game

# 방법 1: CMake 사용 (권장)
mkdir build
cd build
cmake ..
make
./SnakeGame

# 방법 2: Makefile 직접 사용
make
./bin/snake_game
```

## 🎯 게임 조작법

| 키 | 기능 |
|---|---|
| ↑↓←→ | 스네이크 이동 |
| P | 게임 시작/재시작 |
| E | 게임 종료 |

## 🎲 게임 규칙

### 기본 규칙
1. **움직임**: 스네이크는 방향키로 조작하며, 반대 방향으로는 움직일 수 없습니다
2. **충돌**: 벽이나 자신의 몸에 부딪히면 게임 오버
3. **아이템**: 
   - 🟢 **성장 아이템 (+)**: 스네이크 길이 증가
   - 🔴 **독 아이템 (-)**: 스네이크 길이 감소 (3 미만이면 게임 오버)
   - ⏰ **시간 아이템 (T)**: 일시적으로 속도 1.5배 증가
4. **게이트**: 한 쌍의 게이트를 통한 순간이동

### 스테이지별 특징
- **BASIC**: 기본 맵으로 게임 익히기
- **MAZE**: 복잡한 미로 구조
- **ISLANDS**: 분리된 섬들과 게이트 활용
- **SPIRAL**: 나선형 구조의 도전적인 맵
- **CROSS**: 십자형 구조의 최종 스테이지

## 📁 프로젝트 구조

```
snake-game/
├── src/
│   ├── main.cpp      # 게임 시작점 및 메인 루프
│   ├── game.h        # 게임 로직 및 상태 관리
│   ├── map.h         # 맵 데이터 및 스테이지 관리
│   └── block.h       # 게임 오브젝트 클래스들
├── CMakeLists.txt    # CMake 빌드 설정
├── Makefile          # Make 빌드 설정
├── README.md         # 프로젝트 문서
└── .gitignore        # Git 제외 파일 목록
```

## 🏆 점수 계산 시스템

점수는 다음 요소들을 기반으로 계산됩니다:
- **B**: 현재 길이 / 최대 길이 비율
- **+**: 획득한 성장 아이템 수
- **-**: 획득한 독 아이템 수  
- **G**: 통과한 게이트 수
- **시간**: 게임 플레이 시간

## 🔧 문제 해결

### 빌드 오류 시
```bash
# 빌드 디렉토리 정리
rm -rf build/
mkdir build && cd build
cmake .. && make
```

### ncurses 라이브러리 오류 시
```bash
# 라이브러리 재설치
sudo apt-get remove libncurses5-dev
sudo apt-get install libncurses5-dev
```

## 🤝 기여하기

1. 이 저장소를 Fork 합니다
2. 새로운 기능 브랜치를 생성합니다 (`git checkout -b feature/새기능`)
3. 변경사항을 커밋합니다 (`git commit -am '새 기능 추가'`)
4. 브랜치에 Push 합니다 (`git push origin feature/새기능`)
5. Pull Request를 생성합니다

## 📄 라이선스

이 프로젝트는 MIT 라이선스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 📞 연락처

프로젝트에 대한 질문이나 제안사항이 있으시면 이슈를 등록해 주세요.

---

⭐ 이 프로젝트가 도움이 되었다면 별표를 눌러주세요!
