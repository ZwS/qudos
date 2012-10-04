
# Check OS type.
OSTYPE := $(shell uname -s)

WITH_DGA_MOUSE?=YES	# Enable DGA mouse extension.
WITH_QMAX?=YES		# Enable fancier OpenGL graphics.

CC=gcc
CC_VERSION=$(shell $(CC) -dumpversion | sed -e 's/\..*//g')

BASE_CFLAGS=


RELEASE_CFLAGS=$(BASE_CFLAGS) -ffast-math -fexpensive-optimizations

ifneq ($(CC_VERSION),4) 
  RELEASE_CFLAGS+=-funroll-loops
endif

RELEASE_CFLAGS+= -O3 -falign-loops=2 -falign-jumps=2 -falign-functions=2 -fno-strict-aliasing

DEBUG_CFLAGS=$(BASE_CFLAGS) -g -ggdb -Wall -pipe

LDFLAGS=-lm

ifeq ($(OSTYPE),Linux)
LDFLAGS+=-ldl
endif

CFLAGS = $(RELEASE_CFLAGS)
GLXLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86vm -lGL -lGLU -ljpeg -lpng

ifeq ($(strip $(WITH_DGA_MOUSE)),YES)
CFLAGS+= -DUSE_XF86_DGA
GLXLDFLAGS+= -lXxf86dga
endif

ifeq ($(strip $(WITH_QMAX)),YES)
CFLAGS+= -DQMAX
endif

GLX_SRCS :=	gl_blooms.c \
                gl_decals.c \
                gl_draw.c \
                gl_flares.c \
                gl_image.c \
                gl_light.c \
                gl_md3.c \
                gl_mesh.c \
                gl_model.c \
                gl_refl.c \
                gl_rmain.c \
                gl_rmisc.c \
                gl_rsurf.c \
                gl_vlights.c \
                gl_warp.c \
                ../unix/qsh_unix.c \
                ../unix/qgl_unix.c \
                ../unix/rw_unix.c \
                ../unix/gl_glx.c \
                ../game/q_shared.c
	
GLX_OBJS = $(GLX_SRCS:.c=.o)

SHLIBLDFLAGS=-shared

TARGETS=ref_q2glx.so 
        
ref_q2glx.so : $(GLX_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(SHLIBLDFLAGS) $(GLXLDFLAGS)
debug:
	$(MAKE) targets CFLAGS="$(DEBUG_CFLAGS)"
        
release:
	$(MAKE) targets CFLAGS="$(RELEASE_CFLAGS)"
        
all: ref_q2glx.so

install:
	@printf "Moving file to your quake2 directory...\n"
	@mv -fv ref_q2glx.so ~/quake2
	@printf "Done.\n"

targets: $(TARGETS)

clean:		
	rm -f $(GLX_OBJS) $(TARGETS)