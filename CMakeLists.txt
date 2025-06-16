cmake_minimum_required(VERSION 3.12)

project(SnakeGame VERSION 1.0)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 컴파일러 옵션
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# ncurses 라이브러리 찾기
find_package(Curses REQUIRED)

# 소스 파일들 자동 수집
file(GLOB SOURCES "src/*.cpp")

# 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCES})

# 헤더 파일 경로 추가
target_include_directories(${PROJECT_NAME} PRIVATE src)

# 라이브러리 링크
target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})

# 설치 설정
install(TARGETS ${PROJECT_NAME} DESTINATION bin)