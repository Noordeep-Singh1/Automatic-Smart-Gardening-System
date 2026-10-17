# 🌱 Automatic Smart Gardening System

An Arduino-based automatic gardening system that monitors soil moisture and controls irrigation for three plants.

## Features
- Automatic soil moisture monitoring
- Independent watering for three plants
- Relay-based solenoid valve control
- Gravity-fed irrigation system
- LCD display for system information

## Plants
- Aloe Vera
- Hibiscus
- Croton

## Hardware Components
- Arduino Nano
- 3 Capacitive Soil Moisture Sensors
- 4-Channel 5V Relay Module
- 3 × 12V DC Solenoid Valves
- 16×2 I2C LCD
- LM2596 Buck Converter
- 12V Power Adapter
- Water tank and pipes

## Pin Connections

| Component | Arduino Pin |
|---|---|
| Aloe Vera sensor | A0 |
| Hibiscus sensor | A1 |
| Croton sensor | A2 |
| Aloe Vera relay | D2 |
| Hibiscus relay | D3 |
| Croton relay | D4 |
| LCD SDA | A4 |
| LCD SCL | A5 |

## How It Works
1. Soil moisture sensors measure the moisture level of each plant.
2. The Arduino compares the readings with configured thresholds.
3. The corresponding relay controls the solenoid valve when watering is needed.
4. Water flows from the tank to the plant through the pipes.
5. The LCD displays system information.

## How to Run
1. Download or clone this repository.
2. Open the `.ino` file in Arduino IDE.
3. Install any required libraries.
4. Select the correct Arduino Nano board and port.
5. Verify the wiring and upload the code.

## Author
Noor

## 📸 Project Gallery

### Complete Automatic Smart Gardening System

![Complete Smart Gardening System](smart-gardening-system.jpg)

### Electronic Components

![Electronic Components](electronic-components.jpg)


## License
No license selected yet.
