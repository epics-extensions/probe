#include <stdarg.h>
#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
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

char         *alarmSeverityStrs[] = {
                "NO ALARM",
                "MINOR",
                "MAJOR",
                "INVALID",
                "alarm no severtiy",
              };

char         *alarmStatusStrs[] = {
                "NO ALARM",
                "READ",
                "WRITE",
                "HIHI",
                "HIGH",
                "LOLO",
                "LOW",
                "STATE",
                "COS",
                "COMM",
                "TIMEOUT",
                "HW_LIMIT",
                "CALC",
                "SCAN",
                "LINK",
                "SOFT",
                "BAD_SUB",
                "UDF",
		"DISABLE_ALARM",
		"SIMM_ALARM",
                "alarm no status",
              };

void wprintf(Widget w, ...)
{
  char     *format;
  va_list   args;
  char      str[1000];  /* DANGER: Fixed buffer size */
  Arg       wargs[1];
  XmString  xmstr;
  /*
   * Init the variable length args list.
   */
  va_start(args,w);
  /*
   * Extract the format to be used.
   */
  format = va_arg(args, char *);
  /*
   * Use vsprintf to format the string to be displayed in the
   * XmLabel widget, then convert it to a compound string
   */
  vsprintf(str, format, args);

  xmstr =  XmStringLtoRCreate(str, XmSTRING_DEFAULT_CHARSET);

  XtSetArg(wargs[0], XmNlabelString, xmstr);
  XtSetValues(w, wargs, 1);     

  va_end(args);
}

void updateStatusDisplay(atom *channel,unsigned int i)
{
  if (channel->connected) {
    if ((channel->data.D.status > ALARM_NSTATUS) 
         || (channel->data.D.status < NO_ALARM)) {
      fprintf(stderr,"updateMonitor : unknown alarm status (%s) !!!\n",
              ca_name(channel->chId));
    } else {
      if ((channel->data.D.severity > ALARM_NSEV) 
           || (channel->data.D.severity < NO_ALARM)) {
        fprintf(stderr,"updateMonitor : unknown alarm severity (%s) !!!\n",
                 ca_name(channel->chId));
      } else {
        if (channel->monitored) {
          wprintf(channel->d[i].w,"%-8s  %-8s  monitor",
             alarmStatusStrs[channel->data.D.status],
             alarmSeverityStrs[channel->data.D.severity]);
        } else {
          wprintf(channel->d[i].w,"%-8s  %-8s",
             alarmStatusStrs[channel->data.D.status],
             alarmSeverityStrs[channel->data.D.severity]);
        }
      }
    }
  } else {
    wprintf(channel->d[i].w,"Cannot connect channel!");
  }
}

void updateDataDisplay(atom *channel,unsigned int i)
{
  if (channel->connected) {
    switch(ca_field_type(channel->chId)) {
      case DBF_STRING :
        wprintf(channel->d[i].w,"%s",channel->data.S.value);
        break;
      case DBF_ENUM :         
        if (strlen(channel->info.data.E.strs[channel->data.E.value]) > (size_t)0) {
          wprintf(channel->d[i].w,"%s",
                     channel->info.data.E.strs[channel->data.E.value]);
        } else {
          wprintf(channel->d[i].w,"%d",channel->data.E.value);
        }
        break;
      case DBF_CHAR   :  
      case DBF_INT    :
      case DBF_LONG   : 
        wprintf(channel->d[i].w,"%d",channel->data.L.value);
        break;
      case DBF_FLOAT  :  
      case DBF_DOUBLE :
        wprintf(channel->d[i].w,channel->format.str,channel->data.D.value);
        break;
      default :
        break;
    }
  }
}   

void updateLabelDisplay(atom *channel,unsigned int i)
{
  if (channel->connected) {
    wprintf(channel->d[i].w,"%s",ca_name(channel->chId));
  } else {
    wprintf(channel->d[i].w,"Unknown channel name !");
  }
}   

void updateTextDisplay(atom *channel,unsigned int i)
{
  char tmp[40];
  switch(ca_field_type(channel->chId)) {
    case DBF_STRING :
      sprintf(tmp,"%s",channel->data.S.value);
      XmTextSetString(channel->d[i].w,tmp);
      break;
    case DBF_ENUM :         
      if (strlen(channel->info.data.E.strs[channel->data.E.value]) > (size_t)0) {
        sprintf(tmp,"%s",channel->info.data.E.strs[channel->data.E.value]);
      } else {
        sprintf(tmp,"%d",channel->data.E.value);
      }
      XmTextSetString(channel->d[i].w,tmp);
      break;
    case DBF_CHAR   :  
    case DBF_INT    :
    case DBF_LONG   : 
      sprintf(tmp,"%d",channel->data.L.value);
      XmTextSetString(channel->d[i].w,tmp);
      break;
    case DBF_FLOAT  :  
    case DBF_DOUBLE :
      sprintf(tmp,channel->format.str1,channel->data.D.value);
      XmTextSetString(channel->d[i].w,tmp);
      break;
  }
}   

void updateDisplay(atom *channel)
{
unsigned int mask;
int i;

  if (channel->updateMask != NO_UPDATE) {
    mask = 1;
    mask = mask << (LAST_ITEM - 1);
    for (i=(LAST_ITEM-1);i>=0;i--) {
      if ((channel->updateMask & mask) && (channel->d[i].proc)){
        (*(channel->d[i].proc))(channel,i);
      }
      mask = mask >> 1;
    }
  }
  channel->updateMask = NO_UPDATE;
}
         
