import threading
import time

def my_function(stop_event):
    while not stop_event.is_set():
        # 블로킹 함수 호출 또는 다른 작업 수행
        time.sleep(10)
        pass

# 종료 이벤트 객체 생성
stop_event = threading.Event()

# 스레드 생성
my_thread = threading.Thread(target=my_function, args=(stop_event,))

# 스레드 시작
my_thread.start()

# 종료 이벤트 설정하여 스레드 종료 신호 전달
stop_event.set()

# 스레드가 종료될 때까지 대기
while my_thread.is_alive():
    # 일정 시간 대기
    time.sleep(0.1)

print("스레드 종료")
