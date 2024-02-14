import threading

lock_enable = False

class DummyLock:
    def acquire(self):
        pass

    def release(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        pass
