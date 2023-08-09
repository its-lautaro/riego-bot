#include <Arduino.h>
#include <DS1302.h>

#define ESTACION_1 3
#define ESTACION_2 4

#define RST_PIN 5
#define DATA_PIN 6
#define CLK_PIN 7
DS1302 rtc(5, 6, 7);

bool timeOut = false;
unsigned long termination;

// Programa de riego
typedef struct {
    // (D, L, M, X, J, V, S)
    int dows[7] = { 1,0,1,1,1,0,1 };
    // Formato 24hr
    int hr = 12, min = 45;

    int months[12] = { 1,1,1,1,1,1,1,1,1,1,1,1 };
    // Minutos
    int duracion = 1;
} Programa;
Programa prog;

Time getTime() {
    return rtc.getTime();
}

void setTime(DS1302 rtc, int dow, int hr, int min, int sec, int date, int mon, int yr) {
    rtc.setDOW(dow);        // Set Day-of-Week
    rtc.setTime(hr, min, sec);     // 24hr format
    rtc.setDate(date, mon, yr);   // Dd-Mm-YYYY
}

void setSprinkler(int station, bool state) {
    digitalWrite(station, state);
}

bool isProgram() {
    Time now = getTime();
    // Verificar mes
    if (prog.months[now.mon - 1] == 1) {
        //Verificar dow
        if (prog.dows[now.dow - 1] == 1) {
            if (prog.hr == now.hour) {
                if (prog.min == now.min) return true;
            }
        }
    }
    return false;
}

void printTime(DS1302 rtc) {
    // Send Day-of-Week
    Serial.print(rtc.getDOWStr());
    Serial.print(" ");

    // Send date
    Serial.print(rtc.getDateStr());
    Serial.print(" -- ");

    // Send time
    Serial.println(rtc.getTimeStr());
}

void setTimer(unsigned long duracion) {
    unsigned long current = millis();
    termination = (duracion * 60000) + current;
}

bool checkTimer() {
    unsigned long current = millis();
    if (current >= termination) {
        return true;
    }
    else return false;
}

void setup() {
    // -- Begin debug --
    Serial.begin(9600);
    // -- End debug --

    // Válvulas de riego (Low Level Logic)
    pinMode(ESTACION_1, OUTPUT);
    pinMode(ESTACION_2, OUTPUT);
    digitalWrite(ESTACION_1, HIGH);
    digitalWrite(ESTACION_2, HIGH);

    // Inicialización rtc
    rtc.halt(false);
    rtc.writeProtect(false);
}

void loop() {
    if (isProgram()) {
        // Estación 1
        setTimer(prog.duracion);
        // -- Begin debug --
        Serial.println("Estacion 1 Regando");
        // -- End debug --
        while (!checkTimer()) {
            delay(1000);
        }
        // Estación 2
        setTimer(prog.duracion);
        // -- Begin debug --
        Serial.println("Estacion 2 Regando");
        // -- End debug --
        while (!checkTimer()) {
            delay(1000);
        }
        // -- Begin debug --
        Serial.println("Fin programa");
        // -- End debug --
    }
    printTime(rtc);
    delay(1000);
}