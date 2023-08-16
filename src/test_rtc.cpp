// #include <Arduino.h>
// #include <DS1302.h>

// DS1302 rtc(4, 5, 6);
// void setup() {
//     // estado incierto en start up
//     rtc.halt(false);
//     rtc.writeProtect(false);

//     Serial.begin(9600);

//     // Configurar hora
//     rtc.setDOW(WEDNESDAY);        // Set Day-of-Week
//     rtc.setTime(12, 20, 0);     // 24hr format
//     rtc.setDate(9, 8, 2023);   // Dd-Mm-YYYY
// }

// void loop() {
//     // Send Day-of-Week
//     Serial.print(rtc.getDOWStr());
//     Serial.print(" ");

//     // Send date
//     Serial.print(rtc.getDateStr());
//     Serial.print(" -- ");

//     // Send time
//     Serial.println(rtc.getTimeStr());

//     // Wait one second before repeating :)
//     delay(1000);
// }