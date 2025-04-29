# ───────────── gui/globals.py ─────────────
"""
Runtime knobs exported by the C code.

✓ Works on Windows (DLL) and Unix‑likes (SO)
✓ Raises a clear error if the library is missing
"""

import ctypes
import pathlib
import platform

_here   = pathlib.Path(__file__).parent          # …/gui
_is_win = platform.system().lower().startswith("win")

# correct filename for the running OS
_libname = "libsimconf.dll" if _is_win else "libsimconf.so"
_libpath = _here / _libname

# try to load
try:
    _lib = ctypes.CDLL(str(_libpath))
except OSError as e:
    raise OSError(
        f"[globals.py] Could not load {_libname!r} at {_libpath}\n"
        "→ Build the simulator first:  (cd simulator && make)"
    ) from e

# helper shortcuts
def _u16(sym):   return ctypes.c_uint16.in_dll(_lib, sym)
def _i16(sym):   return ctypes.c_int16 .in_dll(_lib, sym)
def _bool(sym):  return ctypes.c_int   .in_dll(_lib, sym)   # C `bool` = int

# exported variables (same names as before)
USER_DRAM_DELAY               = _u16 ("USER_DRAM_DELAY")
USER_CACHE_DELAY              = _u16 ("USER_CACHE_DELAY")
CACHE_ENABLED                 = _bool("CACHE_ENABLED")
CACHE_MODE                    = _u16("CACHE_MODE")
PIPELINE_ENABLED              = _bool("PIPELINE_ENABLED")
memory_operation_in_progress  = _bool("memory_operation_in_progress")
# ──────────────────────────────────────────

