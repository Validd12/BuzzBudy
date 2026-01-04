Smart Home Bluetooth Android App


⸻

Features
	•	Connects to an HC-05 Bluetooth module.
	•	Sends commands to turn alarm ON/OFF.
	•	Sends commands to turn backlight ON/OFF.
	•	Controls servo motor to open/close.
	•	Receives and displays messages from the Arduino device.
	•	Handles Bluetooth permission requests (including Android 12+).
	•	Provides status updates and feedback via the UI.

⸻

Requirements
	•	Android device with Bluetooth support.
	•	Arduino or microcontroller with HC-05 Bluetooth module configured.
	•	Android Studio for building the app.
	•	Minimum SDK: 21 (Android 5.0 Lollipop).
	•	Target SDK: 33 (Android 13).

⸻

Installation
	1.	Clone or download this project.
	2.	Open the project in Android Studio.
	3.	Make sure your Android SDK and build tools are updated.
	4.	Connect your Android device or use an emulator with Bluetooth support.
	5.	Build and run the app.

⸻

Usage
	1.	Pair your Android device with the HC-05 Bluetooth module first via system Bluetooth settings.
	2.	Open the app and press Connect.
	3.	Once connected, use buttons to send commands:
	•	Alarm ON/OFF
	•	Backlight ON/OFF
	•	Servo Open/Close
	4.	Messages from the Arduino device will appear in the response area.
	5.	Disconnect by closing the app or turning off Bluetooth.

⸻

Permissions

The app requests:
	•	BLUETOOTH_CONNECT and BLUETOOTH_SCAN (for Android 12+)
	•	ACCESS_FINE_LOCATION (for Bluetooth scanning on older versions)

⸻

Arduino Integration
	•	The Arduino sketch must use the HC-05 Bluetooth module with Serial communication.
	•	Commands sent from the app must be parsed and executed accordingly.
	•	The Arduino should send text responses ending with a newline (\n) for the app to display.
