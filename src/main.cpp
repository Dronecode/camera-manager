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
#include <string.h>
#include <sys/stat.h>

#include "conf_file.h"
#include "glib_mainloop.h"
#include "log.h"
#include "settings.h"
#include "stream_manager.h"

#define DEFAULT_CONFFILE "/etc/camera-streaming-daemon/main.conf"
#define DEFAULT_CONF_DIR "/etc/camera-streaming-daemon/config.d"

struct options {
    const char *filename;
    const char *conf_dir;
};

static void help(FILE *fp)
{
    fprintf(fp,
            "%s [OPTIONS...]\n\n"
            "  -c --conf-file                .conf file with configurations for "
            "camera-streaming-daemon.\n"
            "  -d --conf-dir <dir>          Directory where to look for .conf files overriding\n"
            "                               default conf file.\n"
            "  -h --help                    Print this message\n",
            program_invocation_short_name);
}

static const char *get_default_file_name()
{
    char *s;

    s = getenv("CSD_CONF_FILE");
    if (s)
        return s;

    return DEFAULT_CONFFILE;
}

static const char *get_default_conf_dir()
{
    char *s;

    s = getenv("CSD_CONF_DIR");
    if (s)
        return s;

    return DEFAULT_CONF_DIR;
}

static int cmpstr(const void *s1, const void *s2)
{
    return strcmp(*(const char **)s1, *(const char **)s2);
}

static int parse_conf_files(ConfFile &conf, struct options *opt)
{
    DIR *dir;
    struct dirent *ent;
    int ret = 0;
    char *files[128] = {};
    int i = 0, j = 0;

    // First, open default conf file
    ret = conf.parse(opt->filename);

    // If there's no default conf file, everything is good
    if (ret < 0 && ret != -ENOENT) {
        return ret;
    }

    // Then, parse all files on configuration directory
    dir = opendir(opt->conf_dir);
    if (!dir)
        return 0;

    while ((ent = readdir(dir))) {
        char path[PATH_MAX];
        struct stat st;

        ret = snprintf(path, sizeof(path), "%s/%s", opt->conf_dir, ent->d_name);
        if (ret >= (int)sizeof(path)) {
            log_error("Couldn't open directory %s", opt->conf_dir);
            ret = -EINVAL;
            goto fail;
        }
        if (stat(path, &st) < 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        files[i] = strdup(path);
        if (!files[i]) {
            ret = -ENOMEM;
            goto fail;
        }
        i++;

        if ((size_t)i > sizeof(files) / sizeof(*files)) {
            log_warning("Too many files on %s. Not all of them will be considered", opt->conf_dir);
            break;
        }
    }

    qsort(files, (size_t)i, sizeof(char *), cmpstr);

    for (j = 0; j < i; j++) {
        ret = conf.parse(files[j]);
        if (ret < 0)
            goto fail;
        free(files[j]);
    }

    closedir(dir);

    return 0;
fail:
    while (j < i) {
        free(files[j++]);
    }

    closedir(dir);

    return ret;
}

static int parse_argv(int argc, char *argv[], struct options *opt)
{
    static const struct option options[] = {
        {"conf-file", required_argument, NULL, 'c'}, {"conf-dir", required_argument, NULL, 'd'},
    };
    int c;

    assert(argc >= 0);
    assert(argv);

    while ((c = getopt_long(argc, argv, "hd:c:d:", options, NULL)) >= 0) {
        switch (c) {
        case 'h':
            help(stdout);
            return 0;
        case 'c': {
            opt->filename = optarg;
            break;
        }
        case 'd': {
            opt->conf_dir = optarg;
            break;
        }
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

    if (!opt->filename)
        opt->filename = get_default_file_name();

    if (!opt->conf_dir)
        opt->conf_dir = get_default_conf_dir();
    return 2;
}

int main(int argc, char *argv[])
{
    struct options opt = {0};
    log_open();

    ConfFile *conf;
    GlibMainloop mainloop;
    StreamManager stream;

    if (parse_argv(argc, argv, &opt) != 2)
        goto close_log;

    conf = new ConfFile();
    parse_conf_files(*conf, &opt);
    stream.init_streams(*conf);
    delete conf;

    log_debug("Starting Camera Streaming Daemon");
    stream.start();
    mainloop.loop();

    log_close();

    return 0;

close_log:
    log_close();
    return EXIT_FAILURE;
}
