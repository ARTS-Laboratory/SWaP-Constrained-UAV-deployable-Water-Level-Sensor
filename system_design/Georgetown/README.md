# Gerogetown
UAV deployable sensor package for measuring water height. 

## V0.1
1. Oldest version
2. Basic datalogger setup with modular RTC and SD card
3. Accelerometer integrated into daughter board

## V0.2
1. Teensy 4.0 microcontroller
2. Added environmental sensor

## V0.3
1. Teensy 4.0 or Arduino Nano microcontroller
2. Added DC current sensor

## V0.4
1. Arduino Nano microcontroller
2. Magnet power control circuit on daughter board
3. No accelerometer on daughter board
4. Solar panel load sharing on daughter board

## V0.5
1. Arduino Nano microcontroller
2. Uses permanent magnet to attach to steel structures
3. Easy sliding water-proof clear tube

## V0.6 (Proposed)
### Power electronics upgrade from V0.5
1. Arduino Nano replaced with ATMEGA 328p SMD chip / USB driver removed from MCU
2. Uses dynamic sleep interval depending on battery voltage level
3. Uses [buck converter](https://www.mouser.com/ProductDetail/Texas-Instruments/TPSM84203EAB?qs=EU6FO9ffTwdvD6IyQBtt5A%3D%3D&mgh=1&utm_id=22370133501&utm_source=google&utm_medium=cpc&utm_marketing_tactic=amercorp&gad_source=1&gad_campaignid=22380187564&gbraid=0AAAAADn_wf2gWZzSurQARsbyVMmz_4UDQ&gclid=Cj0KCQjw4qHEBhCDARIsALYKFNO-ZgKxWQY3H1w5GanZ_Tstxw3fUVWm0au9QuEBnVr6LlNByRsZf7YaAo9nEALw_wcB) instead of linear voltage regulator
4. Uses module that has [single ultra sound tranciever](https://cdn.shopify.com/s/files/1/0550/8091/0899/files/11832.pdf?v=1726578772) to lower current consumption.
























