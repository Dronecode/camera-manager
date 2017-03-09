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
#include <assert.h>
#include <avahi-common/error.h>
#include <stdexcept>

#include "avahi_publisher.h"
#include "log.h"
#include "mainloop.h"

AvahiPublisher::AvahiPublisher(std::vector<Stream> &_streams, int _port, const char *_type)
    : is_running(false)
    , avahi_poll(nullptr)
    , streams(_streams)
    , client(nullptr)
    , group(nullptr)
    , port(_port)
    , type(_type)
{
}

AvahiPublisher::~AvahiPublisher()
{
    stop();
}

void AvahiPublisher::avahi_client_cb(AvahiClient *c, AvahiClientState state, void *userdata)
{
    assert(c);
    AvahiPublisher *publisher = (AvahiPublisher *)userdata;

    switch (state) {
    case AVAHI_CLIENT_S_RUNNING:
        publisher->publish_services(c);
        break;

    case AVAHI_CLIENT_FAILURE:
        log_error("Failed to create avahi client: %s", avahi_strerror(avahi_client_errno(c)));
        // TODO: Restart avahi publisher
        break;

    case AVAHI_CLIENT_S_COLLISION:
    case AVAHI_CLIENT_S_REGISTERING:
        publisher->reset_services();
        break;

    case AVAHI_CLIENT_CONNECTING:;
    }
}

void AvahiPublisher::entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                          void *userdata)
{
    assert(g);

    /* Called whenever the entry group state changes */

    switch (state) {
    case AVAHI_ENTRY_GROUP_COLLISION:
        log_debug("Service name collision. Ignoring service");
        // TODO: Handle error
        break;

    case AVAHI_ENTRY_GROUP_FAILURE:
        log_error("Failed to create avahi group");
        // TODO: Restart avahi publisher
        break;

    default:;
    }
}

void AvahiPublisher::reset_services()
{
    if (group)
        avahi_entry_group_reset(group);
}

void AvahiPublisher::publish_services(AvahiClient *c)
{
    int ret;

    if (!group) {
        group = avahi_entry_group_new(c, entry_group_callback, NULL);
        if (!group) {
            log_error("Failed to create avahi group: %s", avahi_strerror(avahi_client_errno(c)));
            // TODO: Restart avahi publisher
        }
    }

    if (avahi_entry_group_is_empty(group)) {
        for (auto const &value : streams) {
            ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC,
                                                (AvahiPublishFlags)0, value.name.c_str(), type,
                                                NULL, NULL, port, NULL);
            if (ret != 0) {
                // TODO: handle error
                log_error("Failed to add service %s", value.name.c_str());
            }
        }

        ret = avahi_entry_group_commit(group);
        if (ret < 0) {
            // TODO: handle error
            log_error("Failed to add services to avahi group.");
        }
    }
}

void AvahiPublisher::start()
{
    if (is_running)
        return;
    is_running = true;

    int error;

    client = avahi_client_new(Mainloop::get_mainloop()->get_avahi_poll_api(), (AvahiClientFlags)0,
                              avahi_client_cb, this, &error);
    if (!client) {
        log_error("Unalbe to create avahi client. Video streams won't be published as avahi services.");
    }
}

void AvahiPublisher::stop()
{
    if (!is_running)
        return;
    is_running = false;

    avahi_client_free(client);
    client = nullptr;
    group = nullptr;
}
