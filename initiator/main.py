import argparse
from collections import defaultdict
import signal
import sys
import cProfile
import queue 

# arguments
examples = """examples:
    ./main.py -a 127.0.0.1 -p 50051 # tgt_addr, tgt_port
    ./main.py -t True              # check function elapsed time (--timer True)
    ./main.py -d               # debug mode (--debug)
"""
parser = argparse.ArgumentParser(
    description="Initiator caching helper",
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog=examples)
parser.add_argument('-a', '--addr', default='10.0.0.56',
    help="target ip addr")
parser.add_argument('-p', '--port', default=50051,
    help="target port")
parser.add_argument('-t', '--timer', default=False,
    help="timer")
parser.add_argument('-d', "--debug", action="store_true",
    help="debug program")
parser.add_argument('-s', "--steering", default=False,
    help="steering pages within extent size distance")

args = parser.parse_args()

from timer import *
from stats import *
from bpf_tracer import *
from grpc_handler import *
from user_handler import *
from procfs import *

timer_stat._timer_on = args.timer

meta_dict = Metadata()
fpath_dict = defaultdict(lambda: (0, "")) # (path_type, str)

translator = Translator(meta_dict, fpath_dict, int(args.steering))
grpc_handler = gRPCHandler(args.addr, args.port)

event_queue = queue.Queue(maxsize = 100 * 1000)
user_handler = UserHandler(event_queue, meta_dict, translator, grpc_handler, 
        int(args.steering))
user_handler.daemon = True
bpf_tracer = BPFTracer(event_queue, meta_dict, fpath_dict)
bpf_tracer.daemon = True

stats_monitor = threading.Thread(target=run_stats_monitor, 
        args=(translator, ), name="StatsMonitor")
stats_monitor.daemon = True

procfs_monitor = threading.Thread(target=run_procfs_monitor, 
        args=(event_queue, ), name="ProcfsMonitor")
procfs_monitor.daemon = True

def get_current_threads():
    thread_list = threading.enumerate()
    for thread in thread_list:
        print(thread.name)
    print("# of threads=",len(thread_list))

def init():
    print("init...")
    stats_monitor.start()
    procfs_monitor.start()
    """
    cProfile.run('translator.start()', filename='profile/translator.data')
    cProfile.run('grpc_handler.start()', filename='profile/grpc.data')
    cProfile.run('bpf_tracer.start()', filename='profile/bpf.data')
    """
    user_handler.start()
    bpf_tracer.start()

def fini():
    print("fini...")

def signal_handler(signal, frame):
    print('Ctrl+c is pressed..')
    fini()
    sys.exit(0)

def set_niceness():
    niceness = 0 # bigger is more nice to yield cpu to other process, default=0
    os.nice(niceness)
    new_niceness = os.nice(0)
    pid = os.getpid()
    print("pid={}, nice value={}" .format(pid, new_niceness))


if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    
    set_niceness() # make as bg thread to yield CPU for fg application
    
    init()
    
    time.sleep(0.5)

    get_current_threads()
        
    while True:
        time.sleep(1000000)
