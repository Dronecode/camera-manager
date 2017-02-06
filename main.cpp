/*
 * This file is part of the Camera Streaming Daemon project
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>

#include "log.h"

static void help(FILE *fp)
{
    fprintf(fp, "%s [OPTIONS...]\n\n"
                "  -h --help                    Print this message\n",
            program_invocation_short_name);
}

static int parse_argv(int argc, char *argv[])
{
    static const struct option options[] = {{}};
    int c;

    assert(argc >= 0);
    assert(argv);

    while ((c = getopt_long(argc, argv, "h", options, NULL)) >= 0) {
        switch (c) {
        case 'h':
            help(stdout);
            return 0;
        case '?':
        default:
            help(stderr);
            return -EINVAL;
        }
    }

    /* positional arguments */
    if (optind != argc) {
        log_error("Invalid argument");
        help(stderr);
        return -EINVAL;
    }

    return 2;
}

int main(int argc, char *argv[])
{
    log_open();

    if (parse_argv(argc, argv) != 2)
        goto close_log;

    log_debug("Starting Camera Streaming Daemon");

    log_close();

    return 0;

close_log:
    log_close();
    return EXIT_FAILURE;
}
