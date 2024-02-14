import time

def monitor_file(file_path):
    while True:
        try:
            with open(file_path, 'r') as file:
                content = file.read().strip()
                if content:
                    print(f"File content: {content}")
        except FileNotFoundError:
            print(f"File not found: {file_path}")

        time.sleep(0.1)  # 100ms 대기

if __name__ == "__main__":
    file_path = "file.txt"
    monitor_file(file_path)
