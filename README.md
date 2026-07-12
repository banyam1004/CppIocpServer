## 프로젝트 소개
### C++ Winsock기반 IOCP 비동기 게임 서버

## 개발 배경 및 목적
### CppGameServer의 한계(1클라이언트 1스레드)를 극복하기 위해 시작

## 기술 스택
### C++ / Winsock2 / IOCP / MySQL / std::thread / std::mutex

## 주요 기능
- 비동기 소켓 통신
- 워커 스레드 4개로 다중 클라이언트 처리
- 패킷 구조화 (헤더 + 바디)
- 방 시스템 (생성/입장/목록/나가기)
- 로그인/회원가입 (MySQL)
- 캐릭터 정보 저장/불러오기
- 로그 시스템
- 부하 테스트 100명 동시접속 완료

## 전체 흐름
1. 서버 시작 -> IOCP 완료 포트 생성
2. 워커 스레드 4개 생성 -> 완료 포트 대기
3. 클라이언트 접속 -> IOCP에 소켓 등록 -> WSARecv() 등록
4. 데이터 도착 -> 완료 포트 알림 -> 워커 스레드 처리
5. 패킷 파싱 -> 타입별 처리 -> 다시 WSARecv() 등록

## 스레드 구조
- 메인 스레드
  > accept()로 클라이언트 접속 대기
  > 접속하면 IOCP에 소켓 등록
- 워커 스레드(4개)
  > GetQueuedCompletionStatus()로 완료 포트 대기
  > 데이터 도착하면 깨어나서 처리
  > 처리 후 다시 대기
 
## 패킷 구조
[헤더 4byte][바디]
- 헤더
  > PacketType (2byte) : CHAT / CREATE / JOIN / LEAVE / LIST / NOTIFY
  > size (2byte) : 헤더 포함 전체 크기
- 바디
  > 실제 데이터 (채팅 내용, 방 이름 등) 

## DB 구조
- users 테이블
  > id (PK, AUTO_INCREMENT)
  > username (유저 이름)
  > password (비밀번호)
  > created_at (가입 일시)

- characters 테이블
  > id (PK, AUTO_INCREMENT)
  > user_id (FK -> users.id)
  > name (캐릭터 이름)
  > level (레벨)
  > hp (체력)
  > exp (경험치)
  > created_at (생성 일시)

## 트러블 슈팅
- 교착 상태
  > 방 나가기 처리 중 mutex를 잡은 상태에서 deleteRoom() 호출 시 내부에서 같은 mutex 재획득 시도
  > 해결: deleteRoom() 호출 대신 같은 범위 안에서 직접 제거
- WSARecv() 위치 오류
  > switch 문 안에 WSARecv() 배치로 특정 패킷만 다음 수신 등록됨
  > 해결: cout을 활용하여 문제점 확인 후 switch문 밖으로 이동
- MySQL 동시 접근 문제
  > 부하 테스트 중 다수 스레드가 동시에 DB 접근 시 연결 끊김
  > 해결: dbMutex로 DB접근 직렬화 + 재연결 로직 추가

## 한계점
- dbConn 하나로 모든 요청 처리
  > 동시 접속자 많아지면 DB 병목 발생
- DB에 비밀번호를 평문 그대로 저장
- 패킷 암호화 없이 전송
- 서버 하나가 모든 클라이언트 처리
- 게임 로직 없음

## 개선방향
- MySQL 커넥션 풀 도입으로 DB 병목 해소
- Unity 클라이언트 연동으로 실제 게임 구현
- UDP 통신 추가로 실시간 액션 게임 대응
- 패킷 자체 XOR 암호화 적용 차후 TLS/SSL 적용으로 패킷 암호화 
- 로그인 비밀번호 암호화 후 저장, 로그인 시 복호화 후 비밀번호 비교(bcrypt/SHA256)
