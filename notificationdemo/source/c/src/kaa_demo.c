/**
 *  Copyright 2014-2016 CyberVision, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include <kaa_error.h>
#include <platform/kaa_client.h>
#include <utilities/kaa_log.h>
#include <kaa_notification_manager.h>

#define KAA_DEMO_UNUSED(x) (void)(x);

static kaa_client_t *kaa_client = NULL;

enum color_t {RED, YELLOW, GREEN};

static const char *enum_to_str(enum color_t code_color)
{
    switch(code_color) {
        case RED:
            return "CodeRed";
        case YELLOW:
            return "CodeYellow";
        case GREEN:
            return "CodeGreen";
        default:
            return "Incorrect value";
    }
}

void on_notification(void *context, uint64_t *topic_id, kaa_notification_t *notification)
{
    KAA_DEMO_UNUSED(context);
    if (notification->alert_message) {
            kaa_string_t *message = (kaa_string_t *)notification->alert_message;
            printf("Notification for topic id '%llu' received\n", *topic_id);
            printf("Notification body: %s\n", message->data);
            printf("Message alert type: %s\n", enum_to_str(notification->alert_type));
    } else {
        printf("Error:Received notification's body is null\n");
    }
}

void show_topics(kaa_list_t *topics)
{
    if (!topics || !kaa_list_get_size(topics)) {
        printf("Topic list is empty");
        return;
    }

    kaa_list_node_t *it = kaa_list_begin(topics);
    while (it) {
        kaa_topic_t *topic = (kaa_topic_t *)kaa_list_get_data(it);
        printf("Topic: id '%llu', name: %s, type: ", topic->id, topic->name);
        if (topic->subscription_type == MANDATORY_SUBSCRIPTION) {
            printf("MANDATORY\n");
        } else {
            printf("OPTIONAL\n");
        }
        it = kaa_list_next(it);
    }
}

void on_topics_received(void *context, kaa_list_t *topics)
{
    printf("Topic list was updated\n");
    show_topics(topics);

    printf("Type topic ID in order to subscribe on one:");
    size_t topic_id;
    scanf("%zu", &topic_id);

    kaa_error_t err = KAA_ERR_NONE;
    kaa_client_t *client = (kaa_client_t *)context;
    kaa_list_node_t *it = kaa_list_begin(topics);
    while (it) {
        kaa_topic_t *topic = (kaa_topic_t *) kaa_list_get_data(it);
        if (topic->subscription_type == OPTIONAL_SUBSCRIPTION && topic->id == topic_id) {
            printf("Subscribing to optional topic '%llu'\n", topic->id);
            err = kaa_subscribe_to_topic(kaa_client_get_context(client)->notification_manager, &topic->id, false);
            if (err) {
                printf("Failed to subscribe.\n");
            }
        }
        it = kaa_list_next(it);
    }
    err = kaa_sync_topic_subscriptions(kaa_client_get_context(kaa_client)->notification_manager);
    if (err) {
        printf("Failed to sync subscriptions\n");
    }
}

int main(/*int argc, char *argv[]*/)
{
    printf("Notification demo started\n");

    /**
     * Initialize Kaa client.
     */
    kaa_error_t error_code = kaa_client_create(&kaa_client, NULL);
    if (error_code) {
        printf("Failed create Kaa client %d\n", error_code);
        return error_code;
    }

    kaa_topic_listener_t topic_listener = { &on_topics_received, kaa_client };
    kaa_notification_listener_t notification_listener = { &on_notification, kaa_client };

    uint32_t topic_listener_id = 0;
    uint32_t notification_listener_id = 0;

    error_code = kaa_add_topic_list_listener(kaa_client_get_context(kaa_client)->notification_manager
                                           , &topic_listener
                                           , &topic_listener_id);
    if (error_code) {
        printf("Failed add topic listener %d\n", error_code);
        kaa_client_destroy(kaa_client);
        return error_code;
    }

    error_code = kaa_add_notification_listener(kaa_client_get_context(kaa_client)->notification_manager
                                             , &notification_listener
                                             , &notification_listener_id);
    if (error_code) {
        printf("Failed add notification listener %d\n", error_code);
        kaa_client_destroy(kaa_client);
        return error_code;
    }

    /**
     * Start Kaa client main loop.
     */
    error_code = kaa_client_start(kaa_client, NULL, NULL, 0);
    if (error_code) {
        printf("Failed to start Kaa main loop %d\n", error_code);
        kaa_client_destroy(kaa_client);
        return error_code;
    }

    /**
     * Destroy Kaa client.
     */
    kaa_client_destroy(kaa_client);

    printf("Notification demo stopped\n");
    return error_code;
}
