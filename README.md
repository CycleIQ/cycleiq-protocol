# cycleIQ Protocol

Shared C SDK for the cycleIQ ESC firmware and display firmware.

The SDK is transport-neutral: it builds and parses `cycleiq_frame_t` values that
contain the extended CAN ID, payload length, and payload bytes. Each platform is
responsible for sending or receiving those frames through its own CAN driver.

## Versioning

The SDK exposes two versions:

- `CYCLEIQ_SDK_VERSION_*`: the C SDK implementation version.
- `CYCLEIQ_PROTOCOL_VERSION_*`: the wire protocol version.

The display should call `cycleiq_get_protocol_version()`, wait for a
`PEAK_PACKET_TYPE_PROTOCOL_VERSION` response, parse it with
`cycleiq_read_protocol_version()`, and compare versions with
`cycleiq_protocol_is_compatible()` before enabling ride commands.

## ESP-IDF

Use this repository as an ESP-IDF component. The root `CMakeLists.txt` registers
`src/cycleiq_protocol.c` and exports `include/`.

## VESC firmware

Include `cycleiq_protocol.mk` from the firmware makefile, or add:

```make
APPSRC += external/cycleiq-protocol/src/cycleiq_protocol.c
APPINC += external/cycleiq-protocol/include
```

## Example

```c
cycleiq_frame_t frame;
if (cycleiq_set_gear(&frame, 3)) {
  can_send(frame.id, frame.data, frame.len);
}
```
