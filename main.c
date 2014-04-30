#include <sys/types.h> 
#include <sys/ioctl.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>

#include <sys/time.h> 
#include <time.h> 
#include <errno.h>
#include <locale.h>
#include <string.h>

#include <signal.h>
#include <termios.h>

#include "rs232.h"

#define TIMEOUT_SEC 0 

/* Macro */
#undef max 
#define max(x,y) ((x) > (y) ? (x) : (y))

#undef min
#define min(x,y) ((x) < (y) ? (x) : (y))

/* Prototype */

// Gps socket to communicate with CCU
int gps_device = -1;
  
/* Generic */

void signal_handler(int signum)
{
  // Garbage collection
  printf("Terminating program...\n");
  
  close(gps_device);
  exit(signum);
}

int main(int argc, char **argv) 
{
  /* Gps */

  // Gps rs232 device
  char gps_device_buffer[RS232_BUFFER_SIZE];
  char *nmea_token;
  char nmea_message[256];
  
    // timer
  long rover_time_start_hs = 0;
  long rover_time_stop_hs = 0;
  long rover_elapsed_time_hs = 0;
  
  /* Generic Variable */
  int done = 0; // for the while in main loop
  int bytes_read; // to check how many bytes has been read
  int bytes_sent;

  int select_result = -1; // value returned frome select()
  int nfds = 0; // fd to pass to select()
  fd_set rd, wr, er; // structure for select()
  struct timeval select_timeout;
  long current_timeout = 0;
  
  printf("Initializing. . .\n");
    
  /* Peripheral initialization */

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Select UART2_TX and set it as output
  gpio_generic_set_value("/sys/kernel/debug/omap_mux/spi0_d0", 11);
  
  // Select UART1_RX and set it as input pulled up
  gpio_generic_set_value("/sys/kernel/debug/omap_mux/spi0_sclk", 39);
  
  gps_device = com_open("/dev/ttyO2", 4800, 'N', 8, 1);
  
  if(gps_device < 0)
    perror("com_open");
	
  //select_timeout.tv_sec = TIMEOUT_SEC;
  //select_timeout.tv_usec = ARM_TIMEOUT_USEC;

  //current_timeout = select_timeout.tv_usec;
  
  printf("Run main program. . .\n");
 
  while(!done)
  {
    fflush(stdout);

    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&er);
    
    if(gps_device > 0)
    {
      FD_SET(gps_device, &rd);
      nfds = max(nfds, gps_device);
    }
    
    select_result = select(nfds + 1, &rd, &wr, NULL, &select_timeout);

    if(select_result == -1 && errno == EAGAIN)
    {
      perror("select");
      continue;
    }

    if(select_result == -1)
    {
      perror("main:");

      return 1;
    }

    /* Manage gps */
    if(gps_device > 0)
    {
      if(FD_ISSET(gps_device, &rd))
      {
        bytes_read = rs232_read(gps_device);
        if((bytes_read > 0) || ((bytes_read < 0) && rs232_buffer_rx_full))
        {
          bytes_read = rs232_unload_rx_filtered(gps_device_buffer, '\n');

          if(bytes_read > 0)
          {
            gps_device_buffer[bytes_read] = '\0';

            nmea_token = strtok(gps_device_buffer, "\n");

            while(nmea_token != NULL)
            {
              sprintf(nmea_message, "%s\n", nmea_token);
            }
          }
        }
      }
    }

	/* Timeout region */
    /*if((select_timeout.tv_sec == 0) && (select_timeout.tv_usec == 0))
    {
      select_timeout.tv_sec = TIMEOUT_SEC;
      select_timeout.tv_usec = ARM_TIMEOUT_USEC;

      current_timeout = select_timeout.tv_usec;
    }*/
  }  // end while(!= done)

  return 0;
}
