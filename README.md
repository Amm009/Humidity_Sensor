This is a humidity sensor I designed to be installed in the walls of tubs holding 3D printing filament.

The ESP32 wakes from sleep when the touchpad is touched and displays the current humidity and battery level, and then the past humidity level over time with a graph as well.

It can also connect to the internet and be used with MQTT. It reports humidity and battery levels.

AI was used in the development in this code.

Components:
ESP32 (https://www.amazon.com/dp/B0D8T53CQ5) <br/>
AA Battery Tabs (https://www.amazon.com/dp/B077GL8PPJ) <br/>
4 x M3x4x5 Threaded Inserts (https://www.amazon.com/dp/B0F2N8QY45) <br/>
4 x M3x16 Screws (https://www.amazon.com/dp/B0D3X5CT2J) <br/>
SHT31-D Humidity Sensor (https://www.amazon.com/dp/B07ZSZW92J) <br/>
2 x AO3401 P-Channel Mosfet (https://www.amazon.com/dp/B08RHFLH1K) <br/>
2 x AO3400A N-Channel Mosfet (https://www.amazon.com/dp/B0DZHXWCTK) <br/>
2 x 4.7k, 2 x 10k, 2 x 100k SMD Resistors (https://www.amazon.com/dp/B08RYNWTT9) <br/>
1 x Power Switch (https://www.amazon.com/dp/B0DN69L9SG) <br/>
SSD1306 OLED Screen (https://www.amazon.com/dp/B0D2RMQQHR) <br/>
2.54mm Pin Headers (https://www.amazon.com/Ferwooh-Single-Straight-Connector-Spacing/dp/B0CZ6X313F?s=industrial&sr=1-6) <br/>
Buzzer (With the current PCB design, it is inadvisable to use the buzzer. It would also be really annoying if the humidity gets too high with the current code) (https://www.amazon.com/Electromagnetic-Piezo-Buzzer-16ohm-Passive/dp/B097RNHZN4?sr=8-10)
4 x Rechargeable AA's (https://www.amazon.com/EBL-Battery-Rechargeable-Batteries-Foldable/dp/B0FWK97R4W?sr=8-9) <br/>
JST-PH Connectors (https://www.amazon.com/Connector-JST-XH-Connectors-Ratcheting-Crimping/dp/B0CBHL8CMC?sr=8-18) <br/>
