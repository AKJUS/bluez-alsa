/*
 * mock-bluez.c
 * Copyright (c) 2016-2024 Arkadiusz Bokowy
 *
 * This file is a part of bluez-alsa.
 *
 * This project is licensed under the terms of the MIT license.
 *
 */

#include "mock.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <glib-object.h>
#include <glib.h>

#include "ba-config.h"
#include "bluez-iface.h"
#include "shared/a2dp-codecs.h"
#include "shared/bluetooth.h"
#include "shared/defs.h"
#include "shared/log.h"

#include "mock-bluez-iface.h"

/* Bluetooth device name mappings in form of "MAC:name". */
static const char * devices[8] = { NULL };
/* Global BlueZ mock server object manager. */
static GDBusObjectManagerServer *server = NULL;

/* Client manager for registered media applications. */
static GDBusObjectManager *media_app_client = NULL;
/* Mapping between profile UUID and its proxy object. */
static GHashTable *profiles = NULL;

int mock_bluez_device_name_mapping_add(const char *mapping) {
	for (size_t i = 0; i < ARRAYSIZE(devices); i++)
		if (devices[i] == NULL) {
			devices[i] = strdup(mapping);
			return 0;
		}
	return -1;
}

static void mock_bluez_profile_proxy_finish(G_GNUC_UNUSED GObject *source,
		GAsyncResult *result, void *userdata) {
	MockBluezProfile1 *profile = mock_bluez_profile1_proxy_new_finish(result, NULL);
	g_hash_table_insert(profiles, userdata, profile);
	mock_sem_signal(mock_sem_ready);
}

static gboolean mock_bluez_register_profile_handler(MockBluezProfileManager1 *manager,
		GDBusMethodInvocation *invocation, const char *path, const char *uuid,
		G_GNUC_UNUSED GVariant *options, G_GNUC_UNUSED void *userdata) {

	GDBusConnection *conn = g_dbus_method_invocation_get_connection(invocation);
	const char *sender = g_dbus_method_invocation_get_sender(invocation);
	mock_bluez_profile1_proxy_new(conn, G_DBUS_PROXY_FLAGS_NONE, sender, path,
			NULL, mock_bluez_profile_proxy_finish, g_strdup(uuid));

	mock_bluez_profile_manager1_complete_register_profile(manager, invocation);
	return TRUE;
}

static void mock_bluez_profile_manager_add(const char *path) {

	g_autoptr(MockBluezProfileManager1) manager = mock_bluez_profile_manager1_skeleton_new();
	g_signal_connect(manager, "handle-register-profile",
			G_CALLBACK(mock_bluez_register_profile_handler), NULL);

	g_autoptr(GDBusObjectSkeleton) skeleton = g_dbus_object_skeleton_new(path);
	g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(manager));
	g_dbus_object_manager_server_export(server, skeleton);

}

static gboolean mock_bluez_gatt_register_application_handler(MockBluezGattManager1 *gatt,
		GDBusMethodInvocation *invocation, G_GNUC_UNUSED const char *root,
		G_GNUC_UNUSED GVariant *options, G_GNUC_UNUSED void *userdata) {
	mock_bluez_gatt_manager1_complete_register_application(gatt, invocation);
	return TRUE;
}

static void mock_bluez_media_application_client_finish(G_GNUC_UNUSED GObject *source,
		GAsyncResult *result, G_GNUC_UNUSED void *userdata) {
	media_app_client = mock_object_manager_client_new_finish(result, NULL);
	mock_sem_signal(mock_sem_ready);
}

static gboolean mock_bluez_media_register_application_handler(MockBluezMedia1 *media,
		GDBusMethodInvocation *invocation, const char *root, G_GNUC_UNUSED GVariant *options,
		G_GNUC_UNUSED void *userdata) {

	GDBusConnection *conn = g_dbus_method_invocation_get_connection(invocation);
	const char *sender = g_dbus_method_invocation_get_sender(invocation);
	mock_object_manager_client_new(conn, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			sender, root, NULL, mock_bluez_media_application_client_finish, NULL);

	mock_bluez_media1_complete_register_application(media, invocation);
	return TRUE;
}

static void mock_bluez_adapter_add(const char *adapter_path, const char *address) {

	g_autoptr(MockBluezAdapter1) adapter = mock_bluez_adapter1_skeleton_new();
	mock_bluez_adapter1_set_address(adapter, address);

	g_autoptr(MockBluezGattManager1) gatt = mock_bluez_gatt_manager1_skeleton_new();
	g_signal_connect(gatt, "handle-register-application",
			G_CALLBACK(mock_bluez_gatt_register_application_handler), NULL);

	g_autoptr(MockBluezMedia1) media = mock_bluez_media1_skeleton_new();
	g_signal_connect(media, "handle-register-application",
			G_CALLBACK(mock_bluez_media_register_application_handler), NULL);

	g_autoptr(GDBusObjectSkeleton) skeleton = g_dbus_object_skeleton_new(adapter_path);
	g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(adapter));
	g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(gatt));
	g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(media));
	g_dbus_object_manager_server_export(server, skeleton);

}

static void mock_bluez_device_add(const char *device_path, const char *adapter_path,
		const char *address) {

	g_autoptr(MockBluezDevice1) device = mock_bluez_device1_skeleton_new();
	mock_bluez_device1_set_adapter(device, adapter_path);
	mock_bluez_device1_set_alias(device, address);
	mock_bluez_device1_set_icon(device, "audio-card");

	for (size_t i = 0; i < ARRAYSIZE(devices); i++)
		if (devices[i] != NULL &&
				strncmp(devices[i], address, strlen(address)) == 0)
			mock_bluez_device1_set_alias(device, &devices[i][strlen(address) + 1]);

	g_autoptr(GDBusObjectSkeleton) skeleton = g_dbus_object_skeleton_new(device_path);
	g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(device));
	g_dbus_object_manager_server_export(server, skeleton);

}

static gboolean mock_bluez_media_transport_acquire_handler(MockBluezMediaTransport1 *transport,
		GDBusMethodInvocation *invocation, G_GNUC_UNUSED void *userdata) {

	int fds[2];
	socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds);

	g_autoptr(GUnixFDList) fd_list = g_unix_fd_list_new_from_array(&fds[0], 1);
	mock_bluez_media_transport1_complete_try_acquire(transport, invocation,
			fd_list, g_variant_new_handle(0), 256, 256);

	g_thread_unref(mock_bt_dump_thread_new(fds[1]));
	mock_bluez_media_transport1_set_state(transport, "active");

	return TRUE;
}

static gboolean mock_bluez_media_transport_release_handler(MockBluezMediaTransport1 *transport,
		GDBusMethodInvocation *invocation, G_GNUC_UNUSED void *userdata) {
	mock_bluez_media_transport1_complete_release(transport, invocation);
	mock_bluez_media_transport1_set_state(transport, "idle");
	return TRUE;
}

static MockBluezMediaTransport1 * mock_bluez_media_transport_add(const char *transport_path,
		const char *device_path) {

	MockBluezMediaTransport1 *transport = mock_bluez_media_transport1_skeleton_new();
	mock_bluez_media_transport1_set_device(transport, device_path);
	mock_bluez_media_transport1_set_state(transport, "idle");

	g_signal_connect(transport, "handle-acquire",
			G_CALLBACK(mock_bluez_media_transport_acquire_handler), NULL);
	g_signal_connect(transport, "handle-try-acquire",
			G_CALLBACK(mock_bluez_media_transport_acquire_handler), NULL);
	g_signal_connect(transport, "handle-release",
			G_CALLBACK(mock_bluez_media_transport_release_handler), NULL);

	g_autoptr(GDBusObjectSkeleton) skeleton = g_dbus_object_skeleton_new(transport_path);
	g_dbus_object_skeleton_add_interface(skeleton, G_DBUS_INTERFACE_SKELETON(transport));
	g_dbus_object_manager_server_export(server, skeleton);

	return transport;
}

static void *mock_bluez_rfcomm_thread(void *userdata) {

	static const struct {
		const char *command;
		const char *response;
	} responses[] = {
		/* accept HFP codec selection */
		{ "\r\n+BCS:1\r\n", "AT+BCS=1\r" },
		{ "\r\n+BCS:2\r\n", "AT+BCS=2\r" },
		{ "\r\n+BCS:3\r\n", "AT+BCS=3\r" },
	};

	int rfcomm_fd = GPOINTER_TO_INT(userdata);
	char buffer[1024];
	ssize_t len;

	while ((len = read(rfcomm_fd, buffer, sizeof(buffer))) > 0) {
		hexdump("RFCOMM", buffer, len, true);

		for (size_t i = 0; i < ARRAYSIZE(responses); i++) {
			if (strncmp(buffer, responses[i].command, len) != 0)
				continue;
			len = strlen(responses[i].response);
			if (write(rfcomm_fd, responses[i].response, len) != len)
				warn("Couldn't write RFCOMM response: %s", strerror(errno));
			break;
		}

	}

	close(rfcomm_fd);
	return NULL;
}

static void mock_bluez_profile_new_connection_finish(GObject *source,
		GAsyncResult *result, G_GNUC_UNUSED void *userdata) {
	MockBluezProfile1 *profile = MOCK_BLUEZ_PROFILE1(source);
	mock_bluez_profile1_call_new_connection_finish(profile, NULL, result, NULL);
	mock_sem_signal(userdata);
}

int mock_bluez_device_profile_new_connection(const char *device_path,
		const char *uuid, GAsyncQueue *sem_ready) {

	int fds[2];
	socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

	g_autoptr(GUnixFDList) fd_list = g_unix_fd_list_new_from_array(&fds[0], 1);
	mock_bluez_profile1_call_new_connection(g_hash_table_lookup(profiles, uuid),
			device_path, g_variant_new_handle(0), g_variant_new("a{sv}", NULL),
			fd_list, NULL, mock_bluez_profile_new_connection_finish, sem_ready);

	g_thread_unref(g_thread_new(NULL, mock_bluez_rfcomm_thread, GINT_TO_POINTER(fds[1])));

	return 0;
}

static void mock_bluez_media_endpoint_set_configuration_finish(GObject *source,
		GAsyncResult *result, G_GNUC_UNUSED void *userdata) {
	MockBluezMediaEndpoint1 *endpoint = MOCK_BLUEZ_MEDIA_ENDPOINT1(source);
	mock_bluez_media_endpoint1_call_set_configuration_finish(endpoint, result, NULL);
	mock_sem_signal(userdata);
}

int mock_bluez_device_media_set_configuration(const char *device_path,
		const char *transport_path, const char *uuid, uint32_t codec_id,
		const void *configuration, size_t configuration_size,
		GAsyncQueue *sem_ready) {

	const uint8_t codec = codec_id < A2DP_CODEC_VENDOR ? codec_id : A2DP_CODEC_VENDOR;
	const uint32_t vendor = codec_id < A2DP_CODEC_VENDOR ? 0 : codec_id;
	int rv = -1;

	GList *endpoints = g_dbus_object_manager_get_objects(media_app_client);
	for (GList *elem = endpoints; elem != NULL; elem = elem->next) {
		MockBluezMediaEndpoint1 *ep = mock_object_get_bluez_media_endpoint1(elem->data);
		if (mock_bluez_media_endpoint1_get_device(ep) == NULL &&
				strcmp(mock_bluez_media_endpoint1_get_uuid(ep), uuid) == 0 &&
				mock_bluez_media_endpoint1_get_codec(ep) == codec &&
				mock_bluez_media_endpoint1_get_vendor(ep) == vendor) {

			g_autoptr(MockBluezMediaTransport1) transport;
			transport = mock_bluez_media_transport_add(transport_path, device_path);

			g_autoptr(GVariantBuilder) props = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
			g_variant_builder_add(props, "{sv}", "Device", g_variant_new_object_path(
						mock_bluez_media_transport1_get_device(transport)));
			g_variant_builder_add(props, "{sv}", "Configuration", g_variant_new_fixed_array(
						G_VARIANT_TYPE_BYTE, configuration, configuration_size, sizeof(uint8_t)));
			g_variant_builder_add(props, "{sv}", "State", g_variant_new_string(
						mock_bluez_media_transport1_get_state(transport)));

			mock_bluez_media_endpoint1_call_set_configuration(ep, transport_path,
					g_variant_builder_end(props), NULL,
					mock_bluez_media_endpoint_set_configuration_finish, sem_ready);

			/* In case of A2DP Sink profile, activate the transport right away. */
			if (strcmp(uuid, BT_UUID_A2DP_SINK) == 0)
				mock_bluez_media_transport1_set_state(transport, "pending");

			rv = 0;
			break;
		}
	}

	g_list_free_full(endpoints, g_object_unref);
	return rv;
}

static void mock_bluez_dbus_name_acquired(GDBusConnection *conn,
		G_GNUC_UNUSED const char *name, void *userdata) {

	server = g_dbus_object_manager_server_new("/");
	profiles = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);

	mock_bluez_profile_manager_add("/org/bluez");
	mock_bluez_adapter_add(MOCK_BLUEZ_ADAPTER_PATH, "00:00:11:11:22:22");

	mock_bluez_device_add(MOCK_BLUEZ_DEVICE_PATH_1, MOCK_BLUEZ_ADAPTER_PATH, MOCK_DEVICE_1);
	mock_bluez_device_add(MOCK_BLUEZ_DEVICE_PATH_2, MOCK_BLUEZ_ADAPTER_PATH, MOCK_DEVICE_2);

	g_dbus_object_manager_server_set_connection(server, conn);
	mock_sem_signal(userdata);

}

static GThread *mock_bluez_thread = NULL;
static GMainLoop *mock_bluez_main_loop = NULL;
static unsigned int mock_bluez_owner_id = 0;

static void *mock_bluez_loop_run(void *userdata) {

	g_autoptr(GMainContext) context = g_main_context_new();
	mock_bluez_main_loop = g_main_loop_new(context, FALSE);
	g_main_context_push_thread_default(context);

	g_assert((mock_bluez_owner_id = g_bus_own_name_on_connection(config.dbus,
					BLUEZ_SERVICE, G_BUS_NAME_OWNER_FLAGS_NONE,
					mock_bluez_dbus_name_acquired, NULL, userdata, NULL)) != 0);

	g_main_loop_run(mock_bluez_main_loop);

	g_main_context_pop_thread_default(context);
	return NULL;
}

void mock_bluez_service_start(void) {
	g_autoptr(GAsyncQueue) ready = g_async_queue_new();
	mock_bluez_thread = g_thread_new("bluez", mock_bluez_loop_run, ready);
	mock_sem_wait(ready);
}

void mock_bluez_service_stop(void) {

	g_bus_unown_name(mock_bluez_owner_id);

	g_main_loop_quit(mock_bluez_main_loop);
	g_main_loop_unref(mock_bluez_main_loop);
	g_thread_join(mock_bluez_thread);

	g_hash_table_unref(profiles);
	if (media_app_client != NULL)
		g_object_unref(media_app_client);
	g_object_unref(server);

}
