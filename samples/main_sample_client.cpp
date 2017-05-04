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
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

#include "glib_mainloop.h"
#include "log.h"
#include "mainloop.h"

static void resolve_callback(AvahiServiceResolver *resolver, AvahiIfIndex interface,
                             AvahiProtocol protocol, AvahiResolverEvent event, const char *name,
                             const char *type, const char *domain, const char *host_name,
                             const AvahiAddress *address, uint16_t port, AvahiStringList *txt,
                             AvahiLookupResultFlags flags, void *userdata)
{
    assert(resolver);
    assert(userdata);

    switch (event) {
    case AVAHI_RESOLVER_FAILURE:
        log_error("Avahi service resolve failed for service '%s' in domain '%s'"
                  ". error: %s",
                  name, domain, avahi_strerror(avahi_client_errno((AvahiClient *)userdata)));
        break;

    case AVAHI_RESOLVER_FOUND: {
        char address_str[AVAHI_ADDRESS_STR_MAX], *txt_str;
        avahi_address_snprint(address_str, sizeof(address_str), address);

        log_info("Service resolved: '%s' (rtsp://%s:%u%s)", name, address_str, port, name);

        txt_str = avahi_string_list_to_string(txt);
        log_info("TXT: [%s]", txt_str);

        avahi_free(txt_str);
    }
    }

    avahi_service_resolver_free(resolver);
}
static void browse_callback(AvahiServiceBrowser *sb, AvahiIfIndex interface, AvahiProtocol protocol,
                            AvahiBrowserEvent event, const char *name, const char *type,
                            const char *domain, AvahiLookupResultFlags flags, void *userdata)
{
    AvahiClient *client = (AvahiClient *)userdata;
    assert(sb);

    switch (event) {
    case AVAHI_BROWSER_FAILURE:
        log_error("Avahi Browser error");
        Mainloop::get_mainloop()->quit();
        break;

    case AVAHI_BROWSER_NEW:
        if (!(avahi_service_resolver_new(client, interface, protocol, name, type, domain,
                                         AVAHI_PROTO_INET, (AvahiLookupFlags)0, resolve_callback,
                                         client)))
            log_error("Failed to start resolver for service. name: '%s' in domain: %s. "
                      "error: %s",
                      name, domain, avahi_strerror(avahi_client_errno(client)));
        break;

    case AVAHI_BROWSER_REMOVE:
        log_info("Service removed: '%s'", name);
        break;

    case AVAHI_BROWSER_ALL_FOR_NOW:
    case AVAHI_BROWSER_CACHE_EXHAUSTED:
        break;
    }
}

static void client_callback(AvahiClient *client, AvahiClientState state, void *userdata)
{
    assert(client);

    if (state == AVAHI_CLIENT_FAILURE) {
        log_error("Avahi client error");
        Mainloop::get_mainloop()->quit();
    }
}

int main(int argc, char *argv[])
{
    Log::open();

    AvahiClient *client = NULL;
    AvahiServiceBrowser *sb = NULL;
    int error;
    GlibMainloop mainloop;

    log_debug("Camera Streaming Client");

    client = avahi_client_new(mainloop.get_avahi_poll_api(), (AvahiClientFlags)0, client_callback,
                              NULL, &error);
    if (!client) {
        log_error("Failed to create avahi client: %s\n", avahi_strerror(error));
        goto error;
    }

    if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, "_rtsp._udp",
                                         NULL, (AvahiLookupFlags)0, browse_callback, client))) {
        log_error("Failed to create avahi service browser: %s\n", avahi_strerror(error));
        goto error;
    }
    mainloop.loop();

error:
    if (sb)
        avahi_service_browser_free(sb);
    if (client)
        avahi_client_free(client);
    Log::close();

    return 0;
}
