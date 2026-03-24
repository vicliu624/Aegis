# Info App Example

This example corresponds to the runtime-built `demo_info` app in `/apps/demo_info`.

It demonstrates:

- capability-aware behavior without board-name branching
- typed service clients for UI, radio, GPS, settings, notification, storage, power, and time
- Host API ABI usage through `aegis_host_api_v1`
- session-safe UI ownership and shell return after app teardown
