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

#include <stdio.h> // Standard input / output
#include <stdlib.h> // Standard input / output
#include <termios.h> // POSIX terminal control defs
#include <unistd.h> // UNIX standard function defs, close()
#include <errno.h> // Error number definitions
#include <string.h> // strerror()
#include <stdint.h> // for uint8_t
#include <time.h> // for time()

#include <sys/ioctl.h> // ioctl (serial pins, mouse exclusive access)

#include "serial.h"

uint8_t pkt_intellimouse_intro[] = "\x4D\x5A";
 
/*** Serial comms ***/

/* Write to serial out with enforced order, otherwise we may have bytes flipped */
int serial_write(int fd, uint8_t *buffer, int size) { 
  int written=0;
  for(int i=0; i < size; i++) {
    write(fd, &buffer[i], 1);
    written++;
  }  
  return written;
}

int get_pin(int fd, int flag) {
  int serial_state = 0;
  if(ioctl(fd, TIOCMGET, &serial_state) < 0) {
    printf("get_pin(%d) failed: %d: %s\n", flag, errno, strerror(errno));
    return -1;
  }
  return (serial_state & flag) ? 1 : 0; // check bits set?
}

int enable_pin(int fd, int flag) {
  int result = ioctl(fd, TIOCMBIS, &flag); // set
  if(result < 0) { 
    printf("enable_pin(%d) failed: %d: %s\n", flag, errno, strerror(errno)); 
    return -1;
  }
  return 0;
}

int disable_pin(int fd, int flag) {
  int result = ioctl(fd, TIOCMBIC, &flag); // clear
  if(result < 0) { 
    printf("disable_pin(%d) failed: %d: %s\n", flag, errno, strerror(errno)); 
    return -1;
  }
  return 0;
}

int setup_tty(int fd, speed_t baudrate) {
  struct termios tty;
  tcgetattr(fd, &tty);

  /* Set baud rate */
  cfsetospeed(&tty, (speed_t)baudrate); // tty needs to be pointer
  cfsetispeed(&tty, (speed_t)baudrate); // tty needs to be pointer
  
  cfmakeraw(&tty); // Make tty raw, needs to be pointer

  /* Setting other Port Stuff, note: "->" for pointer, "." for direct ref */
  tty.c_cflag     &=  ~PARENB;            // Make 7n1
  tty.c_cflag     &=  ~CSTOPB;            // 1 stop bit
  tty.c_cflag     &=  ~CSIZE;
  tty.c_cflag     |=  CS7;                // CS7=7bit, CS8=8bit
  
  tty.c_cflag     &=  ~CRTSCTS;           // no flow control
  tty.c_cc[VMIN]   =  1;                  // read doesn't block  (optional)
  tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout  (optional)
  tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines  (optional)

  /* Flush Port, then applies attributes */
  if (tcsetattr (fd, TCSANOW, &tty) != 0) { // tty needs to be pointer
    printf("tcflush() failed: %d: %s\n", errno, strerror(errno));
    return -1;
  }
  return 0;
}

void wait_pin_state(int fd, int flag, int desired_state) {
  // Monitor
  int pin_state = -1;
  while(pin_state != desired_state) { 
    pin_state = get_pin(fd, flag);
    usleep(1);
  }
}

void mouse_ident(int fd, int wheel_enabled, int immediate) {
  /*** Microsoft Mouse proto negotiation ***/
  if(!immediate) {
    wait_pin_state(fd, TIOCM_CTS | TIOCM_DSR, 0); // Computers RTS & DTR
    wait_pin_state(fd, TIOCM_CTS, 1); 
    usleep(14); // Simulate real mouse start up.
  }
  /* Byte1:Always M
   * Byte2:[None]=Microsoft 3=Logitech Z=MicrosoftWheel  */
  //uint8_t logitech[] = "\x4D\x33";
  //uint8_t microsoft[] = "\x4D";
  /* IntelliMouse: MZ@... */
  //uint8_t pkt_intellimouse_intro[] = "\x4D\x5A"; // MZ

  if(wheel_enabled) {
    serial_write(fd, pkt_intellimouse_intro, sizeof(pkt_intellimouse_intro)); // 2 byte intro is sufficient
  }
  else {
    serial_write(fd, pkt_intellimouse_intro, 1); // M for basic Microsoft proto. 
  }
}

void timespec_diff(struct timespec *ts1, struct timespec *ts2, struct timespec *result) {
  result->tv_sec  = ts1->tv_sec  - ts2->tv_sec;
  result->tv_nsec = ts1->tv_nsec - ts2->tv_nsec;
  if(result->tv_nsec < 0) {
    result->tv_sec--;
    result->tv_nsec += NS_FULL_SECOND;
  }
}

struct timespec get_target_time(uint32_t delay) {
  struct timespec time;
  clock_gettime(CLOCK_MONOTONIC, &time);

  // 1200 baud (bits/s) is 133.333333333... bytes/s
  // 44.44.. updates per second with 3 bytes.
  // 33.25.. updates per second with 4 bytes.
  // ~0.0075 seconds per byte, target time calculated for 4 bytes.

  uint32_t now_nsec; 
  now_nsec = time.tv_nsec;
  time.tv_nsec = (time.tv_nsec + delay) % NS_FULL_SECOND;
  if(time.tv_nsec < now_nsec) { time.tv_sec++; }

  return time;
}
