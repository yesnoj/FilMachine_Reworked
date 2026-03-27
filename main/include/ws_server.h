/**
 * @file ws_server.h
 *
 * WebSocket server for remote control of FilMachine.
 *
 * Protocol overview (all messages are JSON text frames):
 *
 *  Server → Client  (broadcasts)
 *  ──────────────────────────────
 *  { "type": "state",    "data": { ... full machine state ... } }
 *  { "type": "process_list", "data": [ ... array of processes ... ] }
 *  { "type": "event",    "event": "<name>", "data": { ... } }
 *
 *  Client → Server  (commands)
 *  ──────────────────────────────
 *  { "cmd": "get_state" }
 *  { "cmd": "get_processes" }
 *  { "cmd": "set_setting",  "key": "<field>", "value": <val> }
 *  { "cmd": "start_process","index": <n> }
 *  { "cmd": "stop_now" }
 *  { "cmd": "stop_after" }
 *  { "cmd": "pause" }
 *  { "cmd": "resume" }
 *  { "cmd": "create_process", "data": { ... } }
 *  { "cmd": "edit_process",   "index": <n>, "data": { ... } }
 *  { "cmd": "delete_process", "index": <n> }
 *  { "cmd": "duplicate_process", "index": <n> }
 *  { "cmd": "run_cleaning",   "tanks": [0,1,2], "cycles": <n>, "drain": <bool> }
 *  { "cmd": "run_drain" }
 *  { "cmd": "run_selfcheck" }
 *  { "cmd": "wifi_scan" }
 */

#ifndef WS_SERVER_H
#define WS_SERVER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Server lifecycle ── */

/**
 * Start the WebSocket server on the given port.
 * Returns true on success.  Safe to call multiple times (no-op if running).
 */
bool ws_server_start(uint16_t port);

/**
 * Stop the WebSocket server and disconnect all clients.
 */
void ws_server_stop(void);

/**
 * @return true if the server is currently running.
 */
bool ws_server_is_running(void);

/**
 * @return number of currently connected WebSocket clients.
 */
int ws_server_client_count(void);

/* ── Broadcasting ── */

/**
 * Broadcast the full machine state to all connected clients.
 * Call this periodically (e.g. every 500 ms) or on state change.
 */
void ws_broadcast_state(void);

/**
 * Broadcast the full process list to all connected clients.
 * Call this after process create / edit / delete / reorder.
 */
void ws_broadcast_process_list(void);

/**
 * Broadcast a named event with optional JSON payload.
 * Example: ws_broadcast_event("alarm", "{\"active\":true}");
 */
void ws_broadcast_event(const char *event_name, const char *json_data);

/* ── Default port ── */
#define WS_SERVER_PORT 81

#ifdef __cplusplus
}
#endif

#endif /* WS_SERVER_H */
