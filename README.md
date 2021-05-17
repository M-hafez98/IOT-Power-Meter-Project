# IOT Electrical Meter
This is a firmware communication protocol for GPRS modem (SIM 800) using ARM M0+ based MCU (KL34Z4), in smart meter project.
The GPRS commands are obtained from the application note of SIM 800 modem.
The UART driver is portable and can be used for any ARM-based MCU vendor.
A app.c file is added for communication test, and to run it, it is needed to comment/remove any header files (i.e .h files)
that are not included in the repository.
