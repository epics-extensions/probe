#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
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

/*
 *          X's stuff
 */

extern XmFontList fontList;
extern XFontStruct *font;
extern XmFontList fontList1;
extern XFontStruct *font1;

void updateLabelDisplay();
void updateTextDisplay();
void adjustOptionCallback();


void adjustCancelCallback(w, channel, call_data) 
   Widget              w; 
   atom                *channel;
   XmAnyCallbackStruct *call_data; 
{
   XtUnmanageChild(channel->adjust.dialog);
   XtDestroyWidget(channel->adjust.dialog);
   channel->adjust.upMask = ALL_DOWN;
   channel->upMask &= ~ADJUST_UP;
   channel->updateMask &= ~UPDATE_ADJUST;
   channel->adjust.dialog = NULL;
   channel->adjust.panel = NULL;
   clearButtons(&(channel->adjust.buttons));
   clearSlider(&(channel->adjust.slider));
   channel->d[TEXT1].w = NULL;
   channel->d[TEXT1].proc = NULL; 
   channel->d[LABEL2].w = NULL;
   channel->d[LABEL2].proc = NULL;  
}


void updateAdjustPanel(ch)
atom   *ch;
{
  int i, n;
  Arg wargs[5];
  double max, min;
  char tmp[20];

  if ((ch->upMask & ADJUST_UP) == 0) return;

  if (ch->connected == FALSE) {
    if (ch->adjust.dialog != NULL) {
      adjustCancelCallback(NULL,ch,NULL);
      return;
    }
  } else {
    switch(ca_field_type(ch->chId)) {
    case DBF_STRING :
      if (XtIsManaged(ch->adjust.panel))
        XtUnmanageChild(ch->adjust.panel);        
      if (ch->adjust.upMask != ALL_DOWN) {
        if (ch->adjust.upMask & ADJUST_SLIDER_UP)
          destroySlider(ch);
        if (ch->adjust.upMask & ADJUST_BUTTONS_UP)
          destroyButtons(ch);
        if (ch->adjust.upMask & ADJUST_DIALS_UP)
          probeDisableDial(ch);
      }
      break;
    case DBF_ENUM :
      if (XtIsManaged(ch->adjust.panel))
        XtUnmanageChild(ch->adjust.panel);     
      if (ch->adjust.upMask != ALL_DOWN) {
        if (ch->adjust.upMask & ADJUST_SLIDER_UP)
          destroySlider(ch);
        if (ch->adjust.upMask & ADJUST_BUTTONS_UP)
          destroyButtons(ch);
        if (ch->adjust.upMask & ADJUST_DIALS_UP)
          probeDisableDial(ch->adjust.dials.buttonsSelected);
      }
      createButtons(ch);
      XtManageChild(ch->adjust.panel);
      break;
    case DBF_CHAR   : 
    case DBF_INT    : 
    case DBF_LONG   : 
    case DBF_FLOAT  : 
    case DBF_DOUBLE :
      if (XtIsManaged(ch->adjust.panel))
        XtUnmanageChild(ch->adjust.panel);   
      if (ch->adjust.upMask != ALL_DOWN) {
        if (ch->adjust.upMask & ADJUST_SLIDER_UP)
          destroySlider(ch);
        if (ch->adjust.upMask & ADJUST_BUTTONS_UP)
          destroyButtons(ch);
      }
      createSlider(ch);
      if (ch->adjust.upMask & ADJUST_DIALS_UP)
        probeEnableDial(ch->adjust.dials.buttonsSelected);
      XtManageChild(ch->adjust.panel);
      break;
    default         :
      printf("updateAdjustPanel : Unknown channel type.\n");
      break;
    }
  }
  ch->updateMask &= ~UPDATE_ADJUST;
}

void textAdjustCallback(w, channel, callData)
   Widget               w;
   atom                 *channel;
   XmAnyCallbackStruct  *callData;
{
  int n;
  long outputL;
  double outputD;
  
  if (channel->monitored == FALSE) return;

  switch(ca_field_type(channel->chId)) {
    case DBF_STRING :
      strcpy(channel->data.S.value,XmTextGetString(channel->d[TEXT1].w));  
      break;
    case DBF_ENUM   :
      n = 0;
      do {
        if (strcmp(XmTextGetString(channel->d[TEXT1].w),
                   channel->info.data.E.strs[n]) == 0)
          break;
        n++;
      } while (n<channel->info.data.E.no_str);
      if (n < channel->info.data.E.no_str) {
        channel->data.E.value = (short) n;
        break;
      }
      if (sscanf(XmTextGetString(channel->d[TEXT1].w),
          "%d", &n)) {
        if (n < channel->info.data.E.no_str) {
          channel->data.E.value = (short) n;
          break;
        }
      }
      fprintf(stderr, "textAdjustCallback : Bad value!\007\n");
      return;
      break;
    case DBF_CHAR   : 
    case DBF_INT    :     
    case DBF_LONG   :
      if (sscanf(XmTextGetString(channel->d[TEXT1].w),"%ld", &outputL)) {
        channel->data.L.value = outputL;
      } else {
        fprintf(stderr, "textAdjustCallback : Must be a integer!\007\n");
        return;
      }
      break;
    case DBF_FLOAT  : 
    case DBF_DOUBLE :    
      if (sscanf(XmTextGetString(channel->d[TEXT1].w),"%lf",&outputD)) {
        channel->data.D.value = outputD;
      } else {
        fprintf(stderr, "textAdjustCallback : Must be a number!\007\n");
        return;
      }
      break;
    default         : 
      return;
      break;
  } 
  probeCASetValue(channel);
  updateDisplay(channel);
}


void adjustCallback(w, channel, call_data)
   Widget               w;
   atom                 *channel;
   XmAnyCallbackStruct  *call_data;
{
  int        n,i;
  Arg        wargs[5];
  XmString   xmstr;
  Widget     formatPanel;
  Widget     separator1;
  Widget     separator2;
  Widget     okButton, option, cancel;
  Widget     buttonsHolder;

  if (channel->upMask & ADJUST_UP) return;
  if (channel->connected == FALSE) return;
  if (channel->monitored == FALSE) startMonitor(NULL,channel,NULL);
  channel->upMask |= ADJUST_UP;

  /*
   * Create the XmbulletinBoardDialogBox.
   */

  n = 0;
  if (font)
    XtSetArg(wargs[n], XmNtextFontList, fontList); n++;
  XtSetArg(wargs[n], XmNautoUnmanage, FALSE); n++;
  channel->adjust.dialog = 
            (Widget) XmCreateBulletinBoardDialog(w, "Adjust", wargs, n);

  /*
   * Create a XmRowColumn widget to hold everything.
   */
  formatPanel = XtCreateManagedWidget("panel", 
                                 xmRowColumnWidgetClass,
                                 channel->adjust.dialog, NULL, 0);

  /*
   * Create a label for the panel.
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  channel->d[LABEL2].w = XtCreateManagedWidget("Channel name", 
                                   xmLabelWidgetClass,
                                   formatPanel, wargs, n);
  channel->d[LABEL2].proc = updateLabelDisplay;
  channel->updateMask |= UPDATE_LABEL2;
  
  /*
   * Create a text input.
   */

  n = 0;
  if (font) {
    XtSetArg(wargs[n],  XmNfontList, fontList); n++;
  }
  channel->d[TEXT1].w = XtCreateManagedWidget("textAdjust",
                                   xmTextWidgetClass, 
                                   formatPanel, wargs, n);
  channel->d[TEXT1].proc = updateTextDisplay;
  channel->updateMask |= UPDATE_TEXT1;

  /*
   * Create a separator for the extra control function region.
   */

  n = 0;
  separator1 = XtCreateManagedWidget("separator1",
                  xmSeparatorWidgetClass, formatPanel, wargs, n);

  /*
   *	Create a rowColumn widget to hold the
   *       slider , toggle switch or other special controls.
   */
  n = 0;      
  channel->adjust.panel = XmCreateRowColumn(formatPanel,
                                   "adjust", wargs, n);
  XtManageChild(channel->adjust.panel);
  n = 0;
  separator2 = XtCreateManagedWidget("separator1",
                  xmSeparatorWidgetClass, formatPanel, wargs, n);

  /*
   * Create a XmRowColumn widget to hold the name and value of
   *    the process variable.
   */
  n = 0;
  XtSetArg(wargs[n],  XmNorientation, XmVERTICAL); n++;
  XtSetArg(wargs[n],  XmNnumColumns, 3); n++;
  XtSetArg(wargs[n],  XmNpacking, XmPACK_COLUMN);n++;
  XtSetArg(wargs[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
  buttonsHolder = XtCreateManagedWidget("holder", 
                                   xmRowColumnWidgetClass,
                                   formatPanel, wargs, n);
   /* 
    *  Create 'OK' button
    */

   n = 0;
   if (font) {
     XtSetArg(wargs[n],  XmNfontList, fontList); n++;
   }
   okButton = XtCreateManagedWidget("Ok", 
                               xmPushButtonWidgetClass,
                               buttonsHolder, wargs, n);
   XtAddCallback(okButton, XmNactivateCallback, textAdjustCallback, channel);

   /* 
    *  Create 'option' button
    */

   n = 0;
   if (font) {
     XtSetArg(wargs[n],  XmNfontList, fontList); n++;
   }
   option = XtCreateManagedWidget("Option", 
                               xmPushButtonWidgetClass,
                               buttonsHolder, wargs, n);
/*
   XtAddCallback(option, XmNactivateCallback,adjustOptionCallback,channel);
*/


   /* 
    *  Create 'cancel' button
    */

   n = 0;
   if (font) {
     XtSetArg(wargs[n],  XmNfontList, fontList); n++;
   }
   cancel = XtCreateManagedWidget("Cancel", 
                               xmPushButtonWidgetClass,
                               buttonsHolder, wargs, n);
   XtAddCallback(cancel, XmNactivateCallback,adjustCancelCallback,channel);

   n = 0;
   XtSetArg(wargs[n], XmNdefaultButton, okButton); n++;
   XtSetValues(channel->adjust.dialog,wargs,n);
   channel->d[ADJUST].w = channel->adjust.dialog;
   channel->d[ADJUST].proc = updateAdjustPanel;
   updateAdjustPanel(channel);
   updateDisplay(channel);
   XtManageChild(channel->adjust.dialog);
}

