#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/PanedW.h>
#include <Xm/ScrollBar.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>

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

char       *dialButtonLabels[] = { 
                 "1",
                 "2",
                 "3",
                 "4",
                 "5",
                 "6",
                 "7",
                 "8" };


/*
 *          X's stuff
 */

extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;

void dialCallback(delta, ch, call_data)
   int       delta;
   atom      *ch;
   void      *call_data;
{
   char tmp[20];
   int n;
   Arg wargs[5];
 
  switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
       printf("dialCallback : Unexpected string type.\n");
       return;
       break;
    case DBF_ENUM   : 
       printf("dialCallback : Unexpected enum type.\n");
       return;
       break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
      ch->data.L.value += (long) delta / 90;
      ch->data.L.value = keepInRange(ch->data.L.value,
                              ch->info.data.L.upper_ctrl_limit,
                              ch->info.data.L.lower_ctrl_limit);
      break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
      ch->data.D.value += (double) delta / 90.0 / 
                          (double) ch->adjust.slider.info.D.scl;
      ch->data.D.value = keepInRange(ch->data.D.value,
                              ch->info.data.D.upper_ctrl_limit,
                              ch->info.data.D.lower_ctrl_limit);
      break;   
    default         :
      printf("dialCallback : Unknown Data type.\n");
      return;
      break;
  }
  probeCASetValue(ch);
}

 
printDialVal(delta, dialNum, reason)
int delta;
int dialNum, reason;
{
  printf("printDialVal : dial #%d = %d\n", dialNum, delta);
}


void adjustOptionCancelCallback(w, ch, call_data) 
   Widget               w; 
   atom                 *ch;
   XmAnyCallbackStruct  *call_data; 
{
   XtUnmanageChild(ch->adjust.optionDialog);
   XtDestroyWidget(ch->adjust.optionDialog);
   ch->upMask &= ~OPTION_UP;   
}

void dialButtonsCallback(w, ch, call_data) 
   Widget               w; 
   atom                 *ch;
   XmToggleButtonCallbackStruct *call_data; 
{
   char str[10];
   int n,i;
   Arg        wargs[5];

  Buttons *d = &(ch->adjust.dials);

  if (call_data->set) {
    if ((ch->adjust.upMask & ADJUST_DIALS_UP) == 0) {
      for (i=0; i < NDIALS; i++) {
        if (w == d->buttons[i]) break;
      }
      if ((i < NDIALS) && (i >= 0)) {
        probeAddDialCallback(i,dialCallback,ch);
        d->buttonsSelected = i;
        ch->adjust.upMask |= ADJUST_DIALS_UP;
      } else {
        fprintf(stderr,"dialToggleButtonsCallback : dialNum %d out of range!\n",
               i);
      }
    }
  } else { 
    if (ch->adjust.upMask & ADJUST_DIALS_UP) {
      for (i=0; i < NDIALS; i++) {
        if (w == d->buttons[i]) break;
      }
      if ((i < NDIALS) && (i >= 0)) {
        if (probeRemoveDialCallback(i)) {
          fprintf(stderr,"dialToggleButtonsCallback : no such callback!\n");
        }
        d->buttonsSelected = -1;
        ch->adjust.upMask &= ~ADJUST_DIALS_UP;
      } else {
        fprintf(stderr,"dialToggleButtonsCallback : dialNum %d out of range!\n",
              i);
      }
    }
  }
}

void adjustOptionCallback(w, ch, call_data)
   Widget               w;
   atom                 *ch;
   XmAnyCallbackStruct  *call_data;
{
  int        n,i;
  Arg        wargs[5];
  Widget     dialSelectPanel;
  Widget     dialButtonsHolder;
  Widget     adjustOptionCancel;

 /*
  * Create the message dialog to display the help.
  */
  n = 0;
  if (font) {
    XtSetArg(wargs[n], XmNtextFontList, fontList); n++;
  }
  XtSetArg(wargs[n], XmNautoUnmanage, FALSE); n++;
  ch->adjust.optionDialog = (Widget) XmCreateBulletinBoardDialog(w,
                                      "option", wargs, n);
  /*
   * Create a XmPanedWindow widget to hold everything.
   */
  n = 0;
  XtSetArg(wargs[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
  ch->adjust.dials.panel = XtCreateManagedWidget("panel", 
                                 xmRowColumnWidgetClass,
                                 ch->adjust.optionDialog, wargs, n);
   n = 0;
   dialSelectPanel = XtCreateManagedWidget("dialPanel",
                                  xmRowColumnWidgetClass,
                                  ch->adjust.dials.panel,wargs,n);
   n = 0;
   XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
   XtSetArg(wargs[n], XmNnumColumns, 4); n++;
   dialButtonsHolder = XmCreateRadioBox(ch->adjust.dials.panel, 
                                          "optionButton", wargs, n);
   XtManageChild(dialButtonsHolder);

   for (i=0;i<NDIALS;i++) {
     n = 0;
     if (i == ch->adjust.dials.buttonsSelected) {
       XtSetArg(wargs[n],XmNset,TRUE); n++;
     }
     ch->adjust.dials.buttons[i] = XmCreateToggleButton(dialButtonsHolder,
                              dialButtonLabels[i],wargs,n);
     if (i == ch->adjust.dials.buttonsSelected) {
       XtSetArg(wargs[n],XmNset,TRUE); n++;
     }
     XtAddCallback(ch->adjust.dials.buttons[i],
                  XmNvalueChangedCallback,dialButtonsCallback,ch);
   }

   /* 
    *  Create 'cancel' button
    */

   n = 0;
   if (font1) {
     XtSetArg(wargs[n],  XmNfontList, fontList1); n++;
   }
   adjustOptionCancel = XtCreateManagedWidget("Cancel", 
                               xmPushButtonWidgetClass,
                               ch->adjust.dials.panel, wargs, n);
   XtAddCallback(adjustOptionCancel,
     XmNactivateCallback,adjustOptionCancelCallback,ch);

   ch->upMask |= OPTION_UP;
   XtManageChildren(ch->adjust.dials.buttons,NDIALS); 
   XtManageChild(ch->adjust.optionDialog);
}
