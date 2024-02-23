import struct

class BPFEvent:
    def __init__(self, event_type, dev_id, ino, file_size, indexes, is_readaheads, size):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino
        self.file_size = file_size
        self.indexes = indexes
        self.is_readaheads = is_readaheads
        self.size = size

    def pack(self):
        # 패킹 포맷 문자열 생성
        format_string = 'B I Q Q ' + ' '.join(['Q'] * 32) + ' ' + 'B' * 32 + ' I'

        # 데이터 패킹
        packed_data = struct.pack(format_string,
                                  self.event_type,
                                  self.dev_id,
                                  self.ino,
                                  self.file_size,
                                  *self.indexes,
                                  *self.is_readaheads,
                                  self.size)

        return packed_data

# 예시 데이터 생성
my_data = BPFEvent(
    event_type=1,
    dev_id=123,
    ino=456,
    file_size=1024,
    indexes=[10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 210, 220, 230, 240, 250, 260, 270, 280, 290, 300, 310, 320],
    is_readaheads=[1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0],
    size=12345
)

# 패킹된 데이터 출력
print(repr(my_data.pack()))
