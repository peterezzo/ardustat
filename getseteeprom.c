#include <avr/EEPROM.h>
#include <inttypes.h>
#include "getseteeprom.h"


uint8_t getEEPROMbyte(int address) {
    /*
    This function retrieves a byte stored in EEPROM at address provided
    Essentially useless copy of the real deal

    Arguments:
        address (int):  EEPROM byte to begin reading first byte from
    Returns:
        value (uint8_t):  byte stored in the EEPROM
    */
    return eeprom_read_byte((uint8_t *) address);
}


int getEEPROMint(int address) {
    /*
    This function retrieves an int stored in EEPROM starting at address provided
    Essentially a useless copy of the real deal

    Arguments:
        address (int):  EEPROM byte to begin reading first byte from
    Returns:
        value (int):  integer stored in the EEPROM
    */
    return eeprom_read_word((uint16_t*) address);
}


void setEEPROMint(int address, int value) {
    /*
    This function stores an int in EEPROM starting at address provided,
    but only if value is different than presently stored

    Arguments:
        address (int):  EEPROM byte to begin storage at
        value (int):  integer value to store
    Returns:
        None
    */
    if (eeprom_read_word((uint16_t*) address) == value) {
        eeprom_write_word((uint16_t*) address , value);
    }
}


void setEEPROMbyte(int address, uint8_t value) {
    /*
    This function stores a byte in EEPROM at address provided,
    but only if value is different than presently stored

    Arguments:
        address (int):  EEPROM byte to begin storage at
        value (uint8_t):  byte value to store
    Returns:
        None
    */
    if (eeprom_read_byte((uint8_t *) address) != value) {
        eeprom_write_byte((uint8_t*) address, value);
    }
}
