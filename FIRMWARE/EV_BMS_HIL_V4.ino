#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "driver/temperature_sensor.h"
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 17
#define OLED_SCL 18
#define OLED_RST 21

#define BUTTON_PIN 0
#define EVENT_LOG_SIZE 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

temperature_sensor_handle_t temp_handle;

float chipTemp = 0;
float peakTemp = 0;

volatile int throttleDelay = 0;
volatile int powerPercent = 100;

unsigned long startTime = 0;

bool faultMode = false;

String eventLog[EVENT_LOG_SIZE];
int eventIndex = 0;

enum ThermalState
{
    SAFE,
    DERATE1,
    DERATE2,
    DERATE3,
    DERATE4,
    CRITICAL
};

ThermalState currentState = SAFE;
ThermalState previousState = SAFE;

TaskHandle_t loadTask1;
TaskHandle_t loadTask2;

// =======================
// EVENT LOGGER
// =======================

void addEvent(String event)
{
    eventLog[eventIndex] = event;

    eventIndex++;

    if(eventIndex >= EVENT_LOG_SIZE)
    {
        eventIndex = 0;
    }

    Serial.print("EVENT: ");
    Serial.println(event);
}

// =======================
// CORE 0 LOAD TASK
// =======================

void heavyLoadTask1(void *parameter)
{
    volatile double x = 0;

    while(true)
    {
        if(currentState == CRITICAL || faultMode)
        {
            vTaskDelay(100);
            continue;
        }

        for(int i = 1; i < 100000; i++)
        {
            x += sin(i) * cos(i) * sqrt(i);

            if(x > 999999999)
            {
                x = 0;
            }
        }

        vTaskDelay(1);
        vTaskDelay(throttleDelay / portTICK_PERIOD_MS);
    }
}

// =======================
// CORE 1 LOAD TASK
// =======================

void heavyLoadTask2(void *parameter)
{
    volatile double y = 0;

    while(true)
    {
        if(currentState == CRITICAL || faultMode)
        {
            vTaskDelay(100);
            continue;
        }

        for(int i = 1; i < 100000; i++)
        {
            y += sin(i) * cos(i) * sqrt(i);

            if(y > 999999999)
            {
                y = 0;
            }
        }

        vTaskDelay(1);
        vTaskDelay(throttleDelay / portTICK_PERIOD_MS);
    }
}

// =======================
// SETUP
// =======================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("BOOT");

    startTime = millis();

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    pinMode(OLED_RST, OUTPUT);

    digitalWrite(OLED_RST, LOW);
    delay(50);
    digitalWrite(OLED_RST, HIGH);

    Wire.begin(OLED_SDA, OLED_SCL);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED FAILED");

        while(1);
    }

    temperature_sensor_config_t temp_config =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);

    temperature_sensor_install(&temp_config, &temp_handle);

    temperature_sensor_enable(temp_handle);

    xTaskCreatePinnedToCore(
        heavyLoadTask1,
        "LOAD1",
        10000,
        NULL,
        1,
        &loadTask1,
        0);

    xTaskCreatePinnedToCore(
        heavyLoadTask2,
        "LOAD2",
        10000,
        NULL,
        1,
        &loadTask2,
        1);

    addEvent("SYSTEM START");

    Serial.println("EV BMS HIL V4 STARTED");
}

// =======================
// MAIN LOOP
// =======================

void loop()
{
    // --------------------
    // BUTTON HANDLING
    // --------------------

    static bool buttonLock = false;

if(digitalRead(BUTTON_PIN) == LOW)
{
    if(!buttonLock)
    {
        buttonLock = true;

        faultMode = !faultMode;

        if(faultMode)
        {
            addEvent("BATTERY FAULT ON");
            Serial.println("FAULT ENABLED");
        }
        else
        {
            addEvent("BATTERY FAULT OFF");
            Serial.println("FAULT DISABLED");
        }
    }
}
else
{
    buttonLock = false;
}

    // --------------------
    // TEMP MONITORING
    // --------------------

    temperature_sensor_get_celsius(temp_handle, &chipTemp);

    if(chipTemp > peakTemp)
    {
        peakTemp = chipTemp;
    }

    // --------------------
    // STATE MACHINE
    // --------------------

    if(chipTemp < 45)
    {
        currentState = SAFE;
        throttleDelay = 0;
        powerPercent = 100;
    }
    else if(chipTemp < 47)
    {
        currentState = DERATE1;
        throttleDelay = 20;
        powerPercent = 90;
    }
    else if(chipTemp < 49)
    {
        currentState = DERATE2;
        throttleDelay = 50;
        powerPercent = 70;
    }
    else if(chipTemp < 52)
    {
        currentState = DERATE3;
        throttleDelay = 100;
        powerPercent = 50;
    }
    else if(chipTemp < 55)
    {
        currentState = DERATE4;
        throttleDelay = 250;
        powerPercent = 25;
    }
    else
    {
        currentState = CRITICAL;
        throttleDelay = 1000;
        powerPercent = 0;
    }

    // --------------------
    // EVENT LOGGING
    // --------------------

    if(currentState != previousState)
    {
        switch(currentState)
        {
            case SAFE: addEvent("SAFE"); break;
            case DERATE1: addEvent("DERATE1"); break;
            case DERATE2: addEvent("DERATE2"); break;
            case DERATE3: addEvent("DERATE3"); break;
            case DERATE4: addEvent("DERATE4"); break;
            case CRITICAL: addEvent("CRITICAL"); break;
        }

        previousState = currentState;
    }

    // --------------------
    // RUNTIME
    // --------------------

    unsigned long runtimeSeconds =
        (millis() - startTime) / 1000;

    int minutes = runtimeSeconds / 60;
    int seconds = runtimeSeconds % 60;

    // --------------------
    // OLED
    // --------------------

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print("TEMP:");
    display.print(chipTemp, 1);
    display.println("C");

    display.setCursor(0, 10);
    display.print("PEAK:");
    display.print(peakTemp, 1);
    display.println("C");

    display.setCursor(0, 20);

    switch(currentState)
    {
        case SAFE: display.println("STATE:SAFE"); break;
        case DERATE1: display.println("STATE:D1"); break;
        case DERATE2: display.println("STATE:D2"); break;
        case DERATE3: display.println("STATE:D3"); break;
        case DERATE4: display.println("STATE:D4"); break;
        case CRITICAL: display.println("STATE:CRIT"); break;
    }

    display.setCursor(0, 30);
    display.print("POWER:");
    display.print(powerPercent);
    display.println("%");

    display.setCursor(0, 40);

    if(faultMode)
    {
        display.print("STATUS:FAULT");
    }
    else
    {
        display.print("STATUS:OK");
    }

    display.setCursor(0, 50);

    if(minutes < 10)
        display.print("0");

    display.print(minutes);
    display.print(":");

    if(seconds < 10)
        display.print("0");

    display.print(seconds);

    display.display();

    delay(500);
}
