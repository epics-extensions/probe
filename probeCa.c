#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/PanedW.h>
#include <Xm/MessageB.h>

/*	
 *       System includes for CA		
 */

#include <caerr.h>
#include <cadef.h>
#include <db_access.h>
#include <stdio.h>
#include <string.h>
#include <alarm.h>

#include "probe.h"

/*
 *       CA stuffs
 */

int          ii,is,ip,ia,ig;

void  setDefaultUnits();
void  makeInfoFormatStr();
void  makeHistFormatStr();
void  makeDataFormatStr();

/*
 *	data structure for dials
 */
extern int dialsUp;
/*
 *           X's stuffs
 */

XtWorkProcId work_proc_id = NULL;
XtIntervalId monitorId = NULL;

char         helpStr[] = "Version -1.0\nHelp is not available";
extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;
extern XmFontList fontList2;
extern XFontStruct *font2;
extern XtAppContext app;

void    updateMonitor(XtPointer, XtIntervalId *);
void    printData();
void    startMonitor();
void    stopMonitor();
void    getChan();
void    helpMonitor();
void    quitMonitor();


void probeCAException(arg)
struct exception_handler_args arg;
{
    chid	pCh;
    int		stat;

    pCh = arg.chid;
    stat = arg.stat;
    (void)printf("CA exception handler entered for %s\n", ca_name(pCh));
    (void)printf("%s\n", ca_message(stat));
}

int probeCATaskInit() {
 /* CA Stuff */
  int stat;

  stat = ca_task_initialize();
  SEVCHK(stat,"probeCATaskInit : ca_task_initialize failed!");
  if (stat != ECA_NORMAL) return 1;
  
  stat = ca_add_exception_event(probeCAException, NULL);
  SEVCHK(stat,"probeCATaskInit : ca_add_exception_event failed!");
  if (stat != ECA_NORMAL) return 1;

  stat = ca_pend_io(5.0);
  SEVCHK(stat,"probeCATaskInit : ca_pend_io failed!");
  if (stat != ECA_NORMAL) return 1;

  return 0;
}

int channelIsAlreadyConnected(char name[], atom* channel)
{
  if (strcmp(name,ca_name(channel->chId))) return FALSE;
  return TRUE;
}

int connectChannel(char name[], atom *channel)
{
    int stat;

    channel->upMask &= ~MONITOR_UP;
    if (monitorId) {
      XtRemoveTimeOut(monitorId);
      monitorId = NULL;
    }    

    if (channel->monitored) {
      stat = ca_clear_event(channel->eventId);
      SEVCHK(stat,"connectChannel : ca_clear_event failed!");
      channel->monitored = FALSE;
      if (stat != ECA_NORMAL) return FALSE;
    }

    if (channel->connected) {
      channel->connected = FALSE;
    }

    /* for ca */

    is = ca_search(name,&(channel->chId));
    SEVCHK(is,"connectChannel : ca_search failed!");
    ip = ca_pend_io(2.0);
    SEVCHK(ip,"connectChannel : ca_pend_io failed!");

    if ((is != ECA_NORMAL)||(ip != ECA_NORMAL)) {
      if (channel->chId != NULL) {
        stat = ca_clear_channel(channel->chId);
        channel->chId = NULL;
        SEVCHK(stat,"connectChannel : ca_clear_channel failed!\n");
        stat = ca_pend_io(0.1);
        SEVCHK(stat,"connectChannel : ca_pend_io failed!\n");
      }
      channel->updateMask |= UPDATE_LABELS;
      return FALSE;
    }

    channel->connected = TRUE;
    channel->updateMask |= UPDATE_LABELS | UPDATE_DATA;;
    return TRUE;
}

int getData(atom *channel)
{
   int stat;

   switch(ca_field_type(channel->chId)) {
      case DBF_STRING : 
         stat = ca_get(dbf_type_to_DBR_TIME(DBF_STRING),channel->chId,&(channel->data.S));
         SEVCHK(stat,"getData : ca_get get valueS failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat  = ca_pend_io(1.0);
         SEVCHK(stat,"getData : ca_pend_io for valueS failed !"); 
         time(&channel->currentTime);
         if (stat != ECA_NORMAL) goto getDataErrorHandler;
         
         printf("string type.\n");
         break;

      case DBF_ENUM   : 
         stat = ca_get(dbf_type_to_DBR_CTRL(DBF_ENUM),channel->chId,&(channel->info.data.E));
         SEVCHK(stat,"getData : ca_get get grCtrlE failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat = ca_get(dbf_type_to_DBR_TIME(DBF_ENUM),channel->chId,&(channel->data.E));
         SEVCHK(stat,"getData : ca_get get valueE failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat  = ca_pend_io(1.0);
         SEVCHK(stat,"getData : ca_pend_io for valueE failed !"); 
         time(&channel->currentTime);
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         printf("enum type.\n");
         break;

      case DBF_CHAR   : 
      case DBF_INT    : 
      case DBF_LONG   : 
         stat = ca_get(dbf_type_to_DBR_CTRL(DBF_LONG),
                       channel->chId,&(channel->info.data.L));
         SEVCHK(stat,"getData : ca_get get grCtrlL failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat = ca_get(dbf_type_to_DBR_TIME(DBF_LONG),
                       channel->chId,&(channel->data.L));
         SEVCHK(stat,"getData : ca_get get valueL failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat  = ca_pend_io(1.0);
         SEVCHK(stat,"getData : ca_pend_io for valueL failed !"); 
         time(&channel->currentTime);
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         printf("long type.\n");
         break;

      case DBF_FLOAT  : 
      case DBF_DOUBLE :
         stat = ca_get(dbf_type_to_DBR_CTRL(DBF_DOUBLE),
                       channel->chId,&(channel->info.data.D));
         SEVCHK(stat,"getData : ca_get get grCtrlD failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat = ca_get(dbf_type_to_DBR_TIME(DBF_DOUBLE),
                       channel->chId,&(channel->data.D));
         SEVCHK(stat,"getData : ca_get get valueD failed !"); 
         if (stat != ECA_NORMAL) goto getDataErrorHandler;

         stat  = ca_pend_io(1.0);
         SEVCHK(stat,"getData : ca_pend_io for valueD failed !"); 
         time(&channel->currentTime);
         if (stat != ECA_NORMAL) goto getDataErrorHandler;
         channel->updateMask |= UPDATE_FORMAT;

         printf("double type.\n");
         break;
       default         :
         printf("getData : Unknown channel type.\n");
         break;
     }
   setChannelDefaults(channel);
   makeInfoFormatStr(channel);
   makeDataFormatStr(channel);
   channel->updateMask |= UPDATE_DATA;
   return TRUE;
getDataErrorHandler :
   channel->updateMask |= UPDATE_DATA;
   return FALSE;
}

void initChan(char *name, atom *channel)
{
  char tmp[40];
  int  stat;

  XmTextSetString(channel->d[TEXT1].w,name);
  connectChannel(name, channel);
  getData(channel);
  updateDisplay(channel); 
}


void getChan(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  atom    *channel = (atom *) clientData;
  char tmp[40];
  int  stat;

  if ((channel->connected) && (channelIsAlreadyConnected(XmTextGetString(w),channel))) {
    printf("channel is already connected!\n");
  } else {
    connectChannel(XmTextGetString(w),channel);
  }


  if (channel->connected) {
    if (channel->monitored == FALSE) {
      getData(channel);
    }
  }

  if (channel->upMask & ADJUST_UP) {
    channel->updateMask |= UPDATE_ADJUST;
    startMonitor(NULL,channel,NULL);
  } else {
    updateDisplay(channel);
  }   
}

void startMonitor(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  atom    *channel = (atom *) clientData;
  int stat;

  if (channel->upMask & MONITOR_UP) return;

  if (channel->connected) {
    switch(ca_field_type(channel->chId)) {
      case DBF_STRING :       
        ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_STRING),
                          channel->chId, printData, channel,
                          &(channel->eventId));
        SEVCHK(ia,"startMonitor : ca_add_event for valueS failed!"); 
        ip = ca_pend_io(1.0);
        SEVCHK(ip,"startMonitor : ca_pend_io for valueS failed!");
        break;
      case DBF_ENUM   :   
        ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_ENUM),
                          channel->chId, printData, channel,
                          &(channel->eventId));    
        SEVCHK(ia,"startMonitor : ca_add_event for valueE failed!"); 
        ip = ca_pend_io(1.0);
        SEVCHK(ip,"startMonitor : ca_pend_io for valueE failed!");
        break;
      case DBF_CHAR   : 
      case DBF_INT    :     
      case DBF_LONG   :
        ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_LONG),
                          channel->chId, printData, channel,
                          &(channel->eventId)); 
        SEVCHK(ia,"startMonitor : ca_add_event for valueL failed!"); 
        ip = ca_pend_io(1.0);
        SEVCHK(ip,"startMonitor : ca_pend_io for valueL failed!");
        break;
      case DBF_FLOAT  : 
      case DBF_DOUBLE :     
        ia = ca_add_event(dbf_type_to_DBR_TIME(DBF_DOUBLE),
                          channel->chId, printData, channel,
                          &(channel->eventId));
        SEVCHK(ia,"startMonitor : ca_add_event for valueD failed!"); 
        ip = ca_pend_io(1.0);
        SEVCHK(ip,"startMonitor : ca_pend_io for valueD failed!");
        makeHistFormatStr(channel);
        break;
      default         : 
        break;
    } 

    if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
      channel->monitored = FALSE;
    } else {
      XtIntervalId id;
      channel->monitored = TRUE;
      channel->upMask |= MONITOR_UP;
      channel->updateMask |= UPDATE_STATUS1;
      /*
       * Ask Unix for the time.
       */
      getData(channel);
      resetHistory(channel);
      updateMonitor(channel,&id);
    }
  }
}

void stopMonitor(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  atom *channel = (atom *) clientData;
  if (channel->monitored) {
    ia = ca_clear_event(channel->eventId);
    ip = ca_pend_io(1.0);
    if ((ia != ECA_NORMAL) || (ip != ECA_NORMAL)) {
       printf("stopMonitor : ca_clear_event failed! %d, %d\n",ia,ip);
    }      
    channel->monitored = FALSE;
  }
  if (monitorId) {
    XtRemoveTimeOut(monitorId);
    monitorId = NULL;
  }
  if (channel->upMask & ADJUST_UP)
    adjustCancelCallback(NULL,channel,NULL);
  channel->upMask &= ~MONITOR_UP;
  channel->updateMask |= UPDATE_STATUS1;
  updateDisplay(channel);
}

void xs_ok_callback( 
   Widget    w, 
   XtPointer clientData,
   XtPointer callbackStrut) 
{
   XtUnmanageChild(w);
   XtDestroyWidget(w);   
}

void helpMonitor(
   Widget    w,
   XtPointer clientData,
   XtPointer callbackStrut)
{
  XtPopup(*((Widget *) clientData),XtGrabNone);
}

void quitMonitor(
   Widget    w,
   XtPointer clientData,
   XtPointer callbackStrut)
{
  atom *channel = (atom *) clientData;
  int stat;
  if (channel->monitored) {
    stat = ca_clear_event(channel->eventId);
    SEVCHK(stat,"quitMonitor : ca_clear_event failed!");
    stat = ca_pend_io(5.0);
    SEVCHK(stat,"quitMonitor : ca_clear_event failed!");     
    channel->monitored = FALSE;
  }

  channel->upMask &= ~MONITOR_UP;
  if (monitorId) {
    XtRemoveTimeOut(monitorId);
    monitorId = NULL;
  }

  if (channel->connected) {
    stat = ca_clear_channel(channel->chId);
    SEVCHK(stat,"quitMonitor : ca_clear_channel failed!");
    stat = ca_pend_io(5.0);     
    SEVCHK(stat,"quitMonitor : ca_clear_event failed!");    
    channel->connected = FALSE;
  }
  ca_task_exit();
  if (fontList) XmFontListFree(fontList);
  if (fontList1) XmFontListFree(fontList1);
  if (fontList2) XmFontListFree(fontList2);
  XtCloseDisplay(XtDisplay(w));
  exit(0);
}

void probeCASetValue(atom *ch)
{
  int    stat;
  
  if (ch->connected) {
    switch(ca_field_type(ch->chId)) {
      case DBF_STRING :       
        stat = ca_put(DBF_STRING, ch->chId, ch->data.S.value);
        SEVCHK(stat,"setValue : ca_put for valueS failed!");
        break;
      case DBF_ENUM   :
        stat = ca_put(DBF_SHORT, ch->chId, &(ch->data.E.value));
        SEVCHK(stat,"setValue : ca_put for valueE failed!");
        break;
      case DBF_CHAR   : 
      case DBF_INT    :     
      case DBF_LONG   :
        stat = ca_put(DBF_LONG, ch->chId, &(ch->data.L.value));
        SEVCHK(stat,"setValue : ca_put for valueL failed!");
        break;
      case DBF_FLOAT  : 
      case DBF_DOUBLE :    
        stat = ca_put(DBF_DOUBLE, ch->chId, &(ch->data.D.value));
        SEVCHK(stat,"setValue : ca_put for valueD failed!");
        break;
      default         : 
        break;
    } 
    stat = ca_flush_io();
    SEVCHK(stat,"probeCASetValue : ca_flush_io failed!");
    if (ch->monitored == FALSE) {
      getData(ch);
    }
  }

}

void printData(struct event_handler_args arg)
{
	int nBytes;
        atom *channel = (atom *) arg.usr;
        char *tmp = (char *) &(channel->data);

        /*
         * Ask Unix for the time.
         */
        time(&(channel->lastChangedTime));

	nBytes = dbr_size_n(arg.type, arg.count);
	while (nBytes-- > 0) 
                tmp[nBytes] = ((char *)arg.dbr)[nBytes];
        channel->changed = TRUE;
}
	
void updateMonitor(
   XtPointer clientData,
   XtIntervalId *id)
{
   atom *channel = (atom *) clientData;
   long     next_second = 5;
   unsigned int mask, i;
   int stat;

   if ((channel->upMask & MONITOR_UP) == 0) return;

   stat = ca_pend_event(0.001);
   if ((stat != ECA_NORMAL) && (stat != ECA_TIMEOUT)) {
      printf("upDateMonitor : ca_pend_event failed! %d\n",ip);
    }
   if (channel->changed) {
      updateHistoryInfo(channel);
      channel->changed = FALSE;
   }

   if (channel->updateMask != NO_UPDATE) 
      updateDisplay(channel);         
   XtAppAddTimeOut(app, next_second, updateMonitor, channel);
}
 
