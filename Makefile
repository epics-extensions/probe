ADD_ON = ../..

include $(ADD_ON)/src/config/CONFIG

USR_CFLAGS = $(MOTIF_CFLAGS) $(X11_CFLAGS)
USR_LDFLAGS = $(MOTIF_LDFLAGS) $(X11_LDFLAGS) -lm

CC = acc
GCC = acc

SRCS = \
	probeAdjust.c\
	probeButtons.c\
	probeCa.c\
	probeDial.c\
	probeFormat.c\
	probeHistory.c\
	probeInfo.c\
	probeInit.c\
	probeOption.c\
	probeSlider.c\
	probeUpdate.c\
	probe_main.c
OBJS = \
	probeAdjust.o\
	probeButtons.o\
	probeCa.o\
	probeDial.o\
	probeFormat.o\
	probeHistory.o\
	probeInfo.o\
	probeInit.o\
	probeOption.o\
	probeSlider.o\
	probeUpdate.o\
	probe_main.o

PROD = probe

include $(ADD_ON)/src/config/RULES

probe: $(OBJS) $(DEP_LIBS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

