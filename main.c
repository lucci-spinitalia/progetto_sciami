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

/* Prototype */
void velocity_packet_send(__u8 direction_left, __u8 direction_right, __u16 vel_left, __u16 vel_right, __u8 motor_on);
int calculate_crc(char *message, int length);
int load_device_tree_file(char *name);
int gpio_export(int pin_number);
int gpio_set_value(int pin_number, int value);
int gpio_set_direction(int pin_number, int value);
int gpio_generic_set_value(char *path, int value);

// Gps socket to communicate with CCU
int saetta_device = -1;
  
/* Generic */

void signal_handler(int signum)
{
  // Garbage collection
  printf("Terminating program...\n");
  
  close(saetta_device);
  exit(signum);
}

int main(int argc, char **argv) 
{
  /* Gps */

  // Gps rs232 device
  char gps_device_buffer[RS232_BUFFER_SIZE];

  /* Generic Variable */
  int done = 0; // for the while in main loop
  int bytes_read; // to check how many bytes has been read
  int bytes_sent;

  int i;

  int select_result = -1; // value returned frome select()
  int nfds = 0; // fd to pass to select()
  fd_set rd, wr, er; // structure for select()
  struct timeval select_timeout;
  long current_timeout = 0;
  
  printf("Initializing. . .\n");
    
  /* Peripheral initialization */

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // enable uart2
  load_device_tree_file("enable-uart2");
  
  saetta_device = com_open("/dev/ttyO2", 115200, 'N', 8, 1);
  
  if(saetta_device < 0)
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
    
    if(saetta_device > 0)
    {
      FD_SET(saetta_device, &rd);
      nfds = max(nfds, saetta_device);

      if(rs232_buffer_tx_empty == 0)
      {
    	FD_SET(saetta_device, &wr);
    	nfds = max(nfds, saetta_device);
      }
    }
    
    select_result = select(nfds + 1, &rd, &wr, NULL, NULL);

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
    if(saetta_device > 0)
    {
      if(FD_ISSET(saetta_device, &rd))
      {
        bytes_read = rs232_read(saetta_device);

        if((bytes_read > 0) || ((bytes_read < 0) && rs232_buffer_rx_full))
        {
          bytes_read = rs232_unload_rx_filtered(gps_device_buffer, 0x0A);

          if(bytes_read > 0)
          {
            gps_device_buffer[bytes_read] = '\0';

            if(strncmp(gps_device_buffer, "STARTr", strlen("STARTr") - 1) == 0)
            {
              printf("catch\n");
              velocity_packet_send(1, 0, 1024, 1024, 1);
            }
            else
            {
              for(i = 0; i < bytes_read; i++)
                printf("%x ", gps_device_buffer[i]);

              printf("\n");
            }
          }
        }
      }

      if(FD_ISSET(saetta_device, &wr))
      {
        if(rs232_write(saetta_device) < 0)
          printf("Error on rs232_write");
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

int gpio_export(int pin_number)
{
  FILE *file = NULL;

  file = fopen("/sys/class/gpio/export", "a");

  if(file == NULL)
    return -1;

  fprintf(file, "%i", pin_number);

  fclose(file);
  return 1;
}

int gpio_set_value(int pin_number, int value)
{
  FILE *file = NULL;
  char file_path[64];

  sprintf(file_path, "/sys/class/gpio/gpio%i/value", pin_number);
  file = fopen(file_path, "a");

  if(file == NULL)
    return -1;

  fprintf(file, "%i", value);

  fclose(file);
  return 1;
}

int gpio_set_direction(int pin_number, int value)
{
  FILE *file = NULL;
  char file_path[64];

  sprintf(file_path, "/sys/class/gpio/gpio%i/direction", pin_number);
  file = fopen(file_path, "a");

  if(file == NULL)
    return -1;

  fprintf(file, "%i", value);

  fclose(file);
  return 1;
}

int gpio_generic_set_value(char *path, int value)
{
  FILE *file = NULL;

  file = fopen(path, "a");

  if(file == NULL)
    return -1;

  fprintf(file, "%i", value);

  fclose(file);
  return 1;
}

int load_device_tree_file(char *name)
{
  FILE *file = NULL;

  file = fopen("/sys/devices/bone_capemgr.8/slots", "w");

  if(file == NULL)
    return -1;

  fprintf(file, "%s", name);

  fclose(file);
  return 1;
}

/*
 * Questa funzione si occupa di mandare un messaggio per il comando della velocità del robot
 * saetta. La struttura del pacchetto da inviare è la seguente
 *
 * | 0x7F | direzione sinistra | direzione destra | velocità sinistra lsb | velocità sinistra msb | velocità destra lsb | velocità destra msb | abilitazione motori | crc | 0x0A |
 *
 * I parametri accettati in ingresso sono le direzioni dei motori, le velocità ed il flag di abilitazione
 */
void velocity_packet_send(__u8 direction_left, __u8 direction_right, __u16 vel_left, __u16 vel_right, __u8 motor_on)
{
  char packet_to_send[11];
  long crc = 0;

  packet_to_send[0] = 0x7F;
  packet_to_send[1] = direction_left;
  packet_to_send[2] = direction_right;
  packet_to_send[3] = vel_left & 0xff;
  packet_to_send[4] = vel_left >> 8;
  packet_to_send[5] = vel_right & 0xff;
  packet_to_send[6] = vel_right >> 8;

  if(motor_on > 0)
	packet_to_send[7] = 1;
  else
	packet_to_send[7] = 0;

  crc = calculate_crc(packet_to_send, sizeof(packet_to_send));

  packet_to_send[8] = crc & 0xff;
  packet_to_send[9] = crc >> 8;
  packet_to_send[10] = 0x0a;

  rs232_load_tx((unsigned char *)packet_to_send, sizeof(packet_to_send));
}

int calculate_crc(char *message, int length)
{
  int k = 0;
  long crc = 0;
  const int length_crc = 2;

  for(k = 0; k < length - (length_crc + 1); k++)
    crc += message[k];

  crc = ~crc + 1;

  return crc;
}
