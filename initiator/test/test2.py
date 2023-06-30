import asyncio
import time

class PathFinder:
    def __init__(self, mnt_path, ino):
        self.mnt_path = mnt_path
        self.ino = ino

    async def slow_path(self):
        cmd = f"find {self.mnt_path} -xdev -inum {self.ino}"
        
        # 비동기 프로세스 실행
        process = await asyncio.create_subprocess_shell(
            cmd, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE
        )
        
        # 프로세스 결과 기다리기
        stdout, _ = await process.communicate()
        output = stdout.decode().strip()
        if output:
            return output.split()[0]

async def main():
    mnt_path = "/mnt/nvme0"
    ino = 12
    path_finder = PathFinder(mnt_path, ino)
    
    result = await path_finder.slow_path()
    print(result)

# asyncio 이벤트 루프 생성 및 실행
loop = asyncio.get_event_loop()
#loop.run_until_complete(main())
asyncio.ensure_future(main())

loop.run_forever()

time.sleep(3)
