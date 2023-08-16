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
// Botones - Alias
#define CANCEL 1
#define UP 2
#define ENTER 3
// Botones - Flag
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

// Obtiene la fecha y hora actual
Time getTime() {
    return rtc.getTime();
}
// Programa la hora en el reloj
void setTime(Time curr) {
    // rtc.setDOW(curr.dow);        // Set Day-of-Week
    // rtc.setDate(curr.date, curr.mon, curr.year);   // Dd-Mm-YYYY
    rtc.setTime(curr.hour, curr.min, 0);     // 24hr format
}
// Programa la fecha en el reloj
void setDate(Time curr) {
    rtc.setDOW(curr.dow);
    rtc.setDate(curr.date, curr.mon, curr.year);
}
// Enciende o apaga una estación determinada (1 o 2)
void setSprinkler(int station, bool state) {
    if (state) {
        digitalWrite(station, LOW);
    }
    else {
        digitalWrite(station, HIGH);
    }
}
// Chequea el programa con el tiempo actual
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
// Verifica que sean las 0:00 para realizar el reinicio de seguridad que evita el overflow de millis
bool isSafetyReset() {
    Time now = getTime();
    return (now.hour == 0 && now.min == 1 && now.sec == 0);
}
// Envia señal de reinicio a través del pin que se encuentra conectado a Reset
void safetyReset() {
    digitalWrite(RESET_PIN, LOW);
}
// Inicializa el valor de terminación que chequea el timer para apagar los regadores
void setTimer(unsigned long duracion) {
    unsigned long current = millis();
    termination = (duracion * 60000) + current;
}
// Verifica si ya transcurrio la duración de encendido de los regadores y lo informa mediante el flag global timeOut
void checkTimer() {
    unsigned long current = millis();
    if (current >= termination) {
        timeOut = true;
    }
    else timeOut = false;
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
// Imprime la estación de riego actual
void LCD_Regando(int station) {
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
// Imprime fecha y debajo hora. Se usa para el estado de Stand By
void LCD_Time() {
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
// Imprime hora y min del tiempo pasado por argumento
void LCD_Time(Time curr) {
    delay(50);
    lcd.setCursor(5, 1);
    if (curr.hour < 10) {
        lcd.print(0);
    }
    lcd.print(curr.hour);
    lcd.print(":");
    if (curr.min < 10) {
        lcd.print(0);
    }
    lcd.print(curr.min);
}
// Imprime fecha del tiempo pasado por argumento
void LCD_Date(Time curr) {
    lcd.setCursor(2, 1);
    switch (curr.dow) {
    case MONDAY:
        lcd.print("LUN");
        break;
    case TUESDAY:
        lcd.print("MAR");
        break;
    case WEDNESDAY:
        lcd.print("MIE");
        break;
    case THURSDAY:
        lcd.print("JUE");
        break;
    case FRIDAY:
        lcd.print("VIE");
        break;
    case SATURDAY:
        lcd.print("SAB");
        break;
    case SUNDAY:
        lcd.print("DOM");
        break;
    }
    lcd.setCursor(6, 1);
    if (curr.date < 10) {
        lcd.print(0);
    }
    lcd.print(curr.date);
    lcd.print("/");
    if (curr.mon < 10) {
        lcd.print(0);
    }
    lcd.print(curr.mon);
    lcd.print("/");
    if (curr.year < 10) {
        lcd.print(0);
    }
    lcd.print(curr.year);
}
// La interfaz informa al usuario que esta ingresando a modificar la hora
void LCD_GoToHora() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODIFICAR HORA?");
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
// La interfaz muestra que se esta modificando la hora
void LCD_ModHora() {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("MODIFICAR HORA");
}
// La interfaz muestra que se esta modificando la fecha
void LCD_ModFecha() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODIFICAR FECHA");
}
// La interfaz muestra que se esta modificando el programa
void LCD_ModProg() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MODIFICAR PROG.");
}
void setup() {
    Serial.begin(9600);

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
        Serial.println("Estado: REGANDO 1");
        LCD_Regando(1);
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
        Serial.println("Estado: REGANDO 2");
        LCD_Regando(2);
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
        bool fin = false;
        lcd.blink();
        // Setup menu hora
        LCD_GoToHora();
        Serial.println("Estado: MENU HORA"); // debug
        // Loop menu hora
        while (pollButtons() != CANCEL && !fin) {
            // Setup modificar hora
            if (buttonPress == ENTER) {
                Serial.println("Estado: MOD HORA"); // debug
                LCD_ModHora();
                Time curr = getTime();
                int state = 0; //0 mod hora, 1 mod min, 2 guardar
                // Loop modificar hora
                while (pollButtons() != CANCEL && !fin) {
                    LCD_Time(curr);
                    pollButtons();
                    switch (state) {
                    case 0:
                        lcd.setCursor(6, 1);
                        if (buttonPress == UP) {
                            curr.hour++;
                            if (curr.hour > 23) curr.hour = 0;
                        }
                        else if (buttonPress == ENTER) {
                            state = 1; //grabar hora y pasar a mod min
                        }
                        break;
                    case 1:
                        lcd.setCursor(9, 1);
                        if (buttonPress == UP) {
                            curr.min++;
                            if (curr.min > 59) curr.min = 0;
                        }
                        else if (buttonPress == ENTER) {
                            setTime(curr);
                            // debug
                            Serial.println("Nueva hora guardada");
                            Serial.print(curr.hour);
                            Serial.print(":");
                            Serial.println(curr.min);
                            // end debug
                            fin = true;
                        }
                        break;
                    }
                    if (buttonPress == CANCEL){
                        if(state > 0) state--;
                        else fin=true;
                    }
                }
            }
            // Setup menu fecha
            else if (buttonPress == UP) {
                LCD_GoToFecha();
                // Loop menu fecha
                while (pollButtons() != CANCEL && !fin) {
                    // Setup modificar fecha
                    if (buttonPress == ENTER) {
                        LCD_ModFecha();
                        fin = false;
                        Time curr = getTime();
                        int state = 0; //0 mod dow, 1 mod dia, 2 mod mes, 3 mod año
                        // Loop modificar fecha
                        while (pollButtons() != CANCEL && !fin) {
                            LCD_Date(curr);
                            pollButtons();
                            switch (state) {
                            case 0: //mod dow
                                lcd.setCursor(3, 1);
                                if (buttonPress == UP) {
                                    curr.dow++;
                                    if (curr.dow > 7) curr.dow = 1;
                                }
                                else if (buttonPress == ENTER) {
                                    state = 1; //grabar dow y pasar a mod dia
                                }
                                break;
                            case 1: //mod dia
                                lcd.setCursor(7, 1);
                                if (buttonPress == UP) {
                                    curr.date++;
                                    if (curr.date > 31) curr.date = 1;
                                }
                                else if (buttonPress == ENTER) { //grabar dia y pasar a mod mes
                                    state = 2;
                                }
                                break;
                            case 2: //mod mes
                                lcd.setCursor(10, 1);
                                if (buttonPress == UP) {
                                    curr.mon++;
                                    if (curr.mon > 12) curr.mon = 1;
                                }
                                else if (buttonPress == ENTER) { //grabar mes y pasar a mod año
                                    state = 3;
                                }
                                break;
                            case 3: //mod añó
                                lcd.setCursor(12, 1);
                                if (buttonPress == UP) {
                                    curr.year++;
                                    if (curr.year > 99) curr.year = 0;
                                }
                                else if (buttonPress == ENTER) { //grabar fecha y salir
                                    setDate(curr);
                                    // debug
                                    Serial.println("Nueva fecha guardada");
                                    Serial.print(curr.dow);
                                    Serial.print(" ");
                                    Serial.print(curr.date);
                                    Serial.print("/");
                                    Serial.print(curr.mon);
                                    Serial.print("/");
                                    Serial.println(curr.year);
                                    // end debug
                                    fin = true;
                                }
                                break;
                            }
                            if (buttonPress == CANCEL && state > 0) state--;
                        }
                    }
                    // Setup menu programa
                    else if (buttonPress == UP) {
                        LCD_GoToProg();
                        // Loop menu programa
                        while (pollButtons() != CANCEL) {
                            // Setup modificar programa
                            if (buttonPress == ENTER) {
                                lcd.clear();
                                lcd.setCursor(0, 0);
                                lcd.print("MODIFICAN2 PROGRAMA");
                                lcd.blink();
                                // Loop modificar programa
                                while (pollButtons() != CANCEL) {
                                }
                            }
                            else if (buttonPress == UP) break; // No hay más menús

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
        Serial.println("Estado: RESET");
    }

    // Stand By
    delay(1000);
    LCD_Time();
}