#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Setup LCD and servo
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo greenhouseServo;
OneWire oneWire(A1); // Ensure the pin is correct
DallasTemperature tempSensor(&oneWire);

// Define pin assignments
int moistureSensor = A0;
int fans = 7;
int pump = 6;
int rainSensor = A2;

// Initialize with an impossible value
int lastTempCelsius = -999; 
int lastMoistureSensorValue = -999;
int lastRainSensorValue = -999;
int lastFanStatus = -1;
int lastPumpStatus = -1;
int lastCoverPosition = -1;

// Sensor values
int rainSensorValue = 0;
int moistureSensorValue = 0;
int tempCelsius = 0;

// Environmental thresholds
int temperatureMin = 18;
int temperatureMax = 28;
int moistureMin = 300;
int moistureMax = 800;

// Open/Close angles for the greenhouse servo
int openAngle = 150;  // Open position angle
int closeAngle = 30;  // Closed position angle

// Manual control flags
bool manualFanControl = false;
bool manualPumpControl = false;
bool manualGreenhouseControl = false;

// Display toggle for cycling through data
bool displayToggle = false;

// Begin Serial communication at 9600 Baud rate
void setup() {
    Serial.begin(9600); // Begin Serial communication at 9600 Baud rate
    lcd.init();
    lcd.backlight();
    greenhouseServo.attach(9);
    tempSensor.begin();

    pinMode(moistureSensor, INPUT);
    pinMode(fans, OUTPUT);
    pinMode(pump, OUTPUT);
    pinMode(rainSensor, INPUT);

    lcd.clear();
    lcd.print("Greenhouse Start");
    lcd.setCursor(0, 1);
    lcd.print("Setup Complete");
    delay(2000);  // Display the initial message for 2 seconds
    lcd.clear();
}

void loop() {
    readSensors();
    if (!manualFanControl && !manualPumpControl && !manualGreenhouseControl) {
        automaticControl();
    }
    updateLCD();  // Update LCD display with sensor data and status
    sendSensorData();
    receiveCommands();
    delay(3000); // Adjust delay based on needs
}

void readSensors() {
    tempSensor.requestTemperatures();
    tempCelsius = tempSensor.getTempCByIndex(0);
    rainSensorValue = analogRead(rainSensor);
    moistureSensorValue = analogRead(moistureSensor);
}

void automaticControl() {
    digitalWrite(fans, (tempCelsius >= temperatureMax) ? HIGH : LOW);
    digitalWrite(pump, (moistureSensorValue < moistureMin) ? HIGH : LOW);
    greenhouseServo.write((rainSensorValue > 500) ? openAngle : closeAngle);
}

void sendSensorData() {
    int currentFanStatus = digitalRead(fans);
    int currentPumpStatus = digitalRead(pump);
    int currentCoverPosition = greenhouseServo.read();

    if (tempCelsius != lastTempCelsius ||
        moistureSensorValue != lastMoistureSensorValue ||
        rainSensorValue != lastRainSensorValue ||
        currentFanStatus != lastFanStatus ||
        currentPumpStatus != lastPumpStatus ||
        currentCoverPosition != lastCoverPosition) {
        
        Serial.print("Temp=");
        Serial.print(tempCelsius);
        Serial.print(", Moisture=");
        Serial.print(moistureSensorValue);
        Serial.print(", Rain=");
        Serial.print(rainSensorValue);
        Serial.print(", Fan=");
        Serial.print(currentFanStatus == HIGH ? "On" : "Off");
        Serial.print(", Pump=");
        Serial.print(currentPumpStatus == HIGH ? "On" : "Off");
        Serial.print(", Cover=");
        Serial.println(currentCoverPosition == openAngle ? "Open" : "Closed");

        // Update last known values
        lastTempCelsius = tempCelsius;
        lastMoistureSensorValue = moistureSensorValue;
        lastRainSensorValue = rainSensorValue;
        lastFanStatus = currentFanStatus;
        lastPumpStatus = currentPumpStatus;
        lastCoverPosition = currentCoverPosition;
    }
}

void receiveCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        handleCommand(command);
        Serial.print("Command received: ");
        Serial.println(command);
    }
}

void handleCommand(String command) {
    if (command.startsWith("SET_TEMP_MIN:")) {
        int temp = command.substring(13).toInt();
        if (temp > 0) temperatureMin = temp;
    } else if (command.startsWith("SET_TEMP_MAX:")) {
        int temp = command.substring(13).toInt();
        if (temp > 0) temperatureMax = temp;
    } else if (command == "FAN_ON") {
        digitalWrite(fans, HIGH);
        manualFanControl = true;
    } else if (command == "FAN_OFF") {
        digitalWrite(fans, LOW);
        manualFanControl = false;
    } else if (command == "PUMP_ON") {
        digitalWrite(pump, HIGH);
        manualPumpControl = true;
    } else if (command == "PUMP_OFF") {
        digitalWrite(pump, LOW);
        manualPumpControl = false;
    } else if (command == "OPEN_GREENHOUSE") {
        greenhouseServo.write(openAngle);
        manualGreenhouseControl = true;
    } else if (command == "CLOSE_GREENHOUSE") {
        greenhouseServo.write(closeAngle);
        manualGreenhouseControl = false;
    }
}

void updateLCD() {
    lcd.clear();
    if (displayToggle) {
        lcd.print("Temp: ");
        lcd.print(tempCelsius);
        lcd.print(" C");
        lcd.setCursor(0, 1);
        lcd.print("Moist: ");
        lcd.print(moistureSensorValue);
    } else {
        lcd.print("Fan: ");
        lcd.print(digitalRead(fans) == HIGH ? "On" : "Off");
        lcd.print(" Pump: ");
        lcd.print(digitalRead(pump) == HIGH ? "On" : "Off");
        lcd.setCursor(0, 1);
        lcd.print("Cover: ");
        lcd.print(greenhouseServo.read() == openAngle ? "Open" : "Closed");
    }
    displayToggle = !displayToggle;  // Toggle the flag for next cycle
}
