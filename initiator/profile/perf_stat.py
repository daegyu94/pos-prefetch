import pstats

filename='translator.data'

p = pstats.Stats(filename)
p.strip_dirs()
p.sort_stats('cumulative')
p.print_stats()
