#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_BACKTRACE_MAX_FRAMES 32

/*
 * Print a frame-pointer based stack backtrace to the console.
 *
 * skip = 0 -> frame #0 is the immediate caller of debug_backtrace().
 * skip = 1 -> frame #0 is that caller's caller.
 *
 * Wrapper functions that always call debug_backtrace() from the
 * same spot (assert_fail(), panic(), ...) should pass skip = 1
 * so the backtrace starts at *their* caller.
 *
 * Requires frame pointers (no -fomit-frame-pointer). Addresses
 * are raw return addresses — resolve externally against the
 * unstripped ELF:
 *
 *   addr2line -e iso/boot/kernel.elf -f -C 0xADDRESS
 */
void debug_backtrace(int skip, int max_frames);

#endif
