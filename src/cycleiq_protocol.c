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

static bool cycleiq_set_len(cycleiq_frame_t *frame, uint8_t len) {
  if (frame == NULL || len > CYCLEIQ_CAN_MAX_PAYLOAD_LEN) {
    return false;
  }

  frame->len = len;
  return true;
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

bool cycleiq_command_read_u8(const cycleiq_frame_t *frame, uint8_t *value) {
  if (frame == NULL || value == NULL || frame->len < 1u) {
    return false;
  }

  *value = frame->data[0];
  return true;
}
