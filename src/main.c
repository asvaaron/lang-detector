#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "lang_scan.h"
#include "os_info.h"

static void print_usage(const char *prog_name) {
    printf("Usage: %s [-f|--format table|json|csv] [--allow-root] [-h|--help]\n", prog_name);
    printf("\n");
    printf("  -f, --format <fmt>  Output format: table (default), json, or csv\n");
    printf("  --allow-root        Allow running as root (unsafe: executes binaries found on $PATH)\n");
    printf("  -h, --help          Show this help message\n");
}

static int parse_format(const char *value, LangFormat *out) {
    if (strcmp(value, "table") == 0) {
        *out = LANG_FORMAT_TABLE;
        return 1;
    }
    if (strcmp(value, "json") == 0) {
        *out = LANG_FORMAT_JSON;
        return 1;
    }
    if (strcmp(value, "csv") == 0) {
        *out = LANG_FORMAT_CSV;
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    LangFormat format = LANG_FORMAT_TABLE;
    int allow_root = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--allow-root") == 0) {
            allow_root = 1;
        }
    }

    if (geteuid() == 0 && !allow_root) {
        fprintf(stderr, "error: refusing to run as root (this tool executes binaries found on $PATH)\n");
        fprintf(stderr, "       pass --allow-root to override\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--allow-root") == 0) {
            continue;
        }

        if (strcmp(arg, "-f") == 0 || strcmp(arg, "--format") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: %s requires an argument\n", arg);
                print_usage(argv[0]);
                return 1;
            }
            const char *value = argv[++i];
            if (!parse_format(value, &format)) {
                fprintf(stderr, "error: unknown format '%s' (expected table, json, or csv)\n", value);
                return 1;
            }
            continue;
        }

        if (strncmp(arg, "--format=", 9) == 0) {
            if (!parse_format(arg + 9, &format)) {
                fprintf(stderr, "error: unknown format '%s' (expected table, json, or csv)\n", arg + 9);
                return 1;
            }
            continue;
        }

        fprintf(stderr, "error: unrecognized argument '%s'\n", arg);
        print_usage(argv[0]);
        return 1;
    }

    print_os_info();
    scan_languages(format);
    return 0;
}
