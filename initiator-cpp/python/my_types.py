# enum style
class EventType:
    EVENT_OPEN_ENTRY        = 0
    EVENT_OPEN_END          = 1
    EVENT_CLOSE             = 2
    EVENT_PAGE_DELETION     = 3
    EVENT_PAGE_REFERENCED   = 4
    EVENT_VFS_UNLINK        = 5
    EVENT_CLEANCACHE_REPL   = 6

class FilePathType:
    FPATH_ABSOLUTE = 0
    FPATH_ABSOLUTE_LONG = 1
    FPATH_RELATIVE = 2

class OpenCloseEvent:
    def __init__(self, event_type, dev_id):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino

class PageAccessEvent:
    def __init__(self, event_type, dev_id, ino, index, file_size):
        self.event_type = event_type
        self.dev_id = dev_id
        self.ino = ino
        self.index = index
        self.file_size = file_size

    def __repr__(self):
        return "(PageAccessEvent: type={}, dev_id={}, ino={}, index={}, file_size={})" \
                .format(self.event_type, self.dev_id, self.ino, 
                self.index, self.file_size)

# TODO: readaheads bitmap, indexes -> index + length
class Ext4ReadPagesEvent:
    def __init__(self, dev_id, ino, indexes, is_readaheads, size, file_size):
        self.dev_id = dev_id
        self.ino = ino
        self.indexes = indexes
        self.is_readaheads = is_readaheads
        self.size = size
        self.file_size = file_size
    
    def __repr__(self):
        return "(ReadPagesEvent: {}, {}, {}, {}, {}, {})" \
                .format(self.dev_id, self.ino, self.indexes, 
                self.is_readaheads, self.size, self.file_size)
