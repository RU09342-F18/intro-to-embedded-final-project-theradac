## Hardware Setup
First, view the included detailed block diagram in order to see how to properly set up the circuitry
on a breadboard. A power supply that can provide at least 15V at 2A is required
for proper op-amp operation. The resistor values, component models, and any extra
voltages are all listed in the detailed block diagram. Be sure when setting up to avoid
accidentally plugging any pins into the supply rails, at risk of one damaging or destroying
the board. Plug the voltage output of the distance sensor into pin 6.0 of the
MSP430, and connect the other sensor to the current limit input of the op-amp. Plug
both the MSP430 power cable and the UART cable into USB ports, connect the white
wire of the UART cable to pin 3.3, connect the green cable to pin 3.4, and connect the
black cable to any ground pin on the board. DO NOT connect the red cable, for it is a
5V cable and can damage or destroy the board.
## Software Setup
First, be sure both code composer studio and Realterm are installed on the computer.
In code composer studio, once the board is connected via the USB, debug the project,
proceed on the low power mode message, and once complete, click the resume button
on the top bar to start debugging. In Realterm, under the ports tab, be sure the baud
rate is set to 9600, and make sure the COM port of the UART cable is selected. To
find the COM port, open up device manager (if on windows) and expand the ports tab.
The COM port of the UART cable should be labelled. Once set up, navigate to the
display tab and check off ASCII and navigate to the send tab to send either a ”s”, ”q”,
”r”, or ”t” to change the waveform type. If you enter an invalid type, the default output 
is a square wave.