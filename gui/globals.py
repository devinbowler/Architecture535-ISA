import ctypes, pathlib

_lib = ctypes.CDLL(
    str(pathlib.Path(__file__).with_name("libsimconf.so"))
)

USER_DRAM_DELAY  = ctypes.c_uint16.in_dll(_lib, "USER_DRAM_DELAY")
USER_CACHE_DELAY = ctypes.c_uint16.in_dll(_lib, "USER_CACHE_DELAY")

CACHE_ENABLED    = ctypes.c_bool  .in_dll(_lib, "CACHE_ENABLED")
PIPELINE_ENABLED = ctypes.c_bool  .in_dll(_lib, "PIPELINE_ENABLED")

BREAKPOINT_PC    = ctypes.c_int16 .in_dll(_lib, "BREAKPOINT_PC")

