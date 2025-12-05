# 최소 CMake 버전 설정
cmake_minimum_required(VERSION 3.10)

# 컴파일러 지정 (project() 호출 전에 설정)
# 원하는 컴파일러 경로로 수정하세요.
# Clang/LLVM
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
# GCC
#set(CMAKE_C_COMPILER "/usr/bin/gcc")
#set(CMAKE_CXX_COMPILER "/usr/bin/g++")


# 프로젝트 이름 및 버전 설정
project(test VERSION 1.0)
# 주/부버전 설정
#SET ( PROJECT_VERSION_MAJOR <주_버전> )
#SET ( PROJECT_VERSION_MINOR <부_버전> )

# 프로젝트 파일명 지정 "프로젝트이름-주버전.부버전.out"
#SET ( OUTPUT_ELF "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.out" )

# 빌드 형상(Configuration)
#SET ( CMAKE_BUILD_TYPE <Debug|Release> )

# Makefile 생성 여부
#SET ( CMAKE_VERBOSE_MAKEFILE <true|false> )

# C 표준 설정
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# C++ 표준 설정
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 필요 라이브러리 찾기
#find_package(<라이브러리_이름> CONFIG REQUIRED)

# 공통 링크 라이브러리 (-l)
#LINK_LIBRARIES( <라이브러리> <라이브러리> ... )

# 공통 링크 라이브러리 디렉토리 (-L)
#LINK_DIRECTORIES ( <디렉토리> <디렉토리> ... )

# 소스 파일 목록 설정 (예: main.cpp)
set(SOURCES
    test.cpp
    # 추가 소스 파일이 있으면 여기에 나열
)

# 실행 파일 생성
add_executable(${PROJECT_NAME} ${SOURCES})

# 컴파일 옵션 추가 (PRIVATE 옵션은 해당 타겟에만 적용)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Werror
)

SET ( CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -fsanitize=address" )
SET ( CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer -fsanitize=address" )

SET ( CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -march=native -flto" )
SET ( CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -march=native -flto" )
SET ( CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,--as-needed -Wl,-O2 -Wl,-z,relro -Wl,-z,now -Wl,--strip-all" )
