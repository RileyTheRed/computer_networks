//==================================================== file = knockServer.c ===
//=  Receives port knock sequence from client and opens weblite1 if verified  =
//=============================================================================
//=  Notes:                                                                   =
//=    1) This program conditionally compiles for Winsock and BSD sockets.    =
//=       Set the initial #define to WIN or BSD as appropriate.               =
//=---------------------------------------------------------------------------=
//=  Example execution: () = (knockServer and Client running on separate      =
//=                        = machines)                                        =
//=     Waiting for knock from client                                         =
//=     (If knock sequence is correct)                                        =
//=     >>> weblite is runnning on port 8080 <<<                              =
//=     diff = 1                                                              =
//=     diff = 2                                                              =
//=     diff = 3                                                              =
//=     .                                                                     =
//=     .                                                                     =
//=     .                                                                     =
//=---------------------------------------------------------------------------=
//=  Build:                                                                   =
//=    Windows (WIN):  Borland: bcc32 udpServer.c                             =
//=                    MinGW: gcc udpServer.c -lws2_32 -o updServer           =
//=                    Visual C: cl ucpServer.c wsock32.lib                   =
//=    Unix/Mac (BSD): gcc knockServer.c -lnsl -o server                      =
//=---------------------------------------------------------------------------=
//=  Execute: ./server                                                        =
//=---------------------------------------------------------------------------=
//=  Author:                                                                  =
//=          Timothy Carney                                                   =
//=          University of South Florida                                      =
//=          Email: tfcarney@mail.usf.edu                                     =
//=                                                                           =
//=          Riley Wells                                                      =
//=          University of South Florida                                      =
//=          Email: rww@mail.usf.edu                                          =
//=                                                                           =
//=          (Author of base code and CRC32)                                  =
//=          Ken Christensen                                                  =
//=          University of South Florida                                      =
//=          WWW: http://www.csee.usf.edu/~christen                           =
//=          Email: christen@csee.usf.edu                                     =
//=---------------------------------------------------------------------------=
//=  History:  KJC (08/02/08) - Genesis (from server1.c)                      =
//=            KJC (09/07/09) - Minor clean-up                                =
//=            KJC (09/22/13) - Minor clean-up to fix warnings                =
//=            KJC (09/14/17) - Updated build instructions                    =
//=            TFC (11/13/18) - Removed unnecessary functions                 =
//=            TFC (11/15/18) - Added hashing function and password checks    =
//=            TFC (11/18/18) - Added fork and kill functions                 =
//=            RWW (11/25-26) - Added shared memory segments to be used to    =
//=                             in timeout and weblite functions/processes.   =
//=                             Created check_time() function that is run in  =
//=                             a separate process to determine if the server =
//=                             has timed out and if so kills the web server. =
//=============================================================================
#define  BSD                // WIN for Winsock and BSD for BSD sockets

//----- Include files --------------------------------------------------------
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
  #include <sys/shm.h>
  #include <sys/ipc.h>
  #include <sys/sem.h>
  #include <errno.h>
  #include <netdb.h>        // Needed for sockets stuff
  #include <unistd.h>       // Needed for exec
  #include <signal.h>       // Needed for kill signal
  #include <sys/time.h>     // For getting time values
  #include <stdio.h>        // Needed for processing
  #include <stdlib.h>       // C standard library
#endif

//----- Defines --------------------------------------------------------------
#define  PORT_NUM   1050            // Arbitrary port number for the server
#define  PORT_NUM2  1051            // Second port to listen to
#define  PORT_NUM3  1052            // Third port to listen to
#define  POLYNOMIAL 0x04c11db7L     // Standard CRC-32 ppolynomial
#define  BUFFER_LEN 4096L           // Length of buffer
#define  SHMKEY_1   ((key_t) 1497)  // Shared memory key 1
#define  SHMKEY_2   ((key_t) 1498)  // Shared memory key 2



// definition of the shared memory struct
typedef struct{
    pid_t value;
} shared_pid;

// definition of the shared memory struct
typedef struct{
    time_t last;
} shared_time;

time_t last_called;
pid_t monitor_pid;
//pid_t pid = -1;
int status;
int hasBeenSet = 0;
pid_t parent;


//-----Typedefs for Hashing----------------------------------------------------
typedef unsigned char      byte;    // Byte is a char
typedef unsigned short int word16;  // 16-bit word is a short int
typedef unsigned int       word32;  // 32-bit word is an int

//----- Gloabl variables ------------------------------------------------------
static word32 crc_table[256];       // Table of 8-bit remainders

//-----Hashing Prototypes------------------------------------------------------
void gen_crc_table(void);
word32 update_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size);
void check_time();

//===== Main program =========================================================
int main()
{
#ifdef WIN
  WORD wVersionRequested = MAKEWORD(1,1);       // Stuff for WSA functions
  WSADATA wsaData;                              // Stuff for WSA functions
#endif
  int                  server_s;        // Server socket descriptor
  int                  server_s2;       // Second socket descriptor
  int                  server_s3;       // Third  socket descriptor
  struct sockaddr_in   server_addr;     // Server Internet address
  struct sockaddr_in   client_addr;     // Client Internet address
  struct in_addr       client_ip_addr;  // Client IP address
  int                  addr_len;        // Internet address length
  char                 out_buf[4096];   // Output buffer for data
  char                 in_buf[4096];    // Input buffer for data
  int                  retcode;         // Return code

    shared_pid *shpid;
    shared_time *shtime;


    char *shmadd = 0;
    int shmid_1, shmid_2;
    

    // check to make sure that the shared memory is created or already created
    if ((shmid_1 = shmget(SHMKEY_1, sizeof(pid_t), IPC_CREAT | 0666)) < 0){
        perror("shmget");
        exit(1);
    }
    // check to make sure that the shared memory is created or already created
    if ((shmid_2 = shmget(SHMKEY_2, sizeof(time_t), IPC_CREAT | 0666)) < 0){
        perror("shmget");
        exit(1);
    }

    // check to make sure that the shared memory is accessible
    if ((shpid = (shared_pid *) shmat(shmid_1,shmadd,0)) == (shared_pid *) -1){
        perror("shmat");
        exit(0);
    }
    if ((shtime = (shared_time *) shmat(shmid_2,shmadd,0)) == (shared_time *) -1){
        perror("shmat");
        exit(0);
    }





    shpid->value = -1;


    shtime->last = time(NULL);


    parent = getpid();

#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif
    fflush(stdout);
    if ((monitor_pid = fork()) == 0){
      check_time();
    }

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

  // Create sockets to receive port knocking sequence
  //   - AF_INET is Address Family Internet and SOCK_DGRAM is datagram
  server_s = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_s < 0)
  {
    printf("*** ERROR - socket() failed \n");
    exit(-1);
  }

  server_s2 = socket(AF_INET, SOCK_DGRAM, 0);
  if(server_s2 < 0)
  {
    printf("*** ERROR - socket 2 () failed \n");
    exit(-1);
  }

  server_s3 = socket(AF_INET, SOCK_DGRAM, 0);
  if(server_s3 < 0)
  {
    printf("*** ERROR - socket 3 () failed \n");
    exit(-1);
  }

  // Fill in sockets' address information
  server_addr.sin_family = AF_INET;                 // Address family to use
  server_addr.sin_port = htons(PORT_NUM);           // Port number to use
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen on any IP address
  retcode = bind(server_s, (struct sockaddr *)&server_addr,
    sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind() failed \n");
    exit(-1);
  }

  server_addr.sin_port = htons(PORT_NUM2);

  retcode = bind(server_s2, (struct sockaddr *)&server_addr,
      sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind 2 () failed \n");
    exit(-1);
  }

  server_addr.sin_port = htons(PORT_NUM3);

  retcode = bind(server_s3, (struct sockaddr *)&server_addr,
    sizeof(server_addr));
  if (retcode < 0)
  {
    printf("*** ERROR - bind 3 () failed \n");
    exit(-1);
  }

  addr_len = sizeof(client_addr);
  
  fflush(stdout);
  printf("Waiting for knock from client\n");

  pid_t pid = -1;


  // While loop to continually receive port knocks
  while(1){

    /* wait for the first message from the client. If invalid, go to start of loop. */
    retcode = recvfrom(server_s, in_buf, sizeof(in_buf)+1, 0,
    (struct sockaddr *)&client_addr, &addr_len);
    if (retcode < 0)
    {
      printf("*** ERROR - recvfrom() failed \n");
      exit(-1);
    } 
    if (strcmp(pass1, in_buf) != 0 && strcmp(pass2, in_buf) != 0 && strcmp(pass3, in_buf) != 0){
      
      continue;
      
    }

   

    fflush(stdout);

    /* wait for the second message from the client. If invalid, go to start of loop. */
    retcode = recvfrom(server_s2, in_buf, sizeof(in_buf)+1, 0,
    (struct sockaddr *)&client_addr, &addr_len);
    if (retcode < 0)
    {
      printf("*** ERROR - recvfrom 2 () failed \n");
      exit(-1);
    } 
    if (strcmp(pass1, in_buf) !=0 && strcmp(pass2, in_buf) != 0 && strcmp(pass3, in_buf) != 0){
      
      continue;
      
    }


    fflush(stdout);

    /* wait for the third message from the client. If invalid, go to start of loop. */
    retcode = recvfrom(server_s3, in_buf, sizeof(in_buf)+1, 0,
    (struct sockaddr *)&client_addr, &addr_len);
    if (retcode < 0)
    {
      printf("*** ERROR - recvfrom 3 () failed \n");
      exit(-1);
    } 
    if (strcmp(pass1, in_buf) != 0 && strcmp(pass2, in_buf) != 0 && strcmp(pass3, in_buf) != 0){
      
      continue;
      
    }

    fflush(stdout);
   
    // If port knocking sequence is valid, fork the server and exec the weblite1.c server
    pid = fork();
    if(pid < 0)
    {
        printf("fork failed\n");
        exit(1);
    }
    else if(pid == 0 && shpid->value == -1)
    {
        printf("got to pid == 0\n");
        //hasBeenSet = 1;
        shpid->value = getpid();
  	    shtime->last = time(NULL);
        printf("starting server again\n");
        execlp("./weblite", "./weblite", (char *)NULL);
    }  
    else
    {
        if (waitpid(pid,&status,WNOHANG) == 0){
            //printf("last time = %d\n", last_called);
            //printf("current time = %d\n", time(NULL));
	          shtime->last = time(NULL);
            //last_called = time(NULL);
        }
    }

  }

  // Close all open sockets
  #ifdef WIN
  retcode = closesocket(server_s);
  if (retcode < 0)
  {
    printf("*** ERROR - closesocket() failed \n");
    exit(-1);
  }

  retcode = closesocket(server_s2);
  if (retcode < 0)
  {
    printf("*** ERROR - closesocket 2 () failed \n");
    exit(-1);
  }

  retcode = closesocket(server_s3);
  if (retcode < 0)
  {
    printf("*** ERROR - closesocket 3 () failed \n");
    exit(-1);
  }


#endif
#ifdef BSD
  retcode = close(server_s);
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

// Function to check the time weblite has been running through use of shared memory 
void check_time(){
    int shmid_1 = shmget(SHMKEY_1, sizeof(pid_t), IPC_CREAT | 0666);
    shared_pid *temp_pid = shmat(shmid_1, (char *)0,0);

    int shmid_2 = shmget(SHMKEY_2, sizeof(time_t), IPC_CREAT | 0666);
    shared_time *temp_time = shmat(shmid_2, (char *)0,0);

  
    while(1){
        sleep(1);
        printf("diff = %d\n",(int)difftime(time(NULL),temp_time->last));
	
	      if ((int)difftime(time(NULL),temp_time->last) >= 10){
	        if (temp_pid->value != -1){
		        kill(temp_pid->value, SIGKILL);
		        temp_pid->value = -1;
	        }
       	}
     }
}






