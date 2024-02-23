import struct

class ReadPagesData:
    def __init__(self, dev_id, file_size, ino, indexes, is_readaheads, size):
        self.dev_id = dev_id
        self.file_size = file_size
        self.ino = ino
        self.indexes = indexes
        self.is_readaheads = is_readaheads
        self.size = size

    def pack(self):
        # 패킹 포맷 문자열 생성
        format_string = 'I Q Q ' + ' '.join(['Q'] * 32) + ' ' + 'B' * 32 + ' I'
        
        # 데이터 패킹
        packed_data = struct.pack(format_string, self.dev_id, self.file_size, self.ino,
                                  *self.indexes, *self.is_readaheads, self.size)
        
        return packed_data

# 테스트 데이터 생성
data = ReadPagesData(
    dev_id=1,
    file_size=1024,
    ino=5678,
    indexes=[10, 20, 30] + [0] * 29,  # 예시로 처음 세 개의 값을 주었습니다.
    is_readaheads=[1, 0, 1] + [0] * 29,  # 예시로 처음 세 개의 값을 주었습니다.
    size=4096
)

# 데이터 패킹
packed_data = data.pack()

# 결과 출력
print(f'Packed Data: {packed_data}')
