/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2026, ASD Project Contributors
 *
 * kill — send a signal to a process.
 *
 * Usage:
 *   kill [-s <sig> | -<sig>] <pid>...
 *   kill -l
 */
#include "common.h"

static int parse_int(const char *s) {
    int n = 0, neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
    return neg ? -n : n;
}

static int is_digit_str(const char *s) {
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

typedef struct {
    const char *name;
    int         num;
} sig_map_t;

static const sig_map_t g_sigs[] = {
    { "HUP",   SIGHUP },
    { "INT",   SIGINT },
    { "QUIT",  SIGQUIT },
    { "KILL",  SIGKILL },
    { "TERM",  SIGTERM },
    { "CHLD",  SIGCHLD },
    { "CONT",  SIGCONT },
    { "STOP",  SIGSTOP },
    { NULL,    0 }
};

static int parse_signal(const char *str) {
    if (!str || !*str) return -1;
    if (is_digit_str(str)) {
        int val = parse_int(str);
        if (val >= 0 && val <= 31) return val;
        return -1;
    }
    const char *name = str;
    if (strncmp(name, "SIG", 3) == 0) name += 3;
    for (int i = 0; g_sigs[i].name; i++) {
        if (strcmp(name, g_sigs[i].name) == 0) return g_sigs[i].num;
    }
    return -1;
}

static void print_signals(void) {
    puts(" 1) SIGHUP   2) SIGINT   3) SIGQUIT  9) SIGKILL");
    puts("15) SIGTERM 17) SIGCHLD 18) SIGCONT 19) SIGSTOP");
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        puts("usage: kill [-s <sig> | -<sig>] <pid>...");
        puts("       kill -l");
        return 1;
    }

    if (strcmp(argv[1], "-l") == 0) {
        print_signals();
        return 0;
    }

    int sig = SIGTERM;
    int pid_start = 1;

    if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3) {
            puts("kill: option requires an argument -- 's'");
            return 1;
        }
        sig = parse_signal(argv[2]);
        if (sig < 0) {
            printf("kill: unknown signal %s\n", argv[2]);
            return 1;
        }
        pid_start = 3;
    } else if (argv[1][0] == '-' && argv[1][1] != '\0') {
        sig = parse_signal(argv[1] + 1);
        if (sig < 0) {
            printf("kill: unknown signal %s\n", argv[1] + 1);
            return 1;
        }
        pid_start = 2;
    }

    if (pid_start >= argc) {
        puts("kill: missing pid");
        return 1;
    }

    int errors = 0;
    for (int i = pid_start; i < argc; i++) {
        int pid = parse_int(argv[i]);
        if (pid <= 0) {
            printf("kill: invalid pid: %s\n", argv[i]);
            errors++;
            continue;
        }

        int r = asd_kill(pid, sig);
        if (r < 0) {
            if (r == -3) {
                printf("kill: (%d) - No such process\n", pid);
            } else if (r == -1) {
                printf("kill: (%d) - Operation not permitted\n", pid);
            } else if (r == -22) {
                printf("kill: invalid signal %d\n", sig);
            } else {
                printf("kill: (%d) - error %d\n", pid, r);
            }
            errors++;
        }
    }

    return errors ? 1 : 0;
}
