#ifndef CYCLEIQ_PROTOCOL_H
#define CYCLEIQ_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Shared CycleIQ ESC/display protocol SDK.
 *
 * Transport:
 * - Classic CAN, extended identifiers, max payload 8 bytes.
 * - The high CAN-ID byte is the destination node.
 * - The low CAN-ID byte is the command or telemetry packet type.
 * - Multi-byte integer payload fields are big-endian.
 *
 * Node roles:
 * - PEAK_CAN_ID is the display node. The ESC sends telemetry there.
 * - CYCLEIQ_CAN_ID is the ESC node. The display sends commands there.
 */

#define CYCLEIQ_CAN_MAX_PAYLOAD_LEN 8u

#define PEAK_CAN_ID 0x6Au
#define CYCLEIQ_CAN_ID 0x6Bu

#define CYCLEIQ_CAN_FRAME_ID(node_id, type_or_command)                         \
  (((uint32_t)(node_id) << 8) | (uint8_t)(type_or_command))

#define CYCLEIQ_CAN_NODE_ID(can_id) (((can_id) >> 8) & 0xFFu)
#define CYCLEIQ_CAN_PACKET_TYPE(can_id) ((can_id) & 0xFFu)

typedef enum {
  CYCLEIQ_POWER_OFF = 0x00u,
  CYCLEIQ_POWER_ON = 0x01u,
  CYCLEIQ_COMM_GEAR_SET = 0x02u,
  CYCLEIQ_COMM_MODE_SET = 0x03u,
  CYCLEIQ_COMM_RIDE_MODE_SET = 0x04u,
  CYCLEIQ_COMM_SCREEN_SET = 0x05u,
  CYCLEIQ_COMM_CONFIG_GET = 0x06u,
  CYCLEIQ_COMM_CONFIG_SET = 0x07u,
} cycleiq_command_t;

typedef enum {
  CYCLEIQ_MODE_PAS = 0,
  CYCLEIQ_MODE_TORQUE = 1,
  CYCLEIQ_MODE_HYBRID = 2,
} cycleiq_support_mode_t;

typedef enum {
  CYCLEIQ_RIDE_MODE_NORMAL = 0,
  CYCLEIQ_RIDE_MODE_MOUNTAIN = 1,
} cycleiq_ride_mode_t;

typedef enum {
  CYCLEIQ_SCREEN_MAIN = 0x00u,
  CYCLEIQ_SCREEN_TRIP = 0x01u,
  CYCLEIQ_SCREEN_SYSTEM_INFO = 0x02u,
  CYCLEIQ_SCREEN_GRAPH = 0x03u,
} cycleiq_screen_t;

typedef enum {
  PEAK_PACKET_TYPE_BATTERY_STATUS = 0x10u,
  PEAK_PACKET_TYPE_BATTERY_ENERGY = 0x11u,
  PEAK_PACKET_TYPE_MOTOR_STATUS = 0x12u,
  PEAK_PACKET_TYPE_CONTROLLER_STATE = 0x13u,
  PEAK_PACKET_TYPE_LIVE_STATUS = 0x14u,
  PEAK_PACKET_TYPE_TRIP_PRIMARY = 0x15u,
  PEAK_PACKET_TYPE_TRIP_SECONDARY = 0x16u,
} peak_packet_type_t;

#define PEAK_PACKET_BATTERY_STATUS_LEN 5u
#define PEAK_PACKET_BATTERY_ENERGY_LEN 4u
#define PEAK_PACKET_MOTOR_STATUS_LEN 6u
#define PEAK_PACKET_CONTROLLER_STATE_LEN 3u
#define PEAK_PACKET_LIVE_STATUS_LEN 4u
#define PEAK_PACKET_TRIP_PRIMARY_LEN 8u
#define PEAK_PACKET_TRIP_SECONDARY_LEN 3u

#define CYCLEIQ_SCALE_SPEED_MPS_TO_CKMH 360.0f
#define CYCLEIQ_SCALE_VOLTS_TO_CV 100.0f
#define CYCLEIQ_SCALE_AMPS_TO_CA 100.0f
#define CYCLEIQ_SCALE_WH_TO_DWH 10.0f
#define CYCLEIQ_SCALE_AH_TO_CAH 100.0f
#define CYCLEIQ_SCALE_KM_TO_M 1000.0f

typedef struct {
  uint32_t id;
  uint8_t len;
  uint8_t data[CYCLEIQ_CAN_MAX_PAYLOAD_LEN];
} cycleiq_frame_t;

void cycleiq_frame_init(cycleiq_frame_t *frame, uint8_t destination_node_id,
                        uint8_t type_or_command);
bool cycleiq_frame_from_can(cycleiq_frame_t *frame, uint32_t id,
                            const uint8_t *data, uint8_t len);
bool cycleiq_frame_is_for_node(const cycleiq_frame_t *frame, uint8_t node_id);
uint8_t cycleiq_frame_type(const cycleiq_frame_t *frame);

bool cycleiq_power_off(cycleiq_frame_t *frame);
bool cycleiq_power_on(cycleiq_frame_t *frame);
bool cycleiq_set_gear(cycleiq_frame_t *frame, uint8_t gear);
bool cycleiq_set_support_mode(cycleiq_frame_t *frame,
                              cycleiq_support_mode_t mode);
bool cycleiq_set_ride_mode(cycleiq_frame_t *frame, cycleiq_ride_mode_t mode);
bool cycleiq_set_screen(cycleiq_frame_t *frame, cycleiq_screen_t screen);

bool cycleiq_telemetry_live_status(cycleiq_frame_t *frame, float speed_mps,
                                   uint16_t power_w);
bool cycleiq_telemetry_battery_status(cycleiq_frame_t *frame,
                                      uint8_t battery_pct, float voltage_v,
                                      float current_a);
bool cycleiq_telemetry_motor_status(cycleiq_frame_t *frame,
                                    int8_t motor_temperature_c,
                                    int8_t controller_temperature_c,
                                    float motor_current_a, float motor_rpm);
bool cycleiq_telemetry_controller_state(cycleiq_frame_t *frame, uint8_t gear,
                                        cycleiq_support_mode_t mode,
                                        cycleiq_ride_mode_t ride_mode);
bool cycleiq_telemetry_battery_energy(cycleiq_frame_t *frame, float watt_hours,
                                      float amp_hours);
bool cycleiq_telemetry_trip_primary(cycleiq_frame_t *frame,
                                    float trip_distance_km,
                                    uint32_t trip_time_s);
bool cycleiq_telemetry_trip_secondary(cycleiq_frame_t *frame,
                                      float average_speed_mps,
                                      float range_km);

bool cycleiq_command_read_u8(const cycleiq_frame_t *frame, uint8_t *value);

#ifdef __cplusplus
}
#endif

#endif
