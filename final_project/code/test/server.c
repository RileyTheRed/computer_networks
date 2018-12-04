//==================================================== file = udpServer.c =====
//=  A message "server" program to demonstrate sockets programming            =
//=============================================================================
//=  Notes:                                                                   =
//=    1) This program conditionally compiles for Winsock and BSD sockets.    =
//=       Set the initial #define to WIN or BSD as appropriate.               =
//=    2) This program serves a message to program udpClient running on       =
//=       another host.                                                       =
//=    3) The steps #'s correspond to lecture topics.                         =
//=---------------------------------------------------------------------------=
//=  Example execution: (udpServer and udpClient running on host 127.0.0.1)   =
//=    Waiting for recvfrom() to complete...                                  =
//=    IP address of client = 127.0.0.1  port = 55476)                        =
//=    Received from client: Test message from CLIENT to SERVER               =
//=---------------------------------------------------------------------------=
//=  Build:                                                                   =
//=    Windows (WIN):  Borland: bcc32 udpServer.c                             =
//=                    MinGW: gcc udpServer.c -lws2_32 -o updServer           =
//=                    Visual C: cl ucpServer.c wsock32.lib                   =
//=    Unix/Mac (BSD): gcc ucpServer.c -lnsl -o ucpServer                     =
//=---------------------------------------------------------------------------=
//=  Execute: udpServer                                                       =
//=---------------------------------------------------------------------------=
//=  Author: Ken Christensen                                                  =
//=          University of South Florida                                      =
//=          WWW: http://www.csee.usf.edu/~christen                           =
//=          Email: christen@csee.usf.edu                                     =
//=---------------------------------------------------------------------------=
//=  History:  KJC (08/02/08) - Genesis (from server1.c)                      =
//=            KJC (09/07/09) - Minor clean-up                                =
//=            KJC (09/22/13) - Minor clean-up to fix warnings                =
//=            KJC (09/14/17) - Updated build instructions                    =
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
#define POLYNOMIAL 0x04c11db7L      // Standard CRC-32 ppolynomial
#define BUFFER_LEN       4096L      // Length of buffer


#define SHMKEY_1 ((key_t) 1497)
#define SEMKEY_1 ((key_t) 400L) // semaphore key
#define SHMKEY_2 ((key_t) 1498)
#define SEMKEY_2 ((key_t) 401L) // semaphore key
//#define SHMKEY_3 ((key_t) 1497)
//#define SEMKEY_3 ((key_t) 400L) // semaphore key
#define NSEMS 1               // number of semaphores being created


// definition of the shared memory struct
typedef struct{
    pid_t value;
} shared_pid;

// definition of the shared memory struct
typedef struct{
    time_t last;
} shared_time;


// semaphore buffers
static struct sembuf OP_1 = {0,-1,0};
static struct sembuf OV_1 = {0,1,0};
struct sembuf *P_1 = &OP_1;
struct sembuf *V_1 = &OV_1;

static struct sembuf OP_2 = {0,-1,0};
static struct sembuf OV_2 = {0,1,0};
struct sembuf *P_2 = &OP_2;
struct sembuf *V_2 = &OV_2;


// semaphore union used to generate semaphore
typedef union{
    int val;
    struct semid_ds *buf;
    ushort *array;
} semunion;


int sem_id_1;
int sem_id_2;

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
// POP (wait()) function for semaphore to protect critical section
int POP_1();
int VOP_1();
int POP_2();
int VOP_2();

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

printf("got to start\n");
// declaration of the variables to be used in the semaphores
    int value, value1, shmid_1, shmid_2;
    char *shmadd = 0;
    semunion semctl_arg;
    semctl_arg.val = 1;

    // create semaphores
    sem_id_1 = semget(SEMKEY_1, NSEMS, IPC_CREAT | 0666);
    if (sem_id_1 < 0)
        printf("Error in creating the semaphore.\n");

    sem_id_2 = semget(SEMKEY_2, NSEMS, IPC_CREAT | 0666);
    if (sem_id_2 < 0)
        printf("Error in creating the semaphore.\n");

    // initialize semaphore
    value1 = semctl(sem_id_1, 0, SETVAL, semctl_arg);
    value = semctl(sem_id_2, 0, GETVAL, semctl_arg);

    // setting ID to the pid of the parent
    //ID = getpid();

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



printf("got past start\n");

    shpid->value = -1;

printf("set shpid\n");
    shtime->last = time(NULL);

printf("set shtime\n");


printf("get parent id\n");
    parent = getpid();

printf("got parent id\n");



#ifdef WIN
  // This stuff initializes winsock
  WSAStartup(wVersionRequested, &wsaData);
#endif
  fflush(stdout);
  if ((monitor_pid = fork()) == 0){
    printf("started monitor\n");
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

  // >>> Step #1 <<<
  // Create a socket
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


  // >>> Step #2 <<<
  // Fill-in my socket's address information
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

  int flag;

  addr_len = sizeof(client_addr);
  
  fflush(stdout);
  printf("Waiting for knock from client\n");

  pid_t pid = -1;

  while(1){

    flag = 0;

    /* wait for the first message from the client */
    retcode = recvfrom(server_s, in_buf, sizeof(in_buf)+1, 0,
    (struct sockaddr *)&client_addr, &addr_len);
    if (retcode < 0)
    {
      printf("*** ERROR - recvfrom() failed \n");
      exit(-1);
    } 
    if (strcmp(pass1, in_buf) != 0 && strcmp(pass2, in_buf) != 0 && strcmp(pass3, in_buf) != 0){
      printf("%s\n", in_buf);
      continue;
      
    }

   

  fflush(stdout);
    printf("Received first client knock\n");

    /* wait for the second message from the client */
    retcode = recvfrom(server_s2, in_buf, sizeof(in_buf)+1, 0,
    (struct sockaddr *)&client_addr, &addr_len);
    if (retcode < 0)
    {
      printf("*** ERROR - recvfrom 2 () failed \n");
      exit(-1);
    } 
    if (strcmp(pass1, in_buf) !=0 && strcmp(pass2, in_buf) != 0 && strcmp(pass3, in_buf) != 0){
      printf("%s\n", in_buf);
      continue;
      
    }


  fflush(stdout);
    printf("Received second client knock\n");

    /* wait for the third message from the client */
    retcode = recvfrom(server_s3, in_buf, sizeof(in_buf)+1, 0,
    (struct sockaddr *)&client_addr, &addr_len);
    if (retcode < 0)
    {
      printf("*** ERROR - recvfrom 3 () failed \n");
      exit(-1);
    } 
    if (strcmp(pass1, in_buf) != 0 && strcmp(pass2, in_buf) != 0 && strcmp(pass3, in_buf) != 0){
      printf("%s\n", in_buf);
      continue;
      
    }

    flag = 1;

  fflush(stdout);
    printf("Received third client knock\n");
    // This is where ya boi Tim comes in

    printf("shpid val = %d\n",shpid->value);
    pid = fork();
    waitpid(shpid->value,&status,WUNTRACED);
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
    //waitpid(shpid->value,&status,WUNTRACED);
    else if (WIFSTOPPED(status) > 0)
    {
        kill(shpid->value,SIGCONT);
    }
    else {
        shtime->last = time(NULL);
    }

  }


  // >>> Step #5 <<<
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


void check_time(){
    int shmid_1 = shmget(SHMKEY_1, sizeof(pid_t), IPC_CREAT | 0666);
    shared_pid *temp_pid = shmat(shmid_1, (char *)0,0);

    int shmid_2 = shmget(SHMKEY_2, sizeof(time_t), IPC_CREAT | 0666);
    shared_time *temp_time = shmat(shmid_2, (char *)0,0);

    printf("in monitor\n");
    while(1){
        sleep(1);
        printf("diff = %d\n",(int)difftime(time(NULL),temp_time->last));
	
	if ((int)difftime(time(NULL),temp_time->last) >= 10){
        printf("temp_pid = %d\n",temp_pid->value);
	    if (temp_pid->value != -1){
		kill(temp_pid->value, SIGSTOP);
	    }
	}
    }
}


int POP_1(){
    int status;
    status = semop(sem_id_1, P_1, 1);
    return status;
}

int VOP_1(){
    int status;
    status = semop(sem_id_1, V_1, 1);
    return status;
}


int POP_2(){
    int status;
    status = semop(sem_id_2, P_2, 1);
    return status;
}

int VOP_2(){
    int status;
    status = semop(sem_id_2, V_2, 1);
    return status;
}



