#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>

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

char      *stateButtonLabels[] = {
                 "state 0",
                 "state 1",
                 "state 2",
                 "state 3",
                 "state 4",
                 "state 5",
                 "state 6",
                 "state 7",
                 "state 8",
                 "state 9",
                 "state 10",
                 "state 11",
                 "state 12",
                 "state 13",
                 "state 14",
                 "state 15"};
                 

void adjustButtonsCallback(w,ch, callData)
  Widget    w;
  atom      *ch;
  XmToggleButtonCallbackStruct *callData;
{
  char tmp[20];
  short i;

  Buttons *b = &(ch->adjust.buttons);

  if (!callData->set) return;
  for (i=0; i < ch->info.data.E.no_str; i++) {
    if (w == b->buttons[i]) break;
  }
  if (i == ch->info.data.E.no_str) {
    fprintf(stderr,"adjustButtonsCallback : buttons out of range!\n");
    return;
  }
  b->buttonsSelected = i;
  ch->updateMask |= UPDATE_DISPLAY | UPDATE_TEXT;
  ch->data.E.value = i;

  probeCASetValue(ch);
  updateDisplay(ch);  
}

void updateButtons(ch, dummy)
  atom   *ch;
  int    dummy;
{
  Arg arg;
  Buttons *b = &(ch->adjust.buttons);

  if ((ch->adjust.upMask & ADJUST_BUTTONS_UP) == 0) return;

  if (b->buttonsSelected == ch->data.E.value) return;
  XtSetArg(arg,XmNset,FALSE);
  XtSetValues(b->buttons[b->buttonsSelected],arg,1);
  XtSetArg(arg,XmNset,TRUE);
  XtSetValues(b->buttons[ch->data.E.value],arg,1);
  b->buttonsSelected = ch->data.E.value;
}

void destroyButtons(ch)
  atom   *ch;
{
  short i;
  Buttons *b = &(ch->adjust.buttons);

  XtUnmanageChild(b->panel);
  XtDestroyWidget(b->panel);
  b->panel = NULL;
  for (i=0; i<ch->info.data.E.no_str; i++)
    b->buttons[i] = NULL;
  b->buttonsSelected = -1;
  ch->d[BUTTON].w = NULL;
  ch->d[BUTTON].proc = NULL;
  ch->adjust.upMask &= ~ADJUST_BUTTONS_UP;
}
     

void createButtons(ch) 
atom *ch;
{
  short i, n;
  Arg  wargs[5];

  Buttons *b = &(ch->adjust.buttons);

  if ((ch->adjust.upMask & ADJUST_BUTTONS_UP)) {
    printf("createButtons : buttons not destoryed yet!\n");
    return;
  }

  n = 0;
  XtSetArg(wargs[n], XmNentryClass, xmToggleButtonWidgetClass); n++;
  XtSetArg(wargs[n], XmNnumColumns, 4); n++;
  b->panel = XmCreateRadioBox(ch->adjust.panel, "stateButton", wargs, n);
  for (i=0; i<ch->info.data.E.no_str; i++) {
    n = 0;
    if (i == ch->data.E.value) {
      b->buttonsSelected = i;
      XtSetArg(wargs[n],XmNset,TRUE); n++;
    }
    if (strlen(ch->info.data.E.strs[i]) > 0) {
      b->buttons[i] = XmCreateToggleButton(b->panel,
                         ch->info.data.E.strs[i],wargs,n);
    } else {
      b->buttons[i] = XmCreateToggleButton(b->panel,
                              stateButtonLabels[i],wargs,n);
    }
    XtAddCallback(b->buttons[i], 
             XmNvalueChangedCallback,adjustButtonsCallback,ch);
  }
  ch->d[BUTTON].w = b->panel;
  ch->d[BUTTON].proc = updateButtons;
  ch->adjust.upMask |= ADJUST_BUTTONS_UP;
  XtManageChild(b->panel);
  XtManageChildren(b->buttons,ch->info.data.E.no_str);
}

