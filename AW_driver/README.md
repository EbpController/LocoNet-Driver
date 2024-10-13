This is the AW driver (AW = turnout) software for the PIC18F24/25/26/27/45/46/47Q10 microcontroller family.
It use the LocoNet driver (ln.c and ln.h routines) and must be included in the project.
The code is written in C and is compatible to the "[LocoNet Personal Use Edition 1.0 SPECIFICATION](https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf)" from DigiTrax Inc.

The AW driver uses the Timer 3 and the Comparator 1, both with a high priority interrupt.
The AW driver can support 8 servos, with or without switches to control the sweep movement (= optional).

The following hardware pins on the microcontroller are used:
  - RD0 - RD7: servo motor output
  - RB0 - RB7: switches to control the sweep movement (optional)
  - RA0 - RA1 / RA6 - RA7 / RC0 - RC3: 8 bits for the LocoNet address A3 - A10 (the address bits A0 - A2 = index of the AW)
  - RC4: KAWL line for the switches in left position
  - RC5: KAWR line for the switches in right position
  - RE0: led indicator to show that the device is running

Principle:
 Refer to the LocoNet specifications in https://wiki.rocrail.net/doku.php?id=loconet:ln-pe-en and https://wiki.rocrail.net/doku.php?id=loconet:lnpe-parms-en

 Sending an AW command (following the LocoNet protocol):
  - The first byte (OPC) is the opcode 'B0' to command the AW.
  - The second byte (SW1) is the lower part of the address of the AW, where A0 - A2 = index of the AW (0 to 7) and A3 - A6 the address that must be correspond to the inputs RA0 - RA1 and RA6 - RA7.
  - The third byte (SW2) is the upper part of the address of the AW, where A7 - A10 must correspond to the inputs RC0 - RC3. The ON bit is ignored and must be set to 0. The DIR bit is 0 or 1 to control the AW in the right or left position.
  - The fourth byte (CKSUM) is the checksum of the previous three byttes

 Receiving an AW message (following the LocoNet protocol):
  - The first byte (OPC) is the opcode 'B1' to command the AW.
  - The second byte (SN1) is the lower part of the address of the AW, where A0 - A2 = index of the AW (1 to 8) and A3 - A6 the address that corresponds to the inputs RA0 - RA1 and RA6 - RA7.
  - The third byte (SN2, alternately) is the upper part of the address of the AW, where A7 - A10 corresponds to the inputs RC0 - RC3. The C bit is the KAWL information (AW is in left position), while the T bit is the KAWR information (AW is in right position).
  - The fourth byte (CKSUM) is the checksum of the previous three byttes
  