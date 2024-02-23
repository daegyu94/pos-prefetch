import struct
import json
import timeit

# 테스트 데이터 생성
data = [1, 2, 3, 4, 5, 6]

# struct.pack 성능 측정
def pack_performance():
    packed_data = struct.pack('6i', *data)

# json.dumps 성능 측정
def json_performance():
    json_data = json.dumps(data)

# 측정 시작
struct_time = timeit.timeit(pack_performance, number=1000000)
json_time = timeit.timeit(json_performance, number=1000000)

# 결과 출력
print(f'struct.pack 소요 시간: {struct_time:.6f} 초')
print(f'json.dumps 소요 시간: {json_time:.6f} 초')
