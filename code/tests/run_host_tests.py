"""Compile the real allocator modules for the host and exercise them via ctypes.

This does NOT emulate DTB, Sv39, SBI, or kernel boot. Run make grade in QEMU too.
Usage: python3 tests/run_host_tests.py [--cc /path/to/clang]
"""
import argparse
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--cc', default=shutil.which('clang') or shutil.which('gcc'))
args = parser.parse_args()
if not args.cc:
    parser.error('a native clang or gcc is required')
root = Path(__file__).resolve().parents[1]
sources = ['tests/host_allocators.c', 'kern/mm/best_fit_pmm.c',
           'kern/mm/buddy_pmm.c', 'kern/mm/slub.c', 'libs/string.c']
with tempfile.TemporaryDirectory(prefix='lab2-host-') as tmp:
    win = os.name == 'nt'
    output = Path(tmp) / ('tests.dll' if win else 'tests.so')
    flags = ['-shared', '-nostdlib', '-nostdinc', '-fno-builtin', '-std=gnu99', '-O2']
    if win:
        flags += ['--target=x86_64-pc-windows-msvc', '-fuse-ld=lld',
                  '-Wno-macro-redefined', '-Wl,/noentry']
    else:
        flags += ['-fPIC']
    for inc in ['libs', 'kern/mm', 'kern/debug']:
        flags += ['-I', str(root / inc)]
    subprocess.run([args.cc, *flags, *(str(root / x) for x in sources),
                    '-o', str(output)], check=True)
    library = ctypes.CDLL(str(output))
    result = library.run_tests()
    if win:
        import _ctypes
        _ctypes.FreeLibrary(library._handle)
    assert result == 0
    print('PASS: unchanged Best-Fit checks, Buddy checks, SLUB checks on both managers,')
    print('40,000 deterministic mixed allocation/free operations with overlap/accounting checks,')
    print('and Buddy full exhaustion/recovery/coalescing of 2,048 pages.')
