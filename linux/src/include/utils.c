/* 
 * Anachro Mouse, a usb to serial mouse adaptor. Copyright (C) 2021 Aviancer <oss+amouse@skyvian.me>
 *
 * This library is free software; you can redistribute it and/or modify it under the terms of the 
 * GNU Lesser General Public License as published by the Free Software Foundation; either version 
 * 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this library; 
 * if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdint.h>
#include <stdio.h>
#include "utils.h"

/*** Helper functions ****/

const char * byte_to_bitstring(uint8_t val) {
  static char buffer[9];

  for (int i = 0; 7 >= i; i++) {
    buffer[7-i] = 0x30 + ((val >> i) & 1); 
  }
 return buffer;
}

int clamp(int value, int min, int max) {
  if(value > max) { return max; }
  if(value < min) { return min; }
  return value;
}

void aprint(const char *message) {
  printf("amouse> %s\n", message);
}
