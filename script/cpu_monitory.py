import psutil
import time
import subprocess
import signal
import sys

def get_pid(process_name):
    """ 특정 프로세스의 PID를 얻음. 없으면 None 반환 """
    try:
        pid = subprocess.check_output(["pidof", process_name]).strip().split()[0]
        return int(pid)
    except subprocess.CalledProcessError:
        return None

def monitor_cpu_usage(pid):
    """ 특정 PID의 CPU 사용량을 모니터링하고 평균을 계산 """
    proc = psutil.Process(pid)
    cpu_usages = []

    def signal_handler(sig, frame):
        """ Ctrl+C 시 평균 CPU 사용량 출력 후 종료 """
        if cpu_usages:
            avg_cpu = sum(cpu_usages) / len(cpu_usages)
            sum_cpu= sum(cpu_usages)
            print(f"\nCPU Usage: Avg={avg_cpu:.2f}, Sum={sum_cpu: .2f}")
        print("Exiting...")
        sys.exit(0)
    
    signal.signal(signal.SIGINT, signal_handler)
    
    sec = 0
    while True:
        cpu_usage = proc.cpu_percent(interval=1)
        cpu_usages.append(cpu_usage)
        print(f"{sec}\t{cpu_usage}")
        sec += 1
        #time.sleep(1)

if __name__ == "__main__":
    process_name = "pos_prefetch.out"
    print(f"Waiting for process '{process_name}' to start...")
    
    while True:
        pid = get_pid(process_name)
        if pid:
            print(f"Process '{process_name}' started with PID: {pid}")
            monitor_cpu_usage(pid)
        time.sleep(1)
