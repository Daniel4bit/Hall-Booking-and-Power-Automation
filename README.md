# Hall-Booking-and-Power-Automation
The Room Booking and Power Automation System is an innovative solution designed to optimize energy usage and automate the power supply in rooms based on user needs. The system combines the concepts of room booking, power management, and IoT (Internet of Things) automation to provide a seamless user experience.

Users can scan a QR code located in a room to book power for a specific duration. Once the booking is made, the system powers the room via a relay connected to an ESP32-based controller. It provides an efficient way to manage electricity consumption, improve energy efficiency, and reduce unnecessary power wastage by turning off the power after the set duration.

## Core Features:
### Room Booking via QR Code:

Each room is assigned a unique QR code that users can scan using a smartphone app.
The app allows users to set the duration for power usage.
The system checks the availability of the room (power availability) and grants the user control over the power for the set duration.
### Power Control:

The system uses an ESP32 Ethernet Shield connected to a relay and a buzzer. The relay controls the power supply to the room (typically connected to lights, air conditioning, or other electrical devices).
The ESP32 communicates via MQTT protocol over Ethernet to ensure reliable communication between the server, mobile app, and the device in the room.
### Time-based Power Management:

Once the power is enabled, the system tracks the set time. A countdown is displayed to the user.
10-minute warning alarm: A buzzer connected to the ESP32 will sound an alarm when the remaining time reaches 10 minutes, alerting users to prepare for the power cut-off.
Once the booking time expires, the system automatically disconnects the power by switching off the relay, conserving energy.
### Device Communication:

The ESP32 communicates with the central server using MQTT (Message Queuing Telemetry Transport), a lightweight messaging protocol ideal for IoT devices. This ensures the system can efficiently handle multiple devices and users at the same time with minimal network overhead.
Communication is done over Ethernet, ensuring a stable and reliable connection, even in environments with limited Wi-Fi coverage.
### OTA Firmware Updates:

The ESP32 device supports Over-the-Air (OTA) firmware updates to ensure that the system can be updated remotely, without the need for physical access to the devices.
This feature enables easy updates for bug fixes, security patches, or new features.
### Hardware Components:
ESP32 Ethernet Shield: Acts as the core controller, responsible for receiving MQTT messages and controlling the relay. The Ethernet shield ensures stable and fast connectivity.
Relay Module: Used to control the electrical power to the room (turning on or off lights, fans, AC, etc.).
Buzzer: Provides an audible alarm to indicate when there are 10 minutes left before the power cutoff or when the power is about to be turned off.
Ethernet Communication: The ESP32 communicates via a wired Ethernet connection to reduce latency and improve reliability compared to Wi-Fi in environments where Wi-Fi might be unreliable.
### Software Features:
#### Mobile App:

The mobile app allows users to scan the QR code and set the power duration.
The app communicates with the backend server via MQTT, allowing users to receive updates on the remaining time and power status in real-time.
The app can be developed for both Android and iOS platforms.
#### Backend Server:

The backend manages room availability, booking records, and user requests.
It also pushes updates to the ESP32 devices in real-time, such as power status and countdown information.
The server will keep track of the duration for which power is booked and send the appropriate MQTT message to disconnect the power once the time expires.
#### MQTT Broker:

The MQTT broker acts as the intermediary between the mobile app, backend server, and ESP32 devices.
It ensures messages are reliably delivered and can handle multiple devices simultaneously.
### Workflow:
#### Scanning QR Code:

The user enters the room and scans the QR code displayed on the door or wall using the app.
The app prompts the user to select the duration for which they need power (e.g., 30 minutes, 1 hour, etc.).
The app sends this request to the server via MQTT.

#### Power Booking and Confirmation:

The server checks the availability of power and confirms the booking. A message is sent to the ESP32 device in the room to switch on the power for the specified duration.
The power is supplied to the room via the relay.

#### Time Management:

Once the power is on, the app provides a countdown timer to the user.
10-minute alarm: As the time nears expiration, the buzzer activates, signaling the user that they have 10 minutes left.
#### Power Cutoff:

After the set duration ends, the ESP32 automatically turns off the relay, cutting the power to the room.
The user is notified via the app that the power has been turned off.
#### OTA Updates:

If there are any updates for the system, the ESP32 can be updated remotely using the OTA firmware update functionality.

### Benefits:
Energy Efficiency: By ensuring that power is only available when needed and turning off automatically after the set time, the system reduces energy wastage.
Convenience: Users can easily book power using a simple app, and they receive timely alerts before the power is turned off.
Reliability: The use of Ethernet for communication ensures more stable and reliable performance compared to Wi-Fi, which is essential for critical power management tasks.
Scalability: Multiple rooms and devices can be managed simultaneously with the MQTT protocol, allowing the system to scale to larger buildings or campuses.
