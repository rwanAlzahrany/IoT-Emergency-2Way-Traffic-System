# IoT-Emergency-2Way-Traffic-System

### Overview

This project is a smart traffic light controller built with two ESP8266 (NodeMCU) boards. It uses IR sensors to detect traffic density and automatically adjust green light timing. It also includes an Emergency Override feature that allows an ambulance or fire truck to change the light to green instantly using ESP-NOW wireless communication.

### Main Features

Adaptive Timing: Uses IR sensors to detect if a road is crowded and adds extra green time.
Emergency Mode: A remote button unit sends an instant signal to clear the intersection.
IoT Cloud Logging: Sends traffic data (mode, direction, timing) to ThingSpeak.
Email Alerts: Uses IFTTT to send an email notification when emergency mode is active.
Buzzer Alert: Provides audio feedback during emergency triggers.

### Hardware Used

2x NodeMCU (ESP8266)
2x IR Sensors (for North/South density)
6x LEDs (Red, Yellow, Green for two sides)
1x Push Button (Emergency Trigger)
1x Buzzer
Jumper wires and Breadboard

### How to Use

Traffic Node: Upload Traffic_Controller.ino to the main intersection board.
Emergency Node: Update the trafficMAC address in Emergency_Unit.ino with your controller's MAC address and upload it.
Monitor: Open the Serial Monitor at 115200 baud to see the WiFi connection and traffic states.
Dashboard: Check your ThingSpeak channel to see the live traffic data.
