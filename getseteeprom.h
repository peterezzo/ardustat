#ifdef __cplusplus
extern "C" {
#endif

#ifndef GETSETEEPROM_H
#define GETSETEEPROM_H

#include <inttypes.h>

uint8_t getEEPROMbyte(int);
int getEEPROMint(int);
void setEEPROMbyte(int, uint8_t);
void setEEPROMint(int, int);

#endif /* GETSETEEPROM_H */

#ifdef __cplusplus
}
#endif