//======================================================== file = crc32.c =====
//=  Program to compute CRC-32 using the "table method" for 8-bit subtracts   =
//=============================================================================
//=  Notes: Uses the standard "Charles Michael Heard" code available from     =
//=         http://cell-relay.indiana.edu/cell-relay/publications/software    =
//=         /CRC which was adapted from the algorithm described by Avarm      =
//=         Perez, "Byte-wise CRC Calculations," IEEE Micro 3, 40 (1983).     =
//=---------------------------------------------------------------------------=
//=  Build:  bcc32 crc32.c, gcc crc32.c                                       =
//=---------------------------------------------------------------------------=
//=  History:  KJC (8/24/00) - Genesis (from Heard code, see above)           =
//=============================================================================
//----- Include files ---------------------------------------------------------
#include <stdio.h>                  // Needed for printf()
#include <stdlib.h>                 // Needed for rand()

//----- Type defines ----------------------------------------------------------
typedef unsigned char      byte;    // Byte is a char
typedef unsigned short int word16;  // 16-bit word is a short int
typedef unsigned int       word32;  // 32-bit word is an int

//----- Defines ---------------------------------------------------------------
#define POLYNOMIAL 0x04c11db7L      // Standard CRC-32 ppolynomial
#define BUFFER_LEN       4096L      // Length of buffer

//----- Gloabl variables ------------------------------------------------------
static word32 crc_table[256];       // Table of 8-bit remainders

//----- Prototypes ------------------------------------------------------------
void gen_crc_table(void);
word32 update_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size);
int* get_table_vals(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size);
int new_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size, int num_data[]);

//===== Main program ==========================================================
int main(void)
{
  byte        buff[8]; // Buffer of packet bytes
  word32      crc32;            // 32-bit CRC value
  word16      i;                // Loop counter (16 bit)
  word32      j;                // Loop counter (32 bit)
  int *buff1;


  // insert the word into the buffer
  buff[0] = 'T';
  buff[1] = 'R';
  buff[2] = 'A';
  buff[3] = 'N';
  buff[4] = 'S';
  buff[5] = 'F';
  buff[6] = 'E';
  buff[7] = 'R';

  // Initialize the CRC table
  gen_crc_table();

  // Load buffer with BUFFER_LEN random bytes
  // for (i=0; i<BUFFER_LEN; i++)
  //   buff[i] = (byte) rand();

  // Compute and output CRC
  crc32 = update_crc(-1, buff, 8);
  buff1 = get_table_vals(-1,buff,8);


//   int letter = 65;
//   int max_letter = 90;
//   byte buff[8];

  for (int a = 65; a <= 90; a++){
    for (int b = 65; b <= 90; b++){
      for (int c = 65; c <= 90; c++){
        for (int d = 65; d <= 90; d++){
          for (int e = 65; e <= 90; e++){
            for (int f = 65; f <= 90; f++){
              for (int g = 65; g <= 90; g++){
                for (int h = 65; h <= 90; h++){
                  buff[0] = a;
                  buff[1] = b;
                  buff[2] = c;
                  buff[3] = d;
                  buff[4] = e;
                  buff[5] = f;
                  buff[6] = g;
                  buff[7] = h;
                  if (crc32 == update_crc(-1,buff,8)){
                    printf("%s\n",buff);
                    return 0;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  printf("CRC = %08X \n", crc32);
  //try_and_find_other_value(crc32);
}

//=============================================================================
//=  CRC32 table initialization                                               =
//=============================================================================
void gen_crc_table(void)
{
  register word16 i, j;
  register word32 crc_accum;

  for (i=0;  i<256;  i++)
  {
    crc_accum = ( (word32) i << 24 );
    for ( j = 0;  j < 8;  j++ )
    {
      if ( crc_accum & 0x80000000L )
        crc_accum = (crc_accum << 1) ^ POLYNOMIAL;
      else
        crc_accum = (crc_accum << 1);
    }
    crc_table[i] = crc_accum;
  }
}

//=============================================================================
//=  CRC32 generation                                                         =
//=============================================================================
word32 update_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size)
{
   register word32 i, j;

   for (j=0; j<data_blk_size; j++)
   {
     i = ((int) (crc_accum >> 24) ^ *data_blk_ptr++) & 0xFF;
     crc_accum = (crc_accum << 8) ^ crc_table[i];
   }
   crc_accum = ~crc_accum;

   return crc_accum;
}



int* get_table_vals(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size)
{
   register word32 i, j;
   static int buffer[8];
  //  int tableVals[8];

   for (j=0; j<data_blk_size; j++)
   {
     i = ((int) (crc_accum >> 24) ^ *data_blk_ptr++) & 0xFF;
     buffer[j] = i;
     //printf("%d\n",i);
     crc_accum = (crc_accum << 8) ^ crc_table[i];
     //printf("%d\n",crc_accum);
   }
   crc_accum = ~crc_accum;

  return &buffer[0];
}

int new_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size, int num_data[])
{
   register word32 i, j;

   for (j=0; j<data_blk_size; j++)
   {
     i = ((int) (crc_accum >> 24) ^ *data_blk_ptr++) & 0xFF;
     if (i != *num_data++){
       return 0;
     }
     crc_accum = (crc_accum << 8) ^ crc_table[i];
   }
   crc_accum = ~crc_accum;

   return 1;
}

// void try_and_find_other_value(word32 crc32){
//   int letter = 65;
//   int max_letter = 90;
//   byte buff[8];

//   while(1){
//     for(int i = 0; i < 8; i++){
//       buff[i] = rand() % (90 + 1 - 65) + 65;
//     }
//     // printf("%s\n",buff);
//     // printf("%c\n",buff[7]);
//     if(crc32 == update_crc(-1,buff,7)){
//       printf("%s\n",buff);
//       return;
//     }
//   }

