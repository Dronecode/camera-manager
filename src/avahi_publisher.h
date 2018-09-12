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
#pragma once

#ifdef ENABLE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/watch.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

class AvahiPublisher {
public:
    AvahiPublisher(std::map<std::string, std::vector<std::string>> &_strMap, int _port,
                   const char *_type);
    ~AvahiPublisher();
    void start();
    void stop();

private:
    bool is_running;
    const AvahiPoll *avahi_poll;
    std::map<std::string, std::vector<std::string>> &info_map;
    AvahiClient *client;
    AvahiEntryGroup *group;
    int port;
    const char *type;

    void publish_services(AvahiClient *c);
    void reset_services();
    static void avahi_client_cb(AvahiClient *c, AvahiClientState state, void *userdata);
    static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state,
                                     void *userdata);
    AvahiStringList *txt_record_from_list(const std::vector<std::string> &txtList);
};
#endif
