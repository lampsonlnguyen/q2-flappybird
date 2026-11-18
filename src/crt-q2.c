/* SPDX-License-Identifier: GPL-3.0-or-later
 * MIPS glibc startup based on Diamond Bond's Q2 Groovebox crt-q2.c
 * (GPL-2.0-or-later), derived from Rockbox.
 */
#include <stdlib.h>
#include <sys/stat.h>
__asm__(".set noreorder\n.globl __start\n__start:\n"
        "lui $28, %hi(_gp)\naddiu $28, $28, %lo(_gp)\n"
        "move $31, $0\nlw $5, 0($29)\naddiu $6, $29, 4\n"
        "li $8, -8\nand $29, $29, $8\naddiu $29, $29, -32\n"
        "lui $4, %hi(main)\naddiu $4, $4, %lo(main)\nmove $7, $0\n"
        "sw $0, 16($29)\nsw $2, 20($29)\nsw $29, 24($29)\n"
        "lw $25, %call16(__libc_start_main)($28)\njalr $25\nnop\n"
        "1: b 1b\nnop\n.set reorder\n");
const int _IO_stdin_used = 0x20001;
void *__dso_handle = 0;
int __cxa_atexit(void (*)(void *), void *, void *);
int atexit(void (*fn)(void)) {
    return __cxa_atexit((void (*)(void *))fn, 0, __dso_handle);
}
int __xstat(int, const char *, struct stat *);
int __fxstat(int, int, struct stat *);
int __lxstat(int, const char *, struct stat *);
int __fxstatat(int, int, const char *, struct stat *, int);
int __xmknod(int, const char *, mode_t, dev_t *);
int stat(const char *p, struct stat *s) { return __xstat(_STAT_VER, p, s); }
int fstat(int fd, struct stat *s) { return __fxstat(_STAT_VER, fd, s); }
int lstat(const char *p, struct stat *s) { return __lxstat(_STAT_VER, p, s); }
int fstatat(int fd, const char *p, struct stat *s, int flags) {
    return __fxstatat(_STAT_VER, fd, p, s, flags);
}
int mknod(const char *p, mode_t m, dev_t d) { return __xmknod(_MKNOD_VER, p, m, &d); }
