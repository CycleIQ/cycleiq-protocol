# cycleIQ Protocol

Shared C SDK for the cycleIQ ESC firmware and display firmware.

The SDK is transport-neutral: it builds and parses `cycleiq_frame_t` values that
contain the extended CAN ID, payload length, and payload bytes. Each platform is
responsible for sending or receiving those frames through its own CAN driver.

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
