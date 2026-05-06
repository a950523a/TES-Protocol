# TES-Protocol

Portable C99 CAN codec and types for the **TES-0D-02-01** charging protocol used by Taiwan electric scooters (e.g., eMoving iE125).

Zero platform dependencies — only `<stdint.h>` and `<stdbool.h>`. Compiles on ESP32, STM32, or any C99 toolchain.

## Contents

| File | Description |
|------|-------------|
| `include/tes_protocol/tes_types.h` | All shared types: CAN message structs, state enums, snapshot |
| `tes_codec.c / tes_codec.h` | CAN frame encode/decode for both directions |

## CAN Message IDs

| ID | Direction | Content |
|----|-----------|---------|
| 0x500 | Vehicle → Charger | Status flags, requested current/voltage |
| 0x501 | Vehicle → Charger | SOC, max charge time, ETA |
| 0x5F0 | Vehicle → Charger | Emergency flags |
| 0x508 | Charger → Vehicle | Charger status, available voltage/current |
| 0x509 | Charger → Vehicle | Actual output voltage/current, remaining time |
| 0x5F8 | Charger → Vehicle | Emergency stop |

## Usage

### ESP-IDF component

Add as a git submodule in your `components/` directory:

```bash
git submodule add https://github.com/a950523a/TES-Protocol firmware/components/tes_protocol
```

In your component's `CMakeLists.txt`:
```cmake
idf_component_register(
    ...
    REQUIRES tes_protocol
)
```

### STM32 / bare-metal CMake

```cmake
add_subdirectory(tes_protocol)
target_link_libraries(your_target PRIVATE tes_protocol)
```

## Related projects

- [TES ESP32 Charging Controller](https://github.com/a950523a/TES-Taiwan-Electric-Scooter-Charging-Controller) — charger-side firmware (ESP32-S3)

## License

CC BY-NC-SA 4.0
