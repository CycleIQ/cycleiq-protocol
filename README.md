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
`cycleiq_protocol_support_status()` before enabling ride commands.

Compatibility policy:

- `CYCLEIQ_VERSION_SUPPORTED`: same major and local minor is greater than or
  equal to remote minor.
- `CYCLEIQ_VERSION_PARTIALLY_SUPPORTED`: same major but remote minor is newer;
  stable existing packets are safe, but newer features may be missing locally.
- `CYCLEIQ_VERSION_UNSUPPORTED`: different major; do not send ride commands.

## Walk Mode

Walk Mode is a hold-to-run command. On button press, the display sends
`cycleiq_set_walk_mode(..., true)` immediately and repeats it every 250 ms while
the button remains held. It sends `cycleiq_set_walk_mode(..., false)` immediately
on release or input cancellation.

The ESC expires Walk Mode when no valid ON command has arrived for 1000 ms, so a
display reset or interrupted CAN connection cannot leave walk propulsion active.
The display should parse `PEAK_PACKET_TYPE_WALK_STATE` with
`cycleiq_read_walk_state()` and use the confirmed ESC state for its UI indicator.

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

Run the host protocol tests with:

```sh
make -C tests run
```
