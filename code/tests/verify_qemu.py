#!/usr/bin/env python3
"""Build, boot, grade, and debug all Lab2 modes; restore default Best-Fit.

Run on Ubuntu: python3 tests/verify_qemu.py
Only processes started by this script are stopped. Existing QEMU is untouched.
"""
from pathlib import Path
import os
import signal
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
LOGS = ROOT.parent / 'report' / 'logs'
LOGS.mkdir(parents=True, exist_ok=True)

def command(argv, name, timeout=90):
    proc = subprocess.Popen(argv, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, start_new_session=True)
    try:
        output, _ = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGTERM)
        output, _ = proc.communicate(timeout=5)
        (LOGS / name).write_bytes(output)
        raise RuntimeError(f'{argv}: exceeded {timeout}s; see {name}')
    finally:
        try:
            os.killpg(proc.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
    text = output.decode('utf-8', 'replace')
    (LOGS / name).write_text(text)
    if proc.returncode:
        print(text)
        raise RuntimeError(f'{argv}: exit {proc.returncode}; see {name}')
    return text

def build(mode, definitions=''):
    command(['make', 'clean'], f'{mode}-clean.txt')
    command(['make', '-j2', f'PMM_DEFS={definitions}'], f'{mode}-build.txt')

def boot(mode, markers):
    argv = ['qemu-system-riscv64', '-machine', 'virt', '-nographic',
            '-kernel', 'bin/ucore.img', '-monitor', 'none']
    proc = subprocess.Popen(argv, cwd=ROOT, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, start_new_session=True)
    try:
        output, _ = proc.communicate(timeout=4)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGTERM)
        output, _ = proc.communicate(timeout=5)
    text = output.decode('utf-8', 'replace')
    (LOGS / f'{mode}.txt').write_text(text)
    assert 'kernel panic at' not in text, f'{mode}: kernel panic'
    assert all(marker in text for marker in markers), f'{mode}: missing success marker'
    print(f'PASS: {mode}')

def debug():
    qemu = subprocess.Popen(['qemu-system-riscv64', '-machine', 'virt', '-nographic',
        '-kernel', 'bin/ucore.img', '-S', '-gdb', 'tcp:127.0.0.1:12345', '-monitor', 'none'],
        cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        start_new_session=True)
    try:
        time.sleep(.5)
        if qemu.poll() is not None:
            raise RuntimeError('debug QEMU failed to start; port 12345 may be occupied')
        commands = ['set pagination off', 'file bin/kernel', 'set arch riscv:rv64',
            'target remote localhost:12345', 'info registers pc', 'x/6i $pc',
            'hbreak kern_init', 'continue', 'info registers pc sp satp',
            'break check_alloc_page', 'continue', 'p/x va_pa_offset',
            'p/x npage', 'p/x pages', 'p/x pages[0].flags', 'detach']
        args = ['gdb-multiarch', '-batch', '-nx']
        for entry in commands:
            args += ['-ex', entry]
        text = command(args, 'gdb.txt', 20)
        assert '0xffffffff40000000' in text and '0x88000' in text
        assert 'Breakpoint 1, kern_init' in text and 'Breakpoint 2, check_alloc_page' in text
        print('PASS: GDB / Sv39 / page descriptors')
    finally:
        if qemu.poll() is None:
            os.killpg(qemu.pid, signal.SIGTERM)
        qemu.wait(timeout=5)

def main():
    print('Lab2 validation: default 128 MiB QEMU virt, single CPU', flush=True)
    env = []
    for argv in [['riscv64-unknown-elf-gcc', '--version'],
                 ['qemu-system-riscv64', '--version'], ['gdb-multiarch', '--version']]:
        env.append(subprocess.check_output(argv, text=True))
    (LOGS / 'environment.txt').write_text('\n'.join(env))
    build('best-fit')
    boot('best-fit', ['best_fit_pmm_manager', 'check_alloc_page() succeeded!'])
    text = command(['make', 'grade'], 'grade.txt')
    assert 'Total Score: 20/20' in text
    print('PASS: official grade 20/20', flush=True)
    build('buddy', '-DUSE_BUDDY')
    boot('buddy', ['buddy_pmm_manager', 'buddy_check() succeeded!'])
    build('slub', '-DRUN_SLUB')
    boot('slub', ['best_fit_pmm_manager', 'slub_check() succeeded!'])
    text = command(['python3', 'tests/run_host_tests.py'], 'host-tests.txt')
    assert 'PASS:' in text
    print('PASS: native allocator stress tests', flush=True)
    build('final-default')
    debug()
    print('ALL PASS. Default Best-Fit build restored. Logs:', LOGS, flush=True)

if __name__ == '__main__':
    main()
