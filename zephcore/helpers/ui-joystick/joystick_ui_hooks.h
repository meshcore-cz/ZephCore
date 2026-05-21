/*
 * ZephCore - Joystick UI Hooks
 * Copyright (c) 2026 ZephCore
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bridges the ui_task.h C API to JoystickUITask, and declares the
 * joystick-only extension API used by CompanionMesh when
 * CONFIG_ZEPHCORE_UI_DESIGN_JOYSTICK=y.
 *
 * Non-joystick builds never include this header; guards in CompanionMesh.cpp
 * and main_companion.cpp wrap every include and call site.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Signal callback type, passed during registration so joystick_ui_hooks.cpp
 * can post events to the mesh loop without depending on main_companion internals.
 */
typedef void (*ui_signal_fn)(void);

/**
 * Register the JoystickUITask instance and event signal callbacks.
 * Must be called (from main_companion.cpp) before the mesh event loop starts.
 *
 * @param task Pointer to JoystickUITask instance
 * @param signal_refresh Callback to wake the joystick UI loop tick
 * @param signal_tx Callback to wake the mesh TX path immediately
 */
void joystick_ui_hooks_register(void *task,
	ui_signal_fn signal_refresh,
	ui_signal_fn signal_tx);

/*
 * Joystick UI events: extend by adding a new ui_joystick_event_type value
 * and payload struct rather than adding another function.
 * Covers ping, login, CLI, discovery, and future joystick features.
 * Only compiled in when CONFIG_ZEPHCORE_UI_DESIGN_JOYSTICK is enabled.
 */

enum ui_joystick_event_type {
	UI_JOYSTICK_LOGIN_RESULT   = 0, /* login accepted/rejected */
	UI_JOYSTICK_CLI_RESPONSE   = 1, /* admin CLI command response */
	UI_JOYSTICK_REQ_RESPONSE   = 2, /* unmatched contact REQ response, joystick checks for ping */
	UI_JOYSTICK_DISCOVER_RESP  = 3, /* CTL_TYPE_NODE_DISCOVER_RESP received */
};

struct ui_joystick_login_data {
	bool success;
	uint8_t permissions;
	uint32_t server_time;
};

struct ui_joystick_cli_data {
	const char *text;
};

struct ui_joystick_req_response_data {
	int8_t snr_local;    /* SNR of reply as received by us */
	int8_t snr_remote;   /* SNR of our REQ as seen by repeater, INT8_MIN if unavailable */
	const uint8_t *data; /* response payload after the 4 byte tag (may be NULL) */
	uint8_t data_len;
};

struct ui_joystick_discover_data {
	int8_t snr;        /* SNR of response as received by us (FROM repeater) */
	int8_t snr_remote; /* SNR of our request as received by repeater (TO repeater) */
	uint8_t path_len;
};

/**
 * Dispatch a joystick UI event from the mesh layer.
 *
 * @param type Event discriminator
 * @param pub_key_prefix First 4 bytes of the contact's public key
 * @param payload Pointer to the matching ui_joystick_*_data struct
 */
void ui_notify_joystick_event(enum ui_joystick_event_type type,
	const uint8_t *pub_key_prefix,
	const void *payload);

/**
 * Update radio packet counters (Rx, Tx, errors) for the joystick stats screen.
 * Not part of ui_task.h, call only when CONFIG_ZEPHCORE_UI_DESIGN_JOYSTICK=y.
 */
void ui_notify_radio_stats(uint32_t pkt_recv, uint32_t pkt_sent, uint32_t pkt_errors);

/**
 * Wake the joystick UI loop immediately (before the next render timer fires),
 * call after a time sensitive event (ping response, login).
 */
void ui_signal_refresh(void);

/**
 * Wake the mesh TX path immediately so a packet queued by the joystick UI
 * (ping, login, CLI command, message) is sent without waiting for the next
 * radio event.
 */
void ui_signal_tx(void);

/**
 * Try to match an incoming ACK against the joystick UI's pending-DM table.
 * On match: cancels the retry timer, marks the entry delivered, fires the
 * deferred BLE-app mirror with the success prefix, and writes the recipient's
 * 6-byte pubkey prefix to out_pubkey (caller uses it for BaseChatMesh's
 * return-path-retry housekeeping).
 *
 * @return true if matched (out_pubkey is filled), false otherwise
 */
bool ui_joystick_try_match_ack(uint32_t ack, uint8_t out_pubkey[6]);

#ifdef __cplusplus
}
#endif
