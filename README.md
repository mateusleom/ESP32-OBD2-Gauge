# ESP32-OBD2-Gauge
For complete description, go to original poster (Va&Cob).

In this forks i've made the following changes:
- Removal of selector button, the gauge will boot, connect to ELM327 and go straight to numeri meters. No more menu, DTCs or any kinds of navigation.
- Addition of oil pressure sensor. The code was calibrated to a 150 PSI 0-5V sensor. Although the sensor requires 5vcc, I powered it from 3,3Vcc internal supply of ESP32. It uses pin 27 for ADC. It requires a 2/3 voltage divider in order to read correctly. You must calibrate your pressure sensor with a trusted oil pressure gauge. (https://www.aliexpress.com/item/1005002854614444.html)
- Addition of water level sensor. The sensor accepts 5-25Vcc for supply, but again i used 3.3V from ESP32 and ti worked fine. The sensor used was XKC-25-V wired this way: Brown to ESP 3.3v supply, yellow to esp pin 35, blue and black to gnd.
- Addition of buzzer (https://www.aliexpress.com/item/1005006490431925.html - it must be a 3.3V active buzzer. Passive ones will not work). Uses pins GND and 22.
- 3d printed case for use in subaru's air vent.

- In some screens the colors will become reverse (red will become blue, black will become white, etc). To fix this you must uncomment one of the following lines in \sketch\library modified\TFT_eSPI\User_Setup.h:
           // #define TFT_INVERSION_ON
           // #define TFT_INVERSION_OFF
  One of those lines will fix the issue.


  - Last lines of void setup were modified in order to be able to read from two headers at the same time (7E1 and 7E0) so its possbile to have TCU and ECU information at the same time. Somes formulas where modified, some other lines were also modified.
 
  - I'm not a good coder so don't expect much support as the original poster might be able to do.


Present interface:

![20250320_145110](https://github.com/user-attachments/assets/25c0b1a3-b323-4c86-a890-6be8e1a32db4)



Water level sensor support (3d printed in ABS):

![20250320_144942](https://github.com/user-attachments/assets/404d86b8-3806-4994-82fd-5f378f81d57d)



Water level sensor in place:

![20250320_144935](https://github.com/user-attachments/assets/849eae2b-371d-4263-8520-e362fb60f166)



Oil pressure sensor in place:

![20250320_145540](https://github.com/user-attachments/assets/99c7670b-4fc3-435c-a460-48c47277a65e)




General Wiring:


![20250320_154115](https://github.com/user-attachments/assets/24ba10cc-ca21-401a-8f4f-ce5fa05d6aa1)






