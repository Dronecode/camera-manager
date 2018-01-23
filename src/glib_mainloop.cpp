/*
 * This file is part of the Camera Streaming Daemon
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
#include <algorithm>
#include <assert.h>
#include <glib-unix.h>
#include <glib.h>
#include <signal.h>

#include "glib_mainloop.h"
#include "log.h"

static GMainLoop *gmainloop = nullptr;

struct fd_handler {
    bool (*cb)(const void *data, int flags);
    const void *data;
};

static gboolean exit_signal_handler(gpointer mainloop)
{
    g_main_loop_quit((GMainLoop *)mainloop);
    return true;
}

static void setup_signal_handlers()
{
    GSource *source = NULL;
    GMainContext *ctx = g_main_loop_get_context(gmainloop);

    source = g_unix_signal_source_new(SIGINT);
    g_source_set_callback(source, exit_signal_handler, gmainloop, NULL);
    g_source_attach(source, ctx);

    source = g_unix_signal_source_new(SIGTERM);
    g_source_set_callback(source, exit_signal_handler, gmainloop, NULL);
    g_source_attach(source, ctx);
}

GlibMainloop::GlibMainloop()
#ifdef ENABLE_AVAHI
    : avahi_poll(nullptr)
#endif
{
    gmainloop = g_main_loop_new(NULL, FALSE);
    mainloop = this;
    setup_signal_handlers();
}

GlibMainloop::~GlibMainloop()
{
    mainloop = nullptr;
#ifdef ENABLE_AVAHI
    if (avahi_poll) {
        avahi_glib_poll_free(avahi_poll);
        avahi_poll = nullptr;
    }
#endif

    g_main_loop_unref(gmainloop);
    gmainloop = nullptr;
}

void GlibMainloop::loop()
{
    g_main_loop_run(gmainloop);
}

#ifdef ENABLE_AVAHI
const AvahiPoll *GlibMainloop::get_avahi_poll_api()
{
    if (!avahi_poll)
        avahi_poll = avahi_glib_poll_new(NULL, G_PRIORITY_DEFAULT);
    return avahi_glib_poll_get(avahi_poll);
}
#endif

void GlibMainloop::quit()
{
    g_main_loop_quit(gmainloop);
}

unsigned int GlibMainloop::add_timeout(unsigned int timeout_msec, bool (*cb)(void *),
                                       const void *data)
{
    assert(cb);

    return g_timeout_add(timeout_msec, (GSourceFunc)cb, (void *)data);
}

void GlibMainloop::del_timeout(unsigned int timeout_handler)
{
    g_source_remove(timeout_handler);
}

static gboolean fd_io_cb(gint fd, GIOCondition condition, gpointer user_data)
{
    int flags = 0;
    struct fd_handler *handler;

    assert(user_data);
    handler = (struct fd_handler *)user_data;

    log_debug("Poll IO fd: %d flag: %d", fd, condition);

    if (condition & G_IO_IN)
        flags |= Mainloop::IO_IN;
    if (condition & G_IO_OUT)
        flags |= Mainloop::IO_OUT;

    return handler->cb(handler->data, flags);
}

static void fd_del_cb(void *data)
{
    assert(data);

    delete (struct fd_handler *)data;
}

int GlibMainloop::add_fd(int fd, int flags, bool (*cb)(const void *data, int flags), const void *data)
{
    struct fd_handler *handler;
    int glib_flags = 0;

    assert(cb);

    if (flags & Mainloop::IO_IN)
        glib_flags |= G_IO_IN;
    if (flags & Mainloop::IO_OUT)
        glib_flags |= G_IO_OUT;

    if (!glib_flags) {
        log_error("No valid flags are set. flags=%d", flags);
        return -EINVAL;
    }

    handler = new fd_handler{cb, data};
    log_debug("Adding fd: %d glib_flags: %d", fd, glib_flags);
    return g_unix_fd_add_full(0, fd, (GIOCondition)glib_flags, fd_io_cb, handler, fd_del_cb);
}

void GlibMainloop::remove_fd(int handler)
{
    assert(handler > 0);

    g_source_remove(handler);
}
