# Hydration Alert
Arduino software designed for the Assistive Technology Hydration Alert project

### Installation and Setup
- Download this repository as a zipped folder and extract it.
- Using the Arduino IDE, open HydrationAlert.ino
- Make sure the proper processor is set
  - Go to Tools -> Processor and make sure `ATmega328P (Old Bootloader)` is selected
- Import the SmaartWire library
  - Go to Sketch -> Include Library -> Add .ZIP Library...
  - Select the SmaartWire.zip file included in the repository
- Connect the Hydration Alert project's Arduino Nano to the computer using a USB cable
- Select the proper port under Tools -> Port
- Upload the code to the Arduino Nano

### Expected Behavior
Once the installation and setup process is complete, the Arduino can be powered using a 9V battery.\
Upon receiving power, the TMP107 will be initialized along with some Input/Output pins on the Arduino Nano.\
After this calibration, the TMP107 will measure the current surrounding temperature and set a timeout to begin the reminder vibration sequence.\
At any point, the included SPST switch can be pressed to stop any vibrating and set a new timeout (should be pressed every time the user drinks water).\
When the specified timeout has elapsed, the vibration sequence will begin according to a pattern specified in the code.

The formula for the timeout in minutes is as a function of the temperature in farenheit (x) as follows:\
`-0.0015x^2 + 30`, with a minimum delay of 8 minutes and a maximum of 30.
