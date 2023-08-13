#include <Arduino.h>
#include <DS1302.h>
#include <LiquidCrystal_I2C.h>

// Valvulas
#define ESTACION_1 2
#define ESTACION_2 3

// RTC
#define RST_PIN 4
#define DATA_PIN 5
#define CLK_PIN 6
DS1302 rtc(4, 5, 6);

// Botones
#define CANCEL_PIN 7
#define UP_PIN 8
#define ENTER_PIN 9
#define CANCEL 1
#define UP 2
#define ENTER 3
int buttonPress = 0;

// LCD
#define SDA_PIN A4
#define SCL_PIN A5
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// Safety reset
#define RESET_PIN 12

bool timeOut = false;
unsigned long termination;

// Programa de riego
typedef struct {
    // (L, M, X, J, V, S, D)
    int dows[7] = { 1,0,1,0,1,0,1 };
    // Formato 24hr
    int hr = 20, min = 32;

    int months[12] = { 1,1,1,1,1,1,1,1,1,1,1,1 };
    // Minutos
    int duracion = 60;
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
    if (state) {
        digitalWrite(station, LOW);
    }
    else {
        digitalWrite(station, HIGH);
    }
}

bool isProgram() {
    Time now = getTime();
    // Verificar mes
    if (prog.months[now.mon - 1] == 1) {
        //Verificar dow
        if (prog.dows[now.dow - 1] == 1) {
            if (prog.hr == now.hour) {
                if (prog.min == now.min) {
                    if (now.sec < 3) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// to prevent possible overflow on millis() count which may cause the sprinklers to exceed its duration setting I think it would be necessary to reset the device every 24 hours (overflow occurs every ~49 days)
bool isSafetyReset() {
    Time now = getTime();
    return (now.hour == 0 && now.min == 0 && now.sec == 0);
}

void safetyReset() {
    Serial.print("Last millis: ");
    Serial.println(millis());
    delay(100);
    digitalWrite(RESET_PIN, LOW);
}

void setTimer(unsigned long duracion) {
    unsigned long current = millis();
    termination = (duracion * 60000) + current;
}

void checkTimer() {
    unsigned long current = millis();
    if (current >= termination) {
        timeOut = true;
    }
    else timeOut = false;
}

void lcdRegando(int station) {
    lcd.clear();

    if (station == 1) {
        lcd.print("** Estación 1 **");
    }
    else {
        lcd.print("** Estación 2 **");
    }
    lcd.setCursor(0, 1);
    lcd.print("** Regando... **");
    lcd.blink();
}
// Imprime fecha y debajo hora
void printTime() {
    lcd.clear();
    lcd.setCursor(2, 0);
    // Send Day-of-Week
    lcd.print(rtc.getDOWStr(FORMAT_SHORT));
    lcd.print(" ");
    // Send date
    lcd.print(rtc.getDateStr(FORMAT_SHORT, FORMAT_SHORT, '/'));//14

    lcd.setCursor(4, 1);
    lcd.print(rtc.getTimeStr());
}
// Devuelve el boton pulsado y tambien escribe la variable global buttonPress
int pollButtons() {
    if (digitalRead(CANCEL_PIN)) {
        delay(50);
        while (digitalRead(CANCEL_PIN));
        delay(50);
        buttonPress = 1;
    }
    else if (digitalRead(UP_PIN)) {
        delay(50);
        while (digitalRead(UP_PIN));
        delay(50);
        buttonPress = 2;
    }
    else if (digitalRead(ENTER_PIN)) {
        delay(50);
        while (digitalRead(ENTER_PIN));
        delay(50);
        buttonPress = 3;
    }
    else buttonPress = 0;

    return buttonPress;
}
// La interfaz informa al usuario que esta ingresando a modificar la hora
void LCD_GoToHora() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODIFICAR HORA?");
    lcd.blink();
}
// La interfaz informa al usuario que esta ingresando a modificar la fecha
void LCD_GoToFecha() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODIFICAR FECHA?");
}
// La interfaz informa al usuario que esta ingresando a modificar el programa
void LCD_GoToProg() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODIFICAR PROG.?");
}

void setup() {
    // Inicialización LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);

    // Reinicio de seguridad
    digitalWrite(RESET_PIN, HIGH);
    pinMode(RESET_PIN, OUTPUT);

    // Válvulas de riego (Low Level Logic)
    pinMode(ESTACION_1, OUTPUT);
    pinMode(ESTACION_2, OUTPUT);
    digitalWrite(ESTACION_1, HIGH);
    digitalWrite(ESTACION_2, HIGH);

    // Inicialización rtc
    rtc.halt(false);
    rtc.writeProtect(false);

    // Inicialización botones
    pinMode(CANCEL_PIN, INPUT);
    pinMode(UP_PIN, INPUT);
    pinMode(ENTER_PIN, INPUT);
}

void loop() {
    // Comprobar si debe iniciar el programa
    if (isProgram()) {

        // Estación 1
        lcdRegando(1);
        setSprinkler(ESTACION_1, true);
        setTimer(prog.duracion);
        while (!timeOut) {
            // Poll del boton cancelar
            if (digitalRead(CANCEL_PIN) == 1) {
                timeOut = true;
            }

            checkTimer();
        }
        setSprinkler(ESTACION_1, false);
        timeOut = false;

        // Estación 2
        lcdRegando(2);
        setSprinkler(ESTACION_2, true);
        setTimer(prog.duracion);
        while (!timeOut) {
            // Poll del boton cancelar
            if (digitalRead(CANCEL_PIN) == 1) {
                timeOut = true;
            }

            checkTimer();
        }
        setSprinkler(ESTACION_2, false);
        timeOut = false;
    }

    // Navegar menus
    if (pollButtons() == ENTER) {
        LCD_GoToHora();
        // Cancel regresa atras (Loop normal)
        while (pollButtons() != CANCEL) {
            // Ingreso a modificar hora
            if (buttonPress == ENTER) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("MODIFICANDO HORA");
                while (pollButtons() != CANCEL) {

                }
            }
            // Siguiente menu: modificar fecha
            else if (buttonPress == UP) {
                LCD_GoToFecha();
                // Cancel regresa atras (Modificar hora?)
                while (pollButtons() != CANCEL) {
                    // Ingresar a modificar Fecha
                    if (buttonPress == ENTER) {
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.print("MODIFICAN2 FECHA");
                        lcd.blink();
                        while (pollButtons() != CANCEL) {

                        }
                    }
                    // Siguiente menu: modificar programa
                    else if (buttonPress == UP) {
                        LCD_GoToProg();
                        // Cancel regresa atras (Modificar fecha?)
                        while (pollButtons() != CANCEL) {
                            // Ingresar a menu Modificar Programa
                            if (buttonPress == ENTER) {
                                lcd.clear();
                                lcd.setCursor(0, 0);
                                lcd.print("MODIFICAN2 PROGRAMA");
                                lcd.blink();
                                while (pollButtons() != CANCEL) {
                                }
                            }else if (buttonPress == UP) break;
                            // No hay más menús
                        }
                        LCD_GoToFecha();
                    }
                }
                LCD_GoToHora();
            }
        }
        lcd.noBlink();
    }

    // Reinicio de seguridad a las 0:00 para evitar eventual overflow de millis()
    if (isSafetyReset()) {
        safetyReset();
    }

    // Stand By
    delay(1000);
    printTime();
}