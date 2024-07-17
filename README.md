This is the LocoNet driver software for the PIC18Fxx2y(*), PIC18F24/25Q10, PIC18F26/45/46Q10 and PIC18F27/47Q10 microcontroller family. (* xx = 25/26/45/46, y = 0/5)
The code is written in C, as a replica of the assembly code of [Geert Giebens](https://github.com/GeertGiebens), and is compatible to the "[LocoNet Personal Use Edition 1.0 SPECIFICATION](https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf)" from DigiTrax Inc.

The program takes the EUSART 1 and the Timer 1, both with a low priority interrupt. Also the internal comparator 1 is used.

The following hardware pins on the microcontroller are used:
  - RA0: comparator 1, inverting input (C1IN-)
  - RA3: comparator 1, non-inverting input (C1IN+)
  - RA4: comparator 1, output (C1OUT)
  - RC6: LocoNet transmitter (EUSART 1, TXD) + indication LED (yellow)
  - RC7: LocoNet receiver (EUSART 1, RXD) + indication LED (bleu)

The pins RA1, RA2 and RA5 are used as indication LEDs, where:
  - RA1: valid LocoNet message received (green)
  - RA2: linebreak or error detected on the LocoNet bus (red)
  - RA5: free to use

The pins RC0-RC5 and RA6-RA7 can be used for LocoNet address configuration.
The pins RB0-RB7, RD0-RD7 and RE0-RE3 are free and can be used as GPIO, I2C, ...

Include this library into your (LocoNet) project.
 - To transmit a LocoNet message, the function lnTxMessageHandler(lnMessage*) can be invoked.
 - To receive a LocoNet message, a lnRxMessageHandler(lnMessage*) callback function must be included.