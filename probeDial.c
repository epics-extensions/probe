#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define NDIALS 8
#define DIAL_NUMBER(event_code)	(event_code & 0xFF)

#define DIAL_0_IN_USE 0x00000001
#define DIAL_1_IN_USE 0x00000002
#define DIAL_2_IN_USE 0x00000004
#define DIAL_3_IN_USE 0x00000008
#define DIAL_4_IN_USE 0x00000010
#define DIAL_5_IN_USE 0x00000020
#define DIAL_6_IN_USE 0x00000040
#define DIAL_7_IN_USE 0x00000080

#define DIAL_0_ENABLE 0x00000100
#define DIAL_1_ENABLE 0x00000200
#define DIAL_2_ENABLE 0x00000400
#define DIAL_3_ENABLE 0x00000800
#define DIAL_4_ENABLE 0x00001000
#define DIAL_5_ENABLE 0x00002000
#define DIAL_6_ENABLE 0x00004000
#define DIAL_7_ENABLE 0x00008000

#define DIALS_ENABLE  0x80000000


typedef struct inputevent {
  short dialNum;
  short unused;
  int   delta;
} Event;

typedef struct dialCallbackDataStruc {
  void (*proc)();
  void *ClientData;
} dialCallbackStruc;

static dialCallbackStruc dialCallback[NDIALS] = {
   {NULL, NULL}, {NULL, NULL},
   {NULL, NULL}, {NULL, NULL},
   {NULL, NULL}, {NULL, NULL},
   {NULL, NULL}, {NULL, NULL}
};

int dialsUp = 0;
static int fd;
static char buf[16];
static char tmp;
static Event *event = (Event *) buf;
static int byt_cnt;
static int red;
static float dials[NDIALS] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
static unsigned int dialStatus = 0;
  
int probeInitializeDials() {
/*
  if ((fd = open("/dev/dialbox", O_RDWR)) == -1)  {
    perror("probeInitializeDials : Open failed for /dev/dialbox:");
    return -1;
  }
  dialStatus |= DIALS_ENABLE;
*/
  dialStatus = 0;
  return 0;
}

int probeDialIsPresent() {
  return (dialStatus & DIALS_ENABLE);
}

int probeDialInUsed(n) 
int n;
{
  if ((n >= 0) && (n <NDIALS)) {
    return (dialStatus & (unsigned int) (1 << n));
  } else {
    fprintf(stderr,"dialInUsed : Dial number out of range!\n");
    return -1;
  }
}

int probeDisableDial(n)
  int n;
{
  if ((n >= 0) && (n <NDIALS)) {
    dialStatus &= ~((unsigned int) (0x00000100 << n));
  } else {
    fprintf(stderr,"probeDisableCallback : Dial number out of range!\n");
    return -1;
  }
}

int probeEnableDial(n)
  int n;
{
  if ((n >= 0) && (n <NDIALS)) {
    dialStatus |= (unsigned int) (0x00000100 << n);
  } else {
    fprintf(stderr,"probeEnableCallback : Dial number out of range!\n");
    return -1;
  }
}
  
int probeAddDialCallback(n,proc,ClientData)
  int n;
  void *proc;
  void *ClientData;
{  
  if ((n >= 0) && (n < NDIALS)) {
    dialCallback[n].proc = proc;
    dialCallback[n].ClientData = ClientData;
    dialStatus |= (unsigned int) (1 << n);
    probeEnableDial(n);
    return 0;
  } else {
    fprintf(stderr,"probeAddDialCallback : Dial number out of range!\n");
    return -1;
  }
}
  
int probeRemoveDialCallback(n)
  int n;
{
  if ((n >= 0) && (n < NDIALS)) {
    dialCallback[n].proc = NULL;
    dialStatus &= ~((unsigned int) (1 << n));
    probeDisableDial(n);
    return 0;
  } else {
    fprintf(stderr,"probeRemoveDialCallback : Dial number out of range!\n");
  }
  return -1;
}

int probeScanDials() {
  int tmp, n;
  if (ioctl(fd, FIONREAD, &byt_cnt) == -1 || byt_cnt == 0) return 0;
  if((red = read(fd, buf, sizeof(buf))) == 16) {
    n = DIAL_NUMBER(event->dialNum);
    if ((dialCallback[n].proc) 
       && (dialStatus & (unsigned int) (0x00000100 << n))) {
       (*(dialCallback[n].proc)) (event->delta,
       dialCallback[n].ClientData, NULL);
    }       
  } else {
    fprintf(stderr, "read %d bytes\n", red);
    return -1;
  }
}
  
dumpDials(delta, i, reason) 
int delta;
int i, reason;
{

   dials[i] += ((float) delta) / 64;
   if (dials[i] >= 360.0) {
      dials[i] -= 360.0;
   }
   if (dials[i] <= -360.0) {
       dials[i] += 360.0;
   }
   printf("%8.2f %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f\n",
     dials[0],dials[1],dials[2],dials[3],dials[4],dials[5],dials[6],dials[7]);
}
   

