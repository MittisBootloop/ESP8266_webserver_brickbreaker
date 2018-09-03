# ESP8266_webserver_brickbreaker
Brick breaker clone on SSD1331 OLED- controlled over webpage

Connect the SSD1331 Display to the ESP

 | Display | ESP8266 |
 | --- | --- |
 | SCL/SCKL | GPIO14 |
 | SDA/MOSI | GPIO13 |
 | RES/RST  | GPIO15 |
 | DC       | GPIO5 |
 | CS       | GPIO4 |
 | GND      | GND/G |
 | VCC      | 3/3.3V |
 (the display i used isn't compatible with 5V!)
  
Upload the sketch. Then connect to the ESPs wifi and open in the browser the adress/ip
2.2.2.2 - now you can control the game via the "buttons".
If you've uncommented the AdviceOrientation part before uploding, you can also control
via turning/tilting a mobile device(thanks to Tobozo).
