# haptic_cane
This haptic walking cane was my spring 2023 embedded systems project. It uses ultrasonic distance sensors placed at three different angles to identify potential hazards for the user. It alerts the user to these hazards through a small dc motor with an offset weight that vibrates when the motor runs. Using PWM, the motor vibrates in different patterns and intensities corresponding to different kinds of hazards (trip hazard, wall, stairs, etc.). The distance readings that trigger different hazard responses are customizable through a UART interface. It is saved to EEPROM so that the board does not have to be reprogrammed to add or change hazard events. The user can customize distance thresholds for each sensor, event priority, event vibration patterns, and create compound events (events that trigger when two individual events are both true). Please note that while I wrote the bulk of the firmware and software, this project was designed and assigned to us by the professor. 

Target: TM4C123GH6PM microcontroller

Parts List:
-  1x 3D-printed front sensor assembly 
-  1x 3D-printed back sensor assembly 
-  1x 3D-printed lid 
-  1x 3D-printed power switch slide bar 
-  1x ½” SCH40 PVC Pipe (35”) (upright) 
-  1x ½” SCH40 PVC Pipe (5”) (handle) 
-  1x Grip 
-  1x Foot 
-  4x #4 x 3/8” pan head sheet metal screw
-  1x 3xAA battery pack 
-  3x AA battery 
-  2x #4 x 3/8” flat head sheet metal screw
     (attaches cover to battery pack)
-  2x #4 x 1/4” flat head sheet metal screw
     (attaches cover to back assembly)
-  1x Red/black Dupont connector pair with flying wire
     (splices to battery pack)
-  2x Pin header (single) (battery to controller board) 
-  1x Pin header (4 pos) (ground distribution) 
-  1x Pin header (5 pos) (5V distribution) 
-  1x Small PC board (power distribution) 
-  1x Motor with weight 
-  3x SRF04 ultrasonic sensor 
-  1x Foam or similar support for ultrasonic sensors 
-  6x 15cm socket-to-socket Dupont wire jumper 
-  9x 10cm socket-to-socket Dupont wire jumper 
-  1x PSMN4R3-30PL n-channel MOSFET (motor) 
-  1x 1kohm resistor (gate to processor PWM pin) 
-  1x 10kohm resistor (gate to ground, turn off) 
-  1x TM4C123GXL board

Utilization: 
  
  The values for events are not written in the main code. Instead, they are input by the user and saved to EEPROM. In order to do this, the following commands are supported. 
- **Alert** – 1 argument, this function returns argument 1 as a string encapsulated in exclamation marks. This mostly serves as a debug tool to make sure commands are still being accepted.
- **Event** – 4 arguments, this function writes the event specified by argument one to EEPROM. Arguments 2-4 are the sensor used (0-2), min (mm), and max (mm), respectively.
- **Pattern** – 5 arguments, this function writes the pattern for the event specified by argument one to EEPROM. Arguments 2-5 are PWM (%), beats, Ton (ms), and Toff (ms), respectively.
- **And** – 3 arguments, this function takes two regular events (arguments 2-3) and uses their values to write a compound event specified by argument one to EEPROM. This makes an event with 4 conditions and one to two sensors.
- **Show Events** – no arguments, this function reads EEPROM to print all event sensors and conditions to the screen.
- **Show Patterns** – no arguments, this function reads EEPROM to print all event vibration patterns to the screen.
- **Erase** – one argument, this function writes zeros for all values of the specified event in EEPROM.
- **Haptic** – one argument, this function writes a value of 0 or 1 to the “haptic” field of an event in EEPROM, specifying if the event will be used or not.
  
Motor circuit:
  
  <img width="219" alt="cane_motor_circuit" src="https://github.com/EdgarH52/haptic_cane/assets/89591117/4e5a9b3a-805e-4ea1-964e-a178a839ac7c">

Power supply board + motor circuit:

  ![image](https://github.com/EdgarH52/haptic_cane/assets/89591117/7738239b-a957-4768-926d-c607c2b856cf)

Assembled board + peripherals: 

  ![image](https://github.com/EdgarH52/haptic_cane/assets/89591117/fff13ce7-d956-4977-a54e-998af9d6994e)

Final result: 

  ![image](https://github.com/EdgarH52/haptic_cane/assets/89591117/cfa76c4d-0a28-4965-acaa-9b826d5ad003)![image](https://github.com/EdgarH52/haptic_cane/assets/89591117/c2f21c13-aa92-489e-8f87-d401eaf94bd9)

**Note**: This code uses definitions from Tivaware's tm4c123gh6pm header file. It also uses a a short UART and EEPROM library, as well as external waitMicrosecond() and initSystemClockTo40Mhz() functions. These are not included here.
