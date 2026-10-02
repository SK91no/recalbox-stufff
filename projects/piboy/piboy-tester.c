// SPDX-License-Identifier: GPL-3.0
// piboy-tester

#ifdef RPI3
#define BCM27XX_PERI_BASE        0x3F000000
#endif
#ifdef RPI4
#define BCM27XX_PERI_BASE        0xFE000000
#endif
#define GPIO_BASE                (BCM27XX_PERI_BASE + 0x200000) /* GPIO controller */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define BLOCK_SIZE (4*1024)
#define GC_LENGTH_DMG 12
#define GC_LENGTH_XRS 14
#define BITRATE 7
#define MAX_RETRIES 10
#define EXIT_SUCCESS 0
#define EXIT_ERROR 1

int  mem_fd;
void *gpio_map;
int GC_LENGTH;
static const int gc_gpio_clk = 26;
static const int gc_gpio_data = 27;
static const int gc_clk_bit = 1<<26;
static const int gc_data_bit = 1<<27;

// I/O access
volatile unsigned *gpio;

typedef enum {
  CASE_DMG,
  CASE_XRS,
  CASE_UNKNOWN,
} piboy_case;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0
#define GPIO_STATUS (*(gpio+13))

void setup_io();
void calc_crc16(uint8_t *data, uint8_t len)
{
  int i,j;
  uint16_t crc=0;

  for(i = 0;i<len;i++){
    crc = (uint16_t)(crc ^ ((uint16_t)data[i] << 8));
    for (j=0; j<8; j++){
      if ((crc & 0x8000)!=0) crc = (uint16_t)((crc << 1) ^ 0x1021);
      else crc <<= 1;
    }
  }
  data[len] = crc>>8;
  data[len+1] = crc&0xFF;
}

uint16_t check_crc16(uint8_t data[], int data_length)
{
  int len = data_length-2;
  uint16_t crc=0;
  uint16_t ccrc = (data[data_length-1]<<8) | data[data_length-2];
  int i,j;

  for(i = 0;i<len;i++){
    crc = (uint16_t)(crc ^ ((uint16_t)data[i] << 8));
    for (j=0; j<8; j++){
      if ((crc & 0x8000)!=0) crc = (uint16_t)((crc << 1) ^ 0x1021);
      else crc <<= 1;
    }
  }

  return crc==ccrc ? 0 : 1;
}

void gpio_func(int pin, int state)
{
  volatile unsigned *tgpio = gpio;
  tgpio += (pin/10);
  if(state){*tgpio &= ~(0x7<<(pin%10)*3);  }
  else{*tgpio |= (0x1<<(pin%10)*3);}
}

// read cmd line arguments and return passed case type
// admitted case types: piboy_case 
piboy_case read_case_type(int argc, char **argv) {
  // read args
  if (argc != 2) {
    printf("Missing argument\n%s < dmg | xrs >\n", argv[0]);
    exit(1);
  }

  if (strcmp("dmg", argv[1]) == 0)
    return CASE_DMG;
  if (strcmp("xrs", argv[1]) == 0)
    return CASE_XRS;

  // sanity check
  printf("Unknown case type %s\n%s < dmg | xrs >\n", argv[1], argv[0]);
  exit(1);
}

int main(int argc, char **argv)
{
  int byteindex;
  int retry;
  int STATUS_ID;
  long bitindex;
  unsigned char data[32];
  piboy_case case_type = CASE_UNKNOWN;
  static uint8_t index;

  case_type = read_case_type(argc, argv);
  GC_LENGTH = case_type == CASE_DMG ? GC_LENGTH_DMG : GC_LENGTH_XRS;
  STATUS_ID = case_type == CASE_DMG ? 5 : 7;

  // Set up gpi pointer for direct register access
  setup_io();

  gpio_func(gc_gpio_clk,0);  //output

  for(retry=0;retry<MAX_RETRIES;retry++) {
    gpio_func(gc_gpio_data,1);  //input

    for(byteindex=0;byteindex<GC_LENGTH;byteindex++){
      data[byteindex]=0;
      for(bitindex=0;bitindex<8;bitindex++){
        data[byteindex]<<=1;

        //set clock pin
        GPIO_SET |= gc_clk_bit;
        usleep(BITRATE);
        GPIO_CLR |= gc_clk_bit;
        usleep(BITRATE);
        data[byteindex] |= GPIO_STATUS & gc_data_bit ? 1 : 0;
      }
      //printf("%d=%x\n", byteindex, data[byteindex]);
    }

    gpio_func(gc_gpio_data,0);  //output

    GPIO_SET |= gc_clk_bit;
    usleep(BITRATE);
    GPIO_CLR |= gc_clk_bit;
    usleep(BITRATE);

    if(data[0] && !check_crc16(data, GC_LENGTH)){
      uint8_t len = 0;
      switch(data[0]) {
        case 0xa5: {
          unsigned char val=0x80;
          len = 2;
          //version_val = 0x0100;
          //val = values.fan_val | (values.flags_val&0x1 ? 0x00 : 0x80);
          data[GC_LENGTH] = val;
          data[GC_LENGTH+1] = ~val;
          data[GC_LENGTH+2] = 0;
          data[GC_LENGTH+3] = 0;
        }
        case 0x5a: {
          len = 4;
          //version_val = 0x0101;
          data[GC_LENGTH+0] = 0xC0 | (index&0x3);
          data[GC_LENGTH+1] = 0;
          calc_crc16(&data[GC_LENGTH],2);
          index++;
        }
        default: {
          len = 4;
          //version_val = ((data[0]&0xC0)<<2) | (data[0]&0x3F);
          data[GC_LENGTH+0] = 0xC0 | (index&0x3);
          data[GC_LENGTH+1] = 0;
          calc_crc16(&data[GC_LENGTH],2);
          index++;
        }
      }

      for(byteindex=GC_LENGTH;byteindex<GC_LENGTH+len;byteindex++){
        for(bitindex=0;bitindex<8;bitindex++){
          if(data[byteindex]&(0x80>>bitindex))
            GPIO_SET |= gc_data_bit;
          else
            GPIO_CLR |= gc_data_bit;
          //set clock pin
          GPIO_SET |= gc_clk_bit;
          usleep(BITRATE);
          GPIO_CLR |= gc_clk_bit;
          usleep(BITRATE);
        }
      }
      // read successful
      gpio_func(gc_gpio_clk,1);  //input
      printf("%d\n", data[STATUS_ID]&0xc6);
      return EXIT_SUCCESS;
    }else {
      fprintf(stderr, "try %d, CRC error\n", retry);
    }
  }
  gpio_func(gc_gpio_clk,1);  //input
  fprintf(stderr, "piboy not found\n");
  return EXIT_ERROR;
}

//
// Set up a memory regions to access GPIO
//
void setup_io()
{
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }

   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );

   close(mem_fd); //No need to keep mem_fd open after mmap

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned *)gpio_map;


} // setup_io
