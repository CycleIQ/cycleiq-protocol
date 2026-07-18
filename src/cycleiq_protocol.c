#include "cycleiq_protocol.h"

#include <stddef.h>
#include <string.h>

static uint16_t cycleiq_float_to_u16(float value, float scale) {
  if (value <= 0.0f) {
    return 0;
  }

  value *= scale;
  if (value >= 65535.0f) {
    return 65535;
  }

  return (uint16_t)value;
}

static int16_t cycleiq_float_to_i16(float value, float scale) {
  value *= scale;
  if (value >= 32767.0f) {
    return 32767;
  }
  if (value <= -32768.0f) {
    return -32768;
  }

  return (int16_t)value;
}

static uint8_t cycleiq_float_to_u8(float value, float scale) {
  if (value <= 0.0f) {
    return 0;
  }

  value *= scale;
  if (value >= 255.0f) {
    return 255;
  }

  return (uint8_t)value;
}

static uint32_t cycleiq_float_to_u32(float value, float scale) {
  if (value <= 0.0f) {
    return 0;
  }

  value *= scale;
  if (value >= 4294967295.0f) {
    return 4294967295u;
  }

  return (uint32_t)value;
}

static void cycleiq_write_be_u16(uint8_t *out, uint16_t value) {
  out[0] = (uint8_t)(value >> 8);
  out[1] = (uint8_t)value;
}

static uint16_t cycleiq_read_be_u16(const uint8_t *in) {
  return ((uint16_t)in[0] << 8) | (uint16_t)in[1];
}

static void cycleiq_write_be_i16(uint8_t *out, int16_t value) {
  cycleiq_write_be_u16(out, (uint16_t)value);
}

static void cycleiq_write_be_u32(uint8_t *out, uint32_t value) {
  out[0] = (uint8_t)(value >> 24);
  out[1] = (uint8_t)(value >> 16);
  out[2] = (uint8_t)(value >> 8);
  out[3] = (uint8_t)value;
}

static bool cycleiq_valid_support_mode(cycleiq_support_mode_t mode) {
  return mode <= CYCLEIQ_MODE_HYBRID;
}

static bool cycleiq_valid_ride_mode(cycleiq_ride_mode_t mode) {
  return mode <= CYCLEIQ_RIDE_MODE_MOUNTAIN;
}

static bool cycleiq_valid_screen(cycleiq_screen_t screen) {
  return screen <= CYCLEIQ_SCREEN_GRAPH;
}

static bool cycleiq_valid_config_field(cycleiq_config_field_t field) {
  return field <= CYCLEIQ_CONFIG_FIELD_WHEEL_DIAMETER_MM;
}

static bool cycleiq_valid_config_op(cycleiq_config_op_t op) {
  return op <= CYCLEIQ_CONFIG_OP_DISCARD;
}

static bool cycleiq_set_len(cycleiq_frame_t *frame, uint8_t len) {
  if (frame == NULL || len > CYCLEIQ_CAN_MAX_PAYLOAD_LEN) {
    return false;
  }

  frame->len = len;
  return true;
}

cycleiq_version_t cycleiq_sdk_version(void) {
  cycleiq_version_t version = {
      .major = CYCLEIQ_SDK_VERSION_MAJOR,
      .minor = CYCLEIQ_SDK_VERSION_MINOR,
      .patch = CYCLEIQ_SDK_VERSION_PATCH,
  };
  return version;
}

cycleiq_version_t cycleiq_protocol_version(void) {
  cycleiq_version_t version = {
      .major = CYCLEIQ_PROTOCOL_VERSION_MAJOR,
      .minor = CYCLEIQ_PROTOCOL_VERSION_MINOR,
      .patch = CYCLEIQ_PROTOCOL_VERSION_PATCH,
  };
  return version;
}

cycleiq_version_support_t
cycleiq_protocol_support_status(cycleiq_version_t local,
                                cycleiq_version_t remote) {
  if (local.major != remote.major) {
    return CYCLEIQ_VERSION_UNSUPPORTED;
  }

  if (local.minor < remote.minor) {
    return CYCLEIQ_VERSION_PARTIALLY_SUPPORTED;
  }

  return CYCLEIQ_VERSION_SUPPORTED;
}

bool cycleiq_protocol_is_compatible(cycleiq_version_t local,
                                    cycleiq_version_t remote) {
  return cycleiq_protocol_support_status(local, remote) !=
         CYCLEIQ_VERSION_UNSUPPORTED;
}

void cycleiq_frame_init(cycleiq_frame_t *frame, uint8_t destination_node_id,
                        uint8_t type_or_command) {
  if (frame == NULL) {
    return;
  }

  frame->id = CYCLEIQ_CAN_FRAME_ID(destination_node_id, type_or_command);
  frame->len = 0;
  memset(frame->data, 0, sizeof(frame->data));
}

bool cycleiq_frame_is_for_node(const cycleiq_frame_t *frame, uint8_t node_id) {
  return frame != NULL && CYCLEIQ_CAN_NODE_ID(frame->id) == node_id;
}

bool cycleiq_frame_from_can(cycleiq_frame_t *frame, uint32_t id,
                            const uint8_t *data, uint8_t len) {
  if (frame == NULL || (data == NULL && len > 0u) ||
      len > CYCLEIQ_CAN_MAX_PAYLOAD_LEN) {
    return false;
  }

  frame->id = id;
  frame->len = len;
  memset(frame->data, 0, sizeof(frame->data));
  if (len > 0u) {
    memcpy(frame->data, data, len);
  }

  return true;
}

uint8_t cycleiq_frame_type(const cycleiq_frame_t *frame) {
  if (frame == NULL) {
    return 0;
  }

  return (uint8_t)CYCLEIQ_CAN_PACKET_TYPE(frame->id);
}

bool cycleiq_power_off(cycleiq_frame_t *frame) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_POWER_OFF);
  return cycleiq_set_len(frame, 0);
}

bool cycleiq_power_on(cycleiq_frame_t *frame) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_POWER_ON);
  return cycleiq_set_len(frame, 0);
}

bool cycleiq_set_gear(cycleiq_frame_t *frame, uint8_t gear) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_GEAR_SET);
  if (!cycleiq_set_len(frame, 1)) {
    return false;
  }

  frame->data[0] = gear;
  return true;
}

bool cycleiq_set_support_mode(cycleiq_frame_t *frame,
                              cycleiq_support_mode_t mode) {
  if (!cycleiq_valid_support_mode(mode)) {
    return false;
  }

  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_MODE_SET);
  if (!cycleiq_set_len(frame, 1)) {
    return false;
  }

  frame->data[0] = (uint8_t)mode;
  return true;
}

bool cycleiq_set_ride_mode(cycleiq_frame_t *frame, cycleiq_ride_mode_t mode) {
  if (!cycleiq_valid_ride_mode(mode)) {
    return false;
  }

  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_RIDE_MODE_SET);
  if (!cycleiq_set_len(frame, 1)) {
    return false;
  }

  frame->data[0] = (uint8_t)mode;
  return true;
}

bool cycleiq_set_screen(cycleiq_frame_t *frame, cycleiq_screen_t screen) {
  if (!cycleiq_valid_screen(screen)) {
    return false;
  }

  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_SCREEN_SET);
  if (!cycleiq_set_len(frame, 1)) {
    return false;
  }

  frame->data[0] = (uint8_t)screen;
  return true;
}

bool cycleiq_set_walk_mode(cycleiq_frame_t *frame, bool enabled) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_WALK_SET);
  if (!cycleiq_set_len(frame, CYCLEIQ_COMMAND_WALK_SET_LEN)) {
    return false;
  }

  frame->data[0] = enabled ? 1u : 0u;
  return true;
}

bool cycleiq_get_protocol_version(cycleiq_frame_t *frame) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_PROTOCOL_VERSION_GET);
  return cycleiq_set_len(frame, 0);
}

bool cycleiq_get_config(cycleiq_frame_t *frame, cycleiq_config_field_t field) {
  if (!cycleiq_valid_config_field(field)) {
    return false;
  }

  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_CONFIG_GET);
  if (!cycleiq_set_len(frame, CYCLEIQ_COMMAND_CONFIG_GET_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)field;
  return true;
}

bool cycleiq_set_config_field(cycleiq_frame_t *frame,
                              cycleiq_config_field_t field, uint16_t value) {
  if (field == CYCLEIQ_CONFIG_FIELD_ALL || !cycleiq_valid_config_field(field)) {
    return false;
  }

  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_CONFIG_SET);
  if (!cycleiq_set_len(frame, CYCLEIQ_COMMAND_CONFIG_SET_FIELD_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)CYCLEIQ_CONFIG_OP_SET_FIELD;
  frame->data[1] = (uint8_t)field;
  cycleiq_write_be_u16(&frame->data[2], value);
  return true;
}

bool cycleiq_set_config_snapshot(cycleiq_frame_t *frame,
                                 const cycleiq_config_snapshot_t *snapshot) {
  if (snapshot == NULL) {
    return false;
  }

  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_CONFIG_SET);
  if (!cycleiq_set_len(frame, CYCLEIQ_COMMAND_CONFIG_SET_SNAPSHOT_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)CYCLEIQ_CONFIG_OP_SET_SNAPSHOT;
  cycleiq_write_be_u16(&frame->data[1], snapshot->max_speed_ckph);
  cycleiq_write_be_u16(&frame->data[3], snapshot->battery_resistance_mohm);
  cycleiq_write_be_u16(&frame->data[5], snapshot->wheel_diameter_mm);
  return true;
}

bool cycleiq_commit_config(cycleiq_frame_t *frame) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_CONFIG_SET);
  if (!cycleiq_set_len(frame, CYCLEIQ_COMMAND_CONFIG_SET_OP_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)CYCLEIQ_CONFIG_OP_COMMIT;
  return true;
}

bool cycleiq_discard_config(cycleiq_frame_t *frame) {
  cycleiq_frame_init(frame, CYCLEIQ_CAN_ID, CYCLEIQ_COMM_CONFIG_SET);
  if (!cycleiq_set_len(frame, CYCLEIQ_COMMAND_CONFIG_SET_OP_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)CYCLEIQ_CONFIG_OP_DISCARD;
  return true;
}

bool cycleiq_telemetry_live_status(cycleiq_frame_t *frame, float speed_mps,
                                   uint16_t power_w) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_LIVE_STATUS);
  if (!cycleiq_set_len(frame, PEAK_PACKET_LIVE_STATUS_LEN)) {
    return false;
  }

  cycleiq_write_be_u16(&frame->data[0],
                       cycleiq_float_to_u16(speed_mps,
                                            CYCLEIQ_SCALE_SPEED_MPS_TO_CKMH));
  cycleiq_write_be_u16(&frame->data[2], power_w);
  return true;
}

bool cycleiq_telemetry_battery_status(cycleiq_frame_t *frame,
                                      uint8_t battery_pct, float voltage_v,
                                      float current_a) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_BATTERY_STATUS);
  if (!cycleiq_set_len(frame, PEAK_PACKET_BATTERY_STATUS_LEN)) {
    return false;
  }

  if (battery_pct > 100u) {
    battery_pct = 100u;
  }

  frame->data[0] = battery_pct;
  cycleiq_write_be_u16(
      &frame->data[1],
      cycleiq_float_to_u16(voltage_v, CYCLEIQ_SCALE_VOLTS_TO_CV));
  cycleiq_write_be_i16(
      &frame->data[3],
      cycleiq_float_to_i16(current_a, CYCLEIQ_SCALE_AMPS_TO_CA));
  return true;
}

bool cycleiq_telemetry_motor_status(cycleiq_frame_t *frame,
                                    int8_t motor_temperature_c,
                                    int8_t controller_temperature_c,
                                    float motor_current_a, float motor_rpm) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_MOTOR_STATUS);
  if (!cycleiq_set_len(frame, PEAK_PACKET_MOTOR_STATUS_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)motor_temperature_c;
  frame->data[1] = (uint8_t)controller_temperature_c;
  cycleiq_write_be_i16(
      &frame->data[2],
      cycleiq_float_to_i16(motor_current_a, CYCLEIQ_SCALE_AMPS_TO_CA));
  cycleiq_write_be_u16(&frame->data[4],
                       cycleiq_float_to_u16(motor_rpm, 1.0f));
  return true;
}

bool cycleiq_telemetry_controller_state(cycleiq_frame_t *frame, uint8_t gear,
                                        cycleiq_support_mode_t mode,
                                        cycleiq_ride_mode_t ride_mode) {
  if (!cycleiq_valid_support_mode(mode) || !cycleiq_valid_ride_mode(ride_mode)) {
    return false;
  }

  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_CONTROLLER_STATE);
  if (!cycleiq_set_len(frame, PEAK_PACKET_CONTROLLER_STATE_LEN)) {
    return false;
  }

  frame->data[0] = gear;
  frame->data[1] = (uint8_t)mode;
  frame->data[2] = (uint8_t)ride_mode;
  return true;
}

bool cycleiq_telemetry_battery_energy(cycleiq_frame_t *frame, float watt_hours,
                                      float amp_hours) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_BATTERY_ENERGY);
  if (!cycleiq_set_len(frame, PEAK_PACKET_BATTERY_ENERGY_LEN)) {
    return false;
  }

  cycleiq_write_be_u16(
      &frame->data[0],
      cycleiq_float_to_u16(watt_hours, CYCLEIQ_SCALE_WH_TO_DWH));
  cycleiq_write_be_u16(
      &frame->data[2], cycleiq_float_to_u16(amp_hours, CYCLEIQ_SCALE_AH_TO_CAH));
  return true;
}

bool cycleiq_telemetry_trip_primary(cycleiq_frame_t *frame,
                                    float trip_distance_km,
                                    uint32_t trip_time_s) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_TRIP_PRIMARY);
  if (!cycleiq_set_len(frame, PEAK_PACKET_TRIP_PRIMARY_LEN)) {
    return false;
  }

  cycleiq_write_be_u32(
      &frame->data[0],
      cycleiq_float_to_u32(trip_distance_km, CYCLEIQ_SCALE_KM_TO_M));
  cycleiq_write_be_u32(&frame->data[4], trip_time_s);
  return true;
}

bool cycleiq_telemetry_trip_secondary(cycleiq_frame_t *frame,
                                      float average_speed_mps,
                                      float range_km) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_TRIP_SECONDARY);
  if (!cycleiq_set_len(frame, PEAK_PACKET_TRIP_SECONDARY_LEN)) {
    return false;
  }

  cycleiq_write_be_u16(
      &frame->data[0],
      cycleiq_float_to_u16(average_speed_mps, CYCLEIQ_SCALE_SPEED_MPS_TO_CKMH));
  frame->data[2] = cycleiq_float_to_u8(range_km, 1.0f);
  return true;
}

bool cycleiq_telemetry_protocol_version(cycleiq_frame_t *frame) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_PROTOCOL_VERSION);
  if (!cycleiq_set_len(frame, PEAK_PACKET_PROTOCOL_VERSION_LEN)) {
    return false;
  }

  cycleiq_version_t protocol = cycleiq_protocol_version();
  cycleiq_version_t sdk = cycleiq_sdk_version();
  frame->data[0] = protocol.major;
  frame->data[1] = protocol.minor;
  frame->data[2] = protocol.patch;
  frame->data[3] = sdk.major;
  frame->data[4] = sdk.minor;
  frame->data[5] = sdk.patch;
  return true;
}

bool cycleiq_telemetry_config_field(cycleiq_frame_t *frame,
                                    cycleiq_config_field_t field,
                                    uint16_t value) {
  if (field == CYCLEIQ_CONFIG_FIELD_ALL || !cycleiq_valid_config_field(field)) {
    return false;
  }

  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_CONFIG_FIELD);
  if (!cycleiq_set_len(frame, PEAK_PACKET_CONFIG_FIELD_LEN)) {
    return false;
  }

  frame->data[0] = (uint8_t)field;
  cycleiq_write_be_u16(&frame->data[1], value);
  return true;
}

bool cycleiq_telemetry_config_snapshot(
    cycleiq_frame_t *frame, const cycleiq_config_snapshot_t *snapshot) {
  if (snapshot == NULL) {
    return false;
  }

  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_CONFIG_SNAPSHOT);
  if (!cycleiq_set_len(frame, PEAK_PACKET_CONFIG_SNAPSHOT_LEN)) {
    return false;
  }

  cycleiq_write_be_u16(&frame->data[0], snapshot->max_speed_ckph);
  cycleiq_write_be_u16(&frame->data[2], snapshot->battery_resistance_mohm);
  cycleiq_write_be_u16(&frame->data[4], snapshot->wheel_diameter_mm);
  return true;
}

bool cycleiq_telemetry_config_ack(cycleiq_frame_t *frame, uint8_t command,
                                  cycleiq_config_status_t status,
                                  cycleiq_config_field_t detail) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_CONFIG_ACK);
  if (!cycleiq_set_len(frame, PEAK_PACKET_CONFIG_ACK_LEN)) {
    return false;
  }

  frame->data[0] = command;
  frame->data[1] = (uint8_t)status;
  frame->data[2] = (uint8_t)detail;
  return true;
}

bool cycleiq_telemetry_walk_state(cycleiq_frame_t *frame, bool active) {
  cycleiq_frame_init(frame, PEAK_CAN_ID, PEAK_PACKET_TYPE_WALK_STATE);
  if (!cycleiq_set_len(frame, PEAK_PACKET_WALK_STATE_LEN)) {
    return false;
  }

  frame->data[0] = active ? 1u : 0u;
  return true;
}

bool cycleiq_command_read_u8(const cycleiq_frame_t *frame, uint8_t *value) {
  if (frame == NULL || value == NULL || frame->len < 1u) {
    return false;
  }

  *value = frame->data[0];
  return true;
}

bool cycleiq_command_read_walk_mode(const cycleiq_frame_t *frame,
                                    bool *enabled) {
  if (frame == NULL || enabled == NULL ||
      cycleiq_frame_type(frame) != CYCLEIQ_COMM_WALK_SET ||
      frame->len != CYCLEIQ_COMMAND_WALK_SET_LEN || frame->data[0] > 1u) {
    return false;
  }

  *enabled = frame->data[0] == 1u;
  return true;
}

bool cycleiq_command_read_config_get(const cycleiq_frame_t *frame,
                                     cycleiq_config_field_t *field) {
  if (frame == NULL || field == NULL ||
      frame->len != CYCLEIQ_COMMAND_CONFIG_GET_LEN) {
    return false;
  }

  *field = (cycleiq_config_field_t)frame->data[0];
  return cycleiq_valid_config_field(*field);
}

bool cycleiq_command_read_config_op(const cycleiq_frame_t *frame,
                                    cycleiq_config_op_t *op) {
  if (frame == NULL || op == NULL ||
      frame->len < CYCLEIQ_COMMAND_CONFIG_SET_OP_LEN) {
    return false;
  }

  *op = (cycleiq_config_op_t)frame->data[0];
  return cycleiq_valid_config_op(*op);
}

bool cycleiq_command_read_config_field(
    const cycleiq_frame_t *frame, cycleiq_config_field_t *field,
    uint16_t *value) {
  if (frame == NULL || field == NULL || value == NULL ||
      frame->len != CYCLEIQ_COMMAND_CONFIG_SET_FIELD_LEN ||
      frame->data[0] != (uint8_t)CYCLEIQ_CONFIG_OP_SET_FIELD) {
    return false;
  }

  *field = (cycleiq_config_field_t)frame->data[1];
  *value = cycleiq_read_be_u16(&frame->data[2]);
  return *field != CYCLEIQ_CONFIG_FIELD_ALL &&
         cycleiq_valid_config_field(*field);
}

bool cycleiq_command_read_config_snapshot(
    const cycleiq_frame_t *frame, cycleiq_config_snapshot_t *snapshot) {
  if (frame == NULL || snapshot == NULL ||
      frame->len != CYCLEIQ_COMMAND_CONFIG_SET_SNAPSHOT_LEN ||
      frame->data[0] != (uint8_t)CYCLEIQ_CONFIG_OP_SET_SNAPSHOT) {
    return false;
  }

  snapshot->max_speed_ckph = cycleiq_read_be_u16(&frame->data[1]);
  snapshot->battery_resistance_mohm = cycleiq_read_be_u16(&frame->data[3]);
  snapshot->wheel_diameter_mm = cycleiq_read_be_u16(&frame->data[5]);
  return true;
}

bool cycleiq_read_protocol_version(const cycleiq_frame_t *frame,
                                   cycleiq_version_t *protocol_version,
                                   cycleiq_version_t *sdk_version) {
  if (frame == NULL || protocol_version == NULL || sdk_version == NULL ||
      cycleiq_frame_type(frame) != PEAK_PACKET_TYPE_PROTOCOL_VERSION ||
      frame->len < PEAK_PACKET_PROTOCOL_VERSION_LEN) {
    return false;
  }

  protocol_version->major = frame->data[0];
  protocol_version->minor = frame->data[1];
  protocol_version->patch = frame->data[2];
  sdk_version->major = frame->data[3];
  sdk_version->minor = frame->data[4];
  sdk_version->patch = frame->data[5];
  return true;
}

bool cycleiq_read_config_field(const cycleiq_frame_t *frame,
                               cycleiq_config_field_t *field,
                               uint16_t *value) {
  if (frame == NULL || field == NULL || value == NULL ||
      cycleiq_frame_type(frame) != PEAK_PACKET_TYPE_CONFIG_FIELD ||
      frame->len < PEAK_PACKET_CONFIG_FIELD_LEN) {
    return false;
  }

  *field = (cycleiq_config_field_t)frame->data[0];
  *value = cycleiq_read_be_u16(&frame->data[1]);
  return *field != CYCLEIQ_CONFIG_FIELD_ALL &&
         cycleiq_valid_config_field(*field);
}

bool cycleiq_read_config_snapshot(const cycleiq_frame_t *frame,
                                  cycleiq_config_snapshot_t *snapshot) {
  if (frame == NULL || snapshot == NULL ||
      cycleiq_frame_type(frame) != PEAK_PACKET_TYPE_CONFIG_SNAPSHOT ||
      frame->len < PEAK_PACKET_CONFIG_SNAPSHOT_LEN) {
    return false;
  }

  snapshot->max_speed_ckph = cycleiq_read_be_u16(&frame->data[0]);
  snapshot->battery_resistance_mohm = cycleiq_read_be_u16(&frame->data[2]);
  snapshot->wheel_diameter_mm = cycleiq_read_be_u16(&frame->data[4]);
  return true;
}

bool cycleiq_read_config_ack(const cycleiq_frame_t *frame, uint8_t *command,
                             cycleiq_config_status_t *status,
                             cycleiq_config_field_t *detail) {
  if (frame == NULL || command == NULL || status == NULL || detail == NULL ||
      cycleiq_frame_type(frame) != PEAK_PACKET_TYPE_CONFIG_ACK ||
      frame->len < PEAK_PACKET_CONFIG_ACK_LEN) {
    return false;
  }

  *command = frame->data[0];
  *status = (cycleiq_config_status_t)frame->data[1];
  *detail = (cycleiq_config_field_t)frame->data[2];
  return true;
}

bool cycleiq_read_walk_state(const cycleiq_frame_t *frame, bool *active) {
  if (frame == NULL || active == NULL ||
      cycleiq_frame_type(frame) != PEAK_PACKET_TYPE_WALK_STATE ||
      frame->len != PEAK_PACKET_WALK_STATE_LEN || frame->data[0] > 1u) {
    return false;
  }

  *active = frame->data[0] == 1u;
  return true;
}
