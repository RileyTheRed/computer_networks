//==================================================== file = udpClient.c =====
//=  A message "client" program to demonstrate sockets programming            =
//=============================================================================
//=  Notes:                                                                   =
//=    1) This program conditionally compiles for Winsock and BSD sockets.    =
//=       Set the initial #define to WIN or BSD as appropriate.               =
//=    2) This program needs udpServer to be running on another host.         =
//=       Program udpServer must be started first.                            =
//=    3) This program assumes that the IP address of the host running        =
//=       udpServer is defined in "#define IP_ADDR"                           =
//=    4) The steps #'s correspond to lecture topics.                         =
//=---------------------------------------------------------------------------=
//=  Example execution: (udpServer and udpClient running on host 127.0.0.1)   =
//=    Received from server: This is a reply message from SERVER to CLIENT    =
//=---------------------------------------------------------------------------=
//=  Build:                                                                   =
//=    Windows (WIN):  Borland: bcc32 udpClient.c                             =
//=                    MinGW: gcc udpClient.c -lws2_32 -o updClient           =
//=                    Visual C: cl ucpClient.c wsock32.lib                   =
//=    Unix/Mac (BSD): gcc ucpClient.c -lnsl -o ucpClient                     =
//=---------------------------------------------------------------------------=
//=  Execute: udpClient                                                       =
//=---------------------------------------------------------------------------=
//=  Author: Ken Christensen                                                  =
//=          University of South Florida                                      =
//=          WWW: http://www.csee.usf.edu/~christen                           =
//=          Email: christen@csee.usf.edu                                     =
//=---------------------------------------------------------------------------=
//=  History:  KJC (08/02/08) - Genesis (from client.c)                       =
//=            KJC (09/09/09) - Minor clean-up                                =
//=            KJC (09/22/13) - Minor clean-up to fix warnings                =
//=            KJC (09/14/17) - Updated build instructions                    =
//=============================================================================
#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files ---------------------------------------------------------
#include <stdio.h>          // Needed for printf()
#include <string.h>         // Needed for memcpy() and strcpy()
#include <stdlib.h>         // Needed for exit()
#ifdef WIN
  #include <windows.h>      // Needed for all Winsock stuff
#endif
#ifdef BSD
  #include <sys/types.h>    // Needed for sockets stuff
  #include <netinet/in.h>   // Needed for sockets stuff
  #include <sys/socket.h>   // Needed for sockets stuff
  #include <arpa/inet.h>    // Needed for sockets stuff
  #include <fcntl.h>        // Needed for sockets stuff
  #include <netdb.h>        // Needed for sockets stuff
#endif

//----- Defines ---------------------------------------------------------------
#define STD_PORT             1050   // First Port to knock
#define STD_PORT2            1051   // Second Port to knock
#define STD_PORT3            1052   // Third Port to knock
#define PORT_NUM            "1234"  // Port number used
#define PORT_NUM_2          "4321"
#define PORT_NUM_3          "5555"
#define  IP_ADDR            "127.0.0.1" // IP address of server1 (*** HARDWIRED ***)
#define POLYNOMIAL 0x04c11db7L      // Standard CRC-32 ppolynomial
#define BUFFER_LEN       4096L      // Length of buffer

//-----Typedefs for Hashing----------------------------------------------------
typedef unsigned char      byte;    // Byte is a char
typedef unsigned short int word16;  // 16-bit word is a short int
typedef unsigned int       word32;  // 32-bit word is an int


//----- Gloabl variables ------------------------------------------------------
static word32 crc_table[256];       // Table of 8-bit remainders

//-----Hashing Prototypes------------------------------------------------------
void gen_crc_table(void);
word32 update_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size);

//===== Main program ==========================================================
int main()
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(1,1);       // Stuff for WSA functions
  WSADATA wsaData;                              // Stuff for WSA functions
#endif
  int                  client_s;        // Client socket descriptor
  struct sockaddr_in   server_addr;     // Server Internet address
  int                  addr_len;        // Internet address length
  char                 out_buf[4096];   // Output buffer for data
  char                 in_buf[4096];    // Input buffer for data
  int                  retcode;         // Return code

#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif

  //Initialize hashing variables
  byte        buff[9];                  // Buffer of packet bytes
  word32      p1, p2, p3;               // 32-bit CRC value
  word16      i;                        // Loop counter (16 bit)
  word32      j;                        // Loop counter (32 bit)

  // Initialize the CRC table
  gen_crc_table();

  // Calculate hashed password values
  strcpy(buff, "password");
  p1 = update_crc(-1, buff, 8);
  char pass1[9];
  sprintf(pass1, "%X", p1);

  strcpy(buff, "networks");
  p2 = update_crc(-1, buff, 8);
  char pass2[9];
  sprintf(pass2, "%X", p2);

  char pass3[9];
  strcpy(buff, "usfbulls");
  p3 = update_crc(-1, buff, 8);
  sprintf(pass3, "%X", p3);
  


  // >>> Step #1 <<<
  // Create a socket
  //   - AF_INET is Address Family Internet and SOCK_DGRAM is datagram
  client_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_s < 0)
  {
    printf("*** ERROR - socket() failed \n");
    exit(-1);
  }

  // >>> Step #2 <<<
  // Fill-in server1 socket's address information
  server_addr.sin_family = AF_INET;                 // Address family to use
  server_addr.sin_port = htons(STD_PORT);           // Port num to use
  server_addr.sin_addr.s_addr = inet_addr(IP_ADDR); // IP address to use

  // Assign a message to buffer out_buf
  strcpy(out_buf, pass1);

  // >>> Step #3 <<<
  // Now send the message to server.  The "+ 1" is for the end-of-string
  // delimiter
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
    (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - sendto() failed \n");
    exit(-1);
  }

 

  server_addr.sin_port = htons(STD_PORT2);           // Port num to use

  strcpy(out_buf, pass1);
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
    (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - sendto 2 () failed \n");
    exit(-1);
  }

  server_addr.sin_port = htons(STD_PORT3);           // Port num to use

 


  strcpy(out_buf, pass1);
  retcode = sendto(client_s, out_buf, (strlen(out_buf) + 1), 0,
    (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - sendto 3 () failed \n");
    exit(-1);
  }


 

  // >>> Step #5 <<<
  // Close all open sockets
#ifdef WIN
  retcode = closesocket(client_s);
  if (retcode < 0)
  {
    printf("*** ERROR - closesocket() failed \n");
    exit(-1);
  }
#endif
#ifdef BSD
  retcode = close(client_s);
  if (retcode < 0)
  {
    printf("*** ERROR - close() failed \n");
    exit(-1);
  }
#endif





#ifdef WIN
  // This stuff cleans-up winsock
  WSACleanup();
#endif

  // Return zero and terminate
  return(0);
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


