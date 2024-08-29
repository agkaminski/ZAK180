/* ZAK180 Firmaware
 * AY-3-8912 driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_AY38912_H_
#define DRIVER_AY38912_H_

unsigned char ay38912_readPort(void);

void ay38912_writePort(unsigned char byte);

void ay38912_setPort(unsigned char dir);

#endif
