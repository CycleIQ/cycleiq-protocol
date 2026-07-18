#include "cycleiq_protocol.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static void test_walk_command(bool enabled) {
  cycleiq_frame_t frame;
  bool decoded = !enabled;

  assert(cycleiq_set_walk_mode(&frame, enabled));
  assert(frame.id ==
         CYCLEIQ_CAN_FRAME_ID(CYCLEIQ_CAN_ID, CYCLEIQ_COMM_WALK_SET));
  assert(frame.len == CYCLEIQ_COMMAND_WALK_SET_LEN);
  assert(frame.data[0] == (enabled ? 1u : 0u));
  assert(cycleiq_command_read_walk_mode(&frame, &decoded));
  assert(decoded == enabled);
}

static void test_walk_command_rejects_malformed_frames(void) {
  cycleiq_frame_t frame;
  bool enabled = false;

  assert(cycleiq_set_walk_mode(&frame, true));

  frame.len = 0u;
  assert(!cycleiq_command_read_walk_mode(&frame, &enabled));

  frame.len = 2u;
  assert(!cycleiq_command_read_walk_mode(&frame, &enabled));

  frame.len = CYCLEIQ_COMMAND_WALK_SET_LEN;
  frame.data[0] = 2u;
  assert(!cycleiq_command_read_walk_mode(&frame, &enabled));

  frame.data[0] = 1u;
  frame.id = CYCLEIQ_CAN_FRAME_ID(CYCLEIQ_CAN_ID, CYCLEIQ_COMM_GEAR_SET);
  assert(!cycleiq_command_read_walk_mode(&frame, &enabled));
  assert(!cycleiq_command_read_walk_mode(NULL, &enabled));
  assert(!cycleiq_command_read_walk_mode(&frame, NULL));
}

static void test_walk_state(bool active) {
  cycleiq_frame_t frame;
  bool decoded = !active;

  assert(cycleiq_telemetry_walk_state(&frame, active));
  assert(frame.id ==
         CYCLEIQ_CAN_FRAME_ID(PEAK_CAN_ID, PEAK_PACKET_TYPE_WALK_STATE));
  assert(frame.len == PEAK_PACKET_WALK_STATE_LEN);
  assert(frame.data[0] == (active ? 1u : 0u));
  assert(cycleiq_read_walk_state(&frame, &decoded));
  assert(decoded == active);
}

static void test_walk_state_rejects_malformed_frames(void) {
  cycleiq_frame_t frame;
  bool active = false;

  assert(cycleiq_telemetry_walk_state(&frame, true));

  frame.len = 0u;
  assert(!cycleiq_read_walk_state(&frame, &active));

  frame.len = 2u;
  assert(!cycleiq_read_walk_state(&frame, &active));

  frame.len = PEAK_PACKET_WALK_STATE_LEN;
  frame.data[0] = 2u;
  assert(!cycleiq_read_walk_state(&frame, &active));

  frame.data[0] = 1u;
  frame.id = CYCLEIQ_CAN_FRAME_ID(PEAK_CAN_ID,
                                  PEAK_PACKET_TYPE_CONTROLLER_STATE);
  assert(!cycleiq_read_walk_state(&frame, &active));
  assert(!cycleiq_read_walk_state(NULL, &active));
  assert(!cycleiq_read_walk_state(&frame, NULL));
}

static void test_versions(void) {
  cycleiq_frame_t frame;
  cycleiq_version_t protocol;
  cycleiq_version_t sdk;

  assert(cycleiq_telemetry_protocol_version(&frame));
  assert(frame.id == CYCLEIQ_CAN_FRAME_ID(
                         PEAK_CAN_ID, PEAK_PACKET_TYPE_PROTOCOL_VERSION));
  assert(frame.len == PEAK_PACKET_PROTOCOL_VERSION_LEN);
  assert(cycleiq_read_protocol_version(&frame, &protocol, &sdk));

  assert(protocol.major == 1u);
  assert(protocol.minor == 3u);
  assert(protocol.patch == 0u);
  assert(sdk.major == 0u);
  assert(sdk.minor == 4u);
  assert(sdk.patch == 0u);
}

int main(void) {
  test_walk_command(false);
  test_walk_command(true);
  test_walk_command_rejects_malformed_frames();
  test_walk_state(false);
  test_walk_state(true);
  test_walk_state_rejects_malformed_frames();
  test_versions();
  return 0;
}
