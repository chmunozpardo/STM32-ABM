python
import sys
sys.path.insert(0, 'C:/Program Files/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/share/gcc-8.1.0/python')
sys.path.append("C:/Program Files/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/share/gcc-8.1.0/python/libstdcxx/v6")
from libstdcxx.v6.printers import register_libstdcxx_printers
register_libstdcxx_printers (None)
print("hello world")
end