This is the LocoNet driver software for the PIC18F2525/2620/4525/4620 and PIC18F24/25/26/27/45/46/47Q10 microcontroller family.
The code is written in C, as a replica of the assembly code of [Geert Giebens](https://github.com/GeertGiebens), and is compatible to the "[LocoNet Personal Use Edition 1.0 SPECIFICATION](https://www.digitrax.com/static/apps/cms/media/documents/loconet/loconetpersonaledition.pdf)" from DigiTrax Inc.

The program takes the EUSART 1 and the Timer 1, both with a low priority interrupt. Also the internal comparator 1 is used.

The following hardware pins on the microcontroller are used:
  - RA0: comparator 1, inverting input (C1IN-)
  - RA3: comparator 1, non-inverting input (C1IN+)
  - RA4: comparator 1, output (C1OUT)
  - RC6: LocoNet transmitter (EUSART 1, TXD)
  - RC7: LocoNet receiver (EUSART 1, RXD)

The pins RA5, RE0 and RE1 are used as indication LEDs, where:
  - RA5: data on LocoNet
  - RE0: LocoNet driver in RX mode (optional, set activation in header file)
  - RE1: LocoNet driver in TX mode (optional, set activation in header file)

Include this library into your (LocoNet) project.
 - To transmit a LocoNet message, the function lnTxMessageHandler(lnMessage*) can be invoked.
 - To receive a LocoNet message, a lnRxMessageHandler(lnMessage*) callback function must be included.