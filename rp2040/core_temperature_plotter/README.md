# core_temperature_plotter

Reads the RP2040's built-in temperature sensor and prints the result to the Serial Monitor (and Serial Plotter) once per second.

Uses the `analogReadTemp()` function from the RP2040 Arduino core — no external sensor or library is needed.

## Board selection

In Arduino IDE select **Raspberry Pi Pico** as the target board.

## Serial output

Open the Serial Monitor or Serial Plotter at **115200 baud**.

Example output:
```
Core temperature: 27.3C
Core temperature: 27.4C
```

## Notes

The internal sensor has limited accuracy (±5 °C typical). It is useful for relative measurements and detecting thermal trends rather than precise absolute readings.
