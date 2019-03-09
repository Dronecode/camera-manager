/*
 * This file is part of the Dronecode Camera Manager
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
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "conf_file.h"
#include "glib_mainloop.h"
#include "log.h"
#include "util.h"
#include "CameraServer.h"

#include "Plugins/V4l2Camera/PluginV4l2.h"
static PluginV4l2 pluginV4l2;

#ifdef ENABLE_AERO
#include "Plugins/AeroAtomIspCamera/PluginAeroAtomIsp.h"
static PluginAeroAtomIsp pluginAeroAtomIsp;
#endif

#ifdef ENABLE_CUSTOM
#include "Plugins/CustomCamera/PluginCustom.h"
static PluginCustom pluginCustom;
#endif

#ifdef ENABLE_GAZEBO
#include "Plugins/GazeboCamera/PluginGazebo.h"
static PluginGazebo pluginGazebo;
#endif

#ifdef ENABLE_REALSENSE
#include "Plugins/RealSenseCamera/PluginRealSense.h"
static PluginRealSense pluginRealSense;
#endif

#define DEFAULT_CONFFILE "/etc/dcm/main.conf"
#define DEFAULT_CONF_DIR "/etc/dcm/config.d"

struct options {
    const char *filename;
    const char *conf_dir;
};

static void help(FILE *fp)
{
    fprintf(
        fp,
        "%s [OPTIONS...]\n\n"
        "  -c --conf-file                   .conf file with configurations for "
        "dronecode-camera-manager.\n"
        "  -d --conf-dir <dir>              Directory where to look for .conf files overriding\n"
        "                                   default conf file.\n"
        "  -g --debug-log-level <level>     Set debug log level. Levels are\n"
        "                                   <error|warning|info|debug>\n"
        "  -v --verbose                     Verbose. Same as --debug-log-level=debug\n"
        "  -h --help                        Print this message\n",
        program_invocation_short_name);
}

static const char *get_default_file_name()
{
    char *s;

    s = getenv("DCM_CONF_FILE");
    if (s)
        return s;

    return DEFAULT_CONFFILE;
}

static const char *get_default_conf_dir()
{
    char *s;

    s = getenv("DCM_CONF_DIR");
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

    if (opt->filename != nullptr) {
        // If filename was specified then use it and fail if it doesn't exist
        ret = conf.parse(opt->filename);
        if (ret < 0) {
            return ret;
        }
    } else {
        // Otherwise open default conf file
        ret = conf.parse(get_default_file_name());
        // If there's no default conf file, everything is good
        if (ret < 0 && ret != -ENOENT) {
            return ret;
        }
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

static int log_level_from_str(const char *str)
{
    if (strcaseeq(str, "error"))
        return (int)Log::Level::ERROR;
    if (strcaseeq(str, "warning"))
        return (int)Log::Level::WARNING;
    if (strcaseeq(str, "info"))
        return (int)Log::Level::INFO;
    if (strcaseeq(str, "debug"))
        return (int)Log::Level::DEBUG;

    return -EINVAL;
}

static int parse_argv(int argc, char *argv[], struct options *opt)
{
    static const struct option options[] = {{"conf-file", required_argument, NULL, 'c'},
                                            {"conf-dir", required_argument, NULL, 'd'},
                                            {"debug-log-level", required_argument, NULL, 'g'},
                                            {"verbose", no_argument, NULL, 'v'}};
    int c;

    assert(argc >= 0);
    assert(argv);

    while ((c = getopt_long(argc, argv, "hd:c:d:g:v", options, NULL)) >= 0) {
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
        case 'g': {
            int lvl = log_level_from_str(optarg);
            if (lvl == -EINVAL) {
                log_error("Invalid argument for debug-log-level = %s", optarg);
                help(stderr);
                return -EINVAL;
            }
            Log::set_max_level((Log::Level)lvl);
            break;
        }
        case 'v': {
            Log::set_max_level(Log::Level::DEBUG);
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

    if (!opt->conf_dir)
        opt->conf_dir = get_default_conf_dir();

    return 2;
}

int main(int argc, char *argv[])
{
    struct options opt = {0};
    Log::open();

    ConfFile *conf;
    GlibMainloop mainloop;

    if (parse_argv(argc, argv, &opt) != 2) {
        Log::close();
        return EXIT_FAILURE;
    }

    conf = new ConfFile();
    if (parse_conf_files(*conf, &opt) < 0) {
        delete conf;
        Log::close();
        return EXIT_FAILURE;
    }

    CameraServer camServer(*conf);
    camServer.start();

    delete conf;
    log_debug("Starting Dronecode Camera Manager");

    mainloop.loop();

    Log::close();

    return 0;
}
