# GNU Makefile for Quake II and modifications.
#
# Usage: just type "make".
#
# Configuration: the BUILD_* variables indicate the building of a component,
# and the WITH_* variables set options for these components. The "TYPE"
# variable indicates the type of build to do (release, debug and profile).
#
# Written by Alejandro Pulver for QuDos.
#

# Targets for building.

# FIXME: do not build dependencies when not building (define a variable and
# check for it before including files).

# OS type and architecture detection
ARCH:=		$(shell uname -m | sed -e 's/i.86/i386/')
OSTYPE:=	$(shell uname -s)

# Client and Renderers
BUILD_QUAKE2?=YES	# Build client (OSS sound, cdrom ioctls for cd audio).
BUILD_DEDICATED?=YES	# Build dedicated server.
BUILD_GLX?=YES		# Build OpenGL renderer.
BUILD_SDLGL?=YES	# Build SDL OpenGL renderer.
ifeq ($(OSTYPE),Linux)
BUILD_ALSA_SND?=YES     # Enable support for ALSA (default sound on 2.6 Linux).
endif
BUILD_OSS_SND?=YES      # Enable support for OSS sound.
BUILD_SDL_SND?=YES      # Enable support for SDL (default) sound.

# Mods
BUILD_GAME?=YES		# Build original game modification (game$(ARCH).so).
BUILD_3ZB2?=YES		# Build 3zb2 (bots) modification.
BUILD_CTF?=YES		# Build CTF (Capture The Flag) modification.
BUILD_JABOT?=YES	# Build JABot (bots) modification.
BUILD_ROGUE?=YES	# Build Rogue modification.
BUILD_XATRIX?=YES	# Build Xatrix modification.
BUILD_ZAERO?=YES	# Build Zaero modification.

# Configurable options.
WITH_BOTS?=YES		# Enable Ace Bot support in modifications (Quake2, Rogue, Xatrix and Zaero).
WITH_DATADIR?=YES	# Read from $(DATADIR) and write to "~/.quake2".
WITH_DGA_MOUSE?=NO	# Enable DGA mouse extension.
WITH_GAME_MOD?=YES	# Enable custom addons in the main modification (Quake2, Rogue, Xatrix and Zaero).
WITH_IPV6?=NO		# Enable IPv6 support. Tested on FreeBSD.
WITH_JOYSTICK?=YES	# Enable joystick support.
WITH_LIBDIR?=YES	# Read data and renderers from $(LIBDIR).
WITH_QMAX?=YES		# Enable fancier OpenGL graphics.
WITH_REDBLUE?=YES	# Enable red-blue 3d glasses renderer.
WITH_RETEXTURE?=YES	# Enable retextured graphics support.
WITH_X86_ASM?=YES	# Enable x86 assembly code (only for i386).

# General variables.
#TODO: Find better solution for Launchpad builds
#Destanaion directory
DESTDIR?=
#Prefix where include are
LOCALBASE?=	/usr
PREFIX?=	$(DESTDIR)$(LOCALBASE)

DATADIR?=	$(PREFIX)/share/games/quake2
LIBDIR?=	$(PREFIX)/lib/qudos
BINDIR?=	$(PREFIX)/games

CC?=		gcc
TYPE?=		release
#TYPE?=		debug

OGG_LDFLAGS=	-lvorbisfile -lvorbis -logg

SDL_CONFIG?=	sdl-config
SDL_CFLAGS=	$(shell $(SDL_CONFIG) --cflags)
SDL_LDFLAGS=	$(shell $(SDL_CONFIG) --libs)

ifeq ($(OSTYPE),Linux)
ALSA_LDFLAGS=	-lasound
endif

SHLIB_EXT=	so
SHLIB_CFLAGS=	-fPIC
SHLIB_LDFLAGS=	-shared

# QuDos variables.
VERSION=	QuDos v0.40.3.3
VERSION_BZ2=	v0.40.3.3
SRC_DIR=	src
MOD_DIR=	mods
OBJ_DIR=	QuDos-build
BIN_DIR=	qudos

GAME_NAME=	game.$(SHLIB_EXT)

ifeq ($(OSTYPE),Linux)
  ifeq ($(ARCH),i386)
GAME_NAME=	game$(ARCH).$(SHLIB_EXT)
  endif
endif

# Compilation flags.
CFLAGS+=	-I/usr/include -I$(LOCALBASE)/include \
		-DGAME_NAME='"$(GAME_NAME)"' -DQUDOS_VERSION='"$(VERSION)"'

WARNS=	-Wshadow -Wpointer-arith -Wcast-align -Waggregate-return -Wstrict-prototypes -Wredundant-decls -Wnested-externs

ifeq ($(TYPE),debug)
CFLAGS+=	-Wall -g -ggdb -DDEBUGGING $(WARNS)
else
  ifeq ($(TYPE),profile)
CFLAGS+=	-pg
  else
CFLAGS+=	-O3 -ffast-math -funroll-loops -fomit-frame-pointer \
		-fexpensive-optimizations
    ifeq ($(ARCH),i386)
CFLAGS+=	-falign-loops=2 -falign-jumps=2 -falign-functions=2 \
		-fno-strict-aliasing
    endif
  endif
endif

# Linker flags.
LDFLAGS+=	-L/usr/lib -L$(LOCALBASE)/lib -lm

ifeq ($(OSTYPE),Linux)
LDFLAGS+=	-ldl
endif

REF_LDFLAGS=	-lX11 -lXext -lXxf86vm -lGLU -ljpeg -lpng

ifeq ($(strip $(WITH_DGA_MOUSE)),YES)
REF_LDFLAGS+= -lXxf86dga
endif

# Sources and object code files list.
QuDos_cl_SRCS=	client/cl_cin.c \
		client/cl_ents.c \
		client/cl_fx.c \
		client/cl_input.c \
		client/cl_inv.c \
		client/cl_locs.c \
		client/cl_main.c \
		client/cl_newfx.c \
		client/cl_parse.c \
		client/cl_pred.c \
		client/cl_scrn.c \
		client/cl_tent.c \
		client/cl_view.c \
		client/console.c \
		client/keys.c \
		client/menu.c \
		client/qmenu.c \
		client/snd_dma.c \
		client/snd_mem.c \
		client/snd_mix.c \
		client/snd_ogg.c \
		game/m_flash.c \
		unix/vid_menu.c \
		unix/vid_so.c

QuDos_com_SRCS=	game/q_shared.c \
		\
		qcommon/cmd.c \
		qcommon/cmodel.c \
		qcommon/common.c \
		qcommon/crc.c \
		qcommon/cvar.c \
		qcommon/files.c \
		qcommon/md4.c \
		qcommon/net_chan.c \
		qcommon/pmove.c \
		qcommon/unzip/ioapi.c \
		qcommon/unzip/unzip.c \
		qcommon/wildcard.c \
		\
		server/sv_ccmds.c \
		server/sv_ents.c \
		server/sv_game.c \
		server/sv_init.c \
		server/sv_main.c \
		server/sv_send.c \
		server/sv_user.c \
		server/sv_world.c \
		\
		unix/$(NET_API).c \
		unix/qsh_unix.c \
		unix/sys_unix.c

QuDos_SRCS=	$(QuDos_com_SRCS) \
		$(QuDos_cl_SRCS) \
		unix/$(CD_API).c

QuDos_BIN=	QuDos
QuDos_OBJS=	$(QuDos_SRCS:%.c=%.o)
QuDos_CFLAGS=	$(CFLAGS) -DQ2_BIN
QuDos_LDFLAGS=	$(LDFLAGS) $(OGG_LDFLAGS) -lz

QuDos_ded_SRCS=	$(QuDos_com_SRCS) \
		null/cd_null.c \
		null/cl_null.c

QuDos_ded_BIN=	QuDos-ded
QuDos_ded_OBJS=	$(QuDos_ded_SRCS:%.c=%.o)
QuDos_ded_CFLAGS=	$(CFLAGS) -DDEDICATED_ONLY -DQ2DED_BIN
QuDos_ded_LDFLAGS=	$(LDFLAGS) -lz

ref_com_SRCS=	game/q_shared.c \
		\
		ref_gl/gl_blooms.c \
		ref_gl/gl_decals.c \
		ref_gl/gl_draw.c \
		ref_gl/gl_flares.c \
		ref_gl/gl_image.c \
		ref_gl/gl_light.c \
		ref_gl/gl_md3.c \
		ref_gl/gl_mesh.c \
		ref_gl/gl_model.c \
		ref_gl/gl_refl.c \
		ref_gl/gl_rmain.c \
		ref_gl/gl_rmisc.c \
		ref_gl/gl_rsurf.c \
		ref_gl/gl_vlights.c \
		ref_gl/gl_warp.c \
		\
		unix/qgl_unix.c \
		unix/qsh_unix.c \
		unix/rw_unix.c

ref_glx_SRCS=	$(ref_com_SRCS) \
		unix/gl_glx.c

ref_glx_BIN=	ref_q2glx.$(SHLIB_EXT)
ref_glx_OBJS=	$(ref_glx_SRCS:%.c=%.o)
ref_glx_CFLAGS=	$(CFLAGS) $(SHLIB_CFLAGS) -DREF_GL
ref_glx_LDFLAGS=	$(SHLIB_LDFLAGS) $(LDFLAGS) $(REF_LDFLAGS)

ref_sdlgl_SRCS=	$(ref_com_SRCS) \
		unix/gl_sdl.c

ref_sdlgl_BIN=	ref_q2sdlgl.$(SHLIB_EXT)
ref_sdlgl_OBJS=	$(ref_sdlgl_SRCS:%.c=%.o)
ref_sdlgl_CFLAGS=	$(CFLAGS) $(SDL_CFLAGS) $(SHLIB_CFLAGS) -DREF_SDLGL
ref_sdlgl_LDFLAGS=	$(SHLIB_LDFLAGS) $(LDFLAGS) $(SDL_LDFLAGS) \
			$(REF_LDFLAGS)

ifeq ($(OSTYPE),Linux)
snd_alsa_SRCS=	unix/snd_alsa.c

snd_alsa_BIN=	snd_alsa.$(SHLIB_EXT)
snd_alsa_OBJS=	$(snd_alsa_SRCS:%.c=%.o)
snd_alsa_CFLAGS=	$(CFLAGS) $(ALSA_CFLAGS) $(SHLIB_CFLAGS)
snd_alsa_LDFLAGS=	$(SHLIB_LDFLAGS) $(LDFLAGS) $(ALSA_LDFLAGS)
endif

snd_oss_SRCS=	unix/snd_oss.c

snd_oss_BIN=	snd_oss.$(SHLIB_EXT)
snd_oss_OBJS=	$(snd_oss_SRCS:%.c=%.o)
snd_oss_CFLAGS=		$(CFLAGS) $(SHLIB_CFLAGS)
snd_oss_LDFLAGS=	$(SHLIB_LDFLAGS) $(LDFLAGS)

snd_sdl_SRCS=	unix/snd_sdl.c

snd_sdl_BIN=	snd_sdl.$(SHLIB_EXT)
snd_sdl_OBJS=	$(snd_sdl_SRCS:%.c=%.o)
snd_sdl_CFLAGS=		$(CFLAGS) $(SDL_CFLAGS) $(SHLIB_CFLAGS)
snd_sdl_LDFLAGS=	$(SHLIB_LDFLAGS) $(LDFLAGS) $(SDL_LDFLAGS)

# Targets to build.
ifeq ($(strip $(BUILD_3ZB2)),YES)
TARGETS_GAME+=	$(MOD_DIR)/3zb2
endif

ifeq ($(strip $(BUILD_CTF)),YES)
TARGETS_GAME+=	$(MOD_DIR)/ctf
endif

ifeq ($(strip $(BUILD_DEDICATED)),YES)
TARGETS+=	QuDos_ded
endif

ifeq ($(strip $(BUILD_GAME)),YES)
TARGETS_GAME+=	game
endif

ifeq ($(strip $(BUILD_GLX)),YES)
TARGETS+=	ref_glx
endif

ifeq ($(strip $(BUILD_JABOT)),YES)
TARGETS_GAME+=	$(MOD_DIR)/jabot
endif

ifeq ($(strip $(BUILD_QUAKE2)),YES)
TARGETS+=	QuDos
endif

ifeq ($(strip $(BUILD_ROGUE)),YES)
TARGETS_GAME+=	$(MOD_DIR)/rogue
endif

ifeq ($(strip $(BUILD_SDLGL)),YES)
TARGETS+=	ref_sdlgl
endif

ifeq ($(strip $(BUILD_XATRIX)),YES)
TARGETS_GAME+=	$(MOD_DIR)/xatrix
endif

ifeq ($(strip $(BUILD_ZAERO)),YES)
TARGETS_GAME+=	$(MOD_DIR)/zaero
endif

ifeq ($(OSTYPE),Linux)
 ifeq ($(strip $(BUILD_ALSA_SND)),YES)
  TARGETS+=	snd_alsa
 endif
endif

ifeq ($(strip $(BUILD_OSS_SND)),YES)
TARGETS+=	snd_oss
endif

ifeq ($(strip $(BUILD_SDL_SND)),YES)
TARGETS+=	snd_sdl
endif

# System dependent options.
ifeq ($(OSTYPE),FreeBSD)
CD_API=		cd_freebsd
else
  ifeq ($(OSTYPE),Linux)
CD_API=		cd_linux
  else
CD_API=		cd_null
  endif
endif

# Option detection and handling.
ifeq ($(OSTYPE),Linux)
 ifeq ($(strip $(BUILD_ALSA_SND)),YES)
 SOUND_API=	snd_alsa
 endif
endif

ifeq ($(strip $(BUILD_SDL_SND)),YES)
SOUND_API=	snd_sdl
endif

ifeq ($(strip $(BUILD_OSS_SND)),YES)
SOUND_API=	snd_oss
endif

ifeq ($(strip $(WITH_BOTS)),YES)
CFLAGS+=	-DWITH_ACEBOT
endif

ifeq ($(strip $(WITH_DATADIR)),YES)
CFLAGS+=	-DDATADIR='"$(DATADIR)"'
endif

ifeq ($(strip $(WITH_DGA_MOUSE)),YES)
CFLAGS+=	-DUSE_XF86_DGA
endif

ifeq ($(strip $(WITH_GAME_MOD)),YES)
CFLAGS+=	-DGAME_MOD
endif

ifeq ($(strip $(WITH_IPV6)),YES)
CFLAGS+=	-DHAVE_IPV6
NET_API=	net_udp6
  ifeq ($(OSTYPE),FreeBSD)
CFLAGS+=	-DHAVE_SIN6_LEN
  endif
else
NET_API=	net_udp
endif

ifeq ($(strip $(WITH_JOYSTICK)),YES)
CFLAGS+=	-DJoystick
ref_com_SRCS+=	unix/joystick.c
endif

ifeq ($(strip $(WITH_LIBDIR)),YES)
CFLAGS+=	-DLIBDIR='"$(LIBDIR)"'
endif

ifeq ($(strip $(WITH_QMAX)),YES)
CFLAGS+=	-DQMAX
endif

ifeq ($(strip $(WITH_REDBLUE)),YES)
CFLAGS+=	-DREDBLUE
endif

ifeq ($(strip $(WITH_RETEXTURE)),YES)
CFLAGS+=	-DRETEX
endif

ifeq ($(ARCH),i386)
  ifeq ($(strip $(WITH_X86_ASM)),YES)
QuDos_OBJS+=	snd_mixa.o
  else
CFLAGS+=	-DC_ONLY
  endif
endif

# Targets for building.
.PHONY: clean distclean distclean_full bin_clean obj_clean src_clean

DEPEND_TARGETS=	$(patsubst %,$(OBJ_DIR)/%/.depend,$(TARGETS)) \
		$(patsubst %,$(OBJ_DIR)/%/.depend,$(notdir $(TARGETS_GAME)))

BUILD_TARGETS=	$(foreach target,$(TARGETS),$(patsubst %,$(BIN_DIR)/%,$($(target)_BIN))) \
		$(foreach target,$(TARGETS_GAME),$(patsubst %,$(BIN_DIR)/%/$(GAME_NAME),$(notdir $(target))))

all: build

build: $(DEPEND_TARGETS) $(BUILD_TARGETS)

install: build
ifeq ($(strip $(BUILD_QUAKE2)),YES)
	@printf "Installing client...\n"
	@install -D -m 755 $(BIN_DIR)/$(QuDos_BIN) $(PREFIX)/games/$(QuDos_BIN)
	@install -D -m 644 data/QuDos.desktop $(PREFIX)/share/applications/QuDos.desktop
	@install -D -m 644 data/quake2.png $(PREFIX)/share/icons/hicolor/128x128/apps/quake2.png
endif
ifeq ($(strip $(BUILD_DEDICATED)),YES)
	@printf "Installing dedicated server...\n"
	@install -D -m 755 $(BIN_DIR)/$(QuDos_ded_BIN) $(PREFIX)/games/$(QuDos_ded_BIN)
endif
ifeq ($(strip $(BUILD_GLX)),YES)
	@printf "Installing OpenGL renderer...\n"
	@install -D -m 644 $(BIN_DIR)/$(ref_glx_BIN) $(LIBDIR)/$(ref_glx_BIN)
endif
ifeq ($(strip $(BUILD_SDLGL)),YES)
	@printf "Installing SDL OpenGL renderer...\n"
	@install -D -m 644 $(BIN_DIR)/$(ref_sdlgl_BIN) $(LIBDIR)/$(ref_sdlgl_BIN)
endif
ifeq ($(strip $(BUILD_ALSA_SND)),YES)
	@printf "Installing support for ALSA...\n"
	@install -D -m 644 $(BIN_DIR)/$(snd_alsa_BIN) $(LIBDIR)/$(snd_alsa_BIN)
endif
ifeq ($(strip $(BUILD_OSS_SND)),YES)
	@printf "Installing support for OSS sound...\n"
	@install -D -m 644 $(BIN_DIR)/$(snd_oss_BIN) $(LIBDIR)/$(snd_oss_BIN)
endif
ifeq ($(strip $(BUILD_SDL_SND)),YES)
	@printf "Installing support for SDL sound...\n"
	@install -D -m 644 $(BIN_DIR)/$(snd_sdl_BIN) $(LIBDIR)/$(snd_sdl_BIN)
endif
ifeq ($(strip $(BUILD_GAME)),YES)
	@printf "Installing original game modification...\n"
	@install -D -m 644 $(BIN_DIR)/baseq2/$(GAME_NAME) $(LIBDIR)/baseq2/$(GAME_NAME)
endif
ifeq ($(strip $(BUILD_3ZB2)),YES)
	@printf "Installing 3zb2 (bots) modification...\n"
	@install -D -m 644 $(BIN_DIR)/3zb2/$(GAME_NAME) $(LIBDIR)/3zb2/$(GAME_NAME)
endif
ifeq ($(strip $(BUILD_CTF)),YES)
	@printf "Installing CTF (Capture The Flag) modification...\n"
	@install -D -m 644 $(BIN_DIR)/ctf/$(GAME_NAME) $(LIBDIR)/ctf/$(GAME_NAME)
endif
ifeq ($(strip $(BUILD_JABOT)),YES)
	@printf "Installing JABot (bots) modification...\n"
	@install -D -m 644 $(BIN_DIR)/jabot/$(GAME_NAME) $(LIBDIR)/jabot/$(GAME_NAME)
endif
ifeq ($(strip $(BUILD_ROGUE)),YES)
	@printf "Installing Rogue modification...\n"
	@install -D -m 644 $(BIN_DIR)/rogue/$(GAME_NAME) $(LIBDIR)/rogue/$(GAME_NAME)
endif
ifeq ($(strip $(BUILD_XATRIX)),YES)
	@printf "Installing Xatrix modification...\n"
	@install -D -m 644 $(BIN_DIR)/xatrix/$(GAME_NAME) $(LIBDIR)/xatrix/$(GAME_NAME)
endif
ifeq ($(strip $(BUILD_ZAERO)),YES)
	@printf "Installing Zaero modification...\n"
	@install -D -m 644 $(BIN_DIR)/zaero/$(GAME_NAME) $(LIBDIR)/zaero/$(GAME_NAME)
endif
ifeq ($(strip $(WITH_QMAX)),YES)
	@printf "Installing additional graphics...\n"
	@install -D -m 644 quake2/baseq2/qudos.pk3 $(PREFIX)/share/games/quake2/baseq2/qudos.pk3
endif
	@printf "Done.\n"

deb: 
	@debuild -B

clean: obj_clean

distclean: bin_clean obj_clean

distclean_full: bin_clean obj_clean src_clean

bin_clean:
	@printf "Cleaning binaries... "
	@rm -rf $(BIN_DIR)
	@printf "Done.\n"

obj_clean:
	@printf "Cleaning object code files... "
	@rm -rf $(OBJ_DIR)
	@printf "Done.\n"

src_clean:
	@printf "Cleaning backup files generated by text editors... "
	@find . -type f \( -name "*~" -or -name "*.orig" \) -delete
	@rm -f tags
	@printf "Done.\n"
bz2:
	@printf "Creating bzip2 compressed file ...\n"
	@cp data/qudos.pk3 quake2/baseq2
ifeq ($(strip $(WITH_QMAX)),YES)
	@tar cjvf QuDosmaX-$(VERSION_BZ2).tar.bz2 quake2 docs/license.txt docs/QuDos.txt
else
	@tar cjvf QuDos-$(VERSION_BZ2).tar.bz2 quake2 docs/license.txt docs/QuDos.txt
endif
	@printf "Done.\n"
# Auto-generate rules.
$(OBJ_DIR)/QuDos/.depend: $(patsubst %,$(SRC_DIR)/%,$(QuDos_SRCS))
	@mkdir -p $(OBJ_DIR)/QuDos $(BIN_DIR)
	$(CC) -MM $(QuDos_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/QuDos/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(QuDos_CFLAGS) -o $$@ $$<' > $@

$(OBJ_DIR)/QuDos_ded/.depend: $(patsubst %,$(SRC_DIR)/%,$(QuDos_ded_SRCS))
	@mkdir -p $(OBJ_DIR)/QuDos_ded $(BIN_DIR)
	$(CC) -MM $(QuDos_ded_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/QuDos_ded/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(QuDos_ded_CFLAGS) -o $$@ $$<' > $@

$(OBJ_DIR)/ref_glx/.depend: $(patsubst %,$(SRC_DIR)/%,$(ref_glx_SRCS))
	@mkdir -p $(OBJ_DIR)/ref_glx $(BIN_DIR)
	$(CC) -MM $(ref_glx_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/ref_glx/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(ref_glx_CFLAGS) -o $$@ $$<' > $@

$(OBJ_DIR)/ref_sdlgl/.depend: $(patsubst %,$(SRC_DIR)/%,$(ref_sdlgl_SRCS))
	@mkdir -p $(OBJ_DIR)/ref_sdlgl $(BIN_DIR)
	$(CC) -MM $(ref_sdlgl_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/ref_sdlgl/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(ref_sdlgl_CFLAGS) -o $$@ $$<' > $@

ifeq ($(OSTYPE),Linux)
$(OBJ_DIR)/snd_alsa/.depend: $(patsubst %,$(SRC_DIR)/%,$(snd_alsa_SRCS))
	@mkdir -p $(OBJ_DIR)/snd_alsa $(BIN_DIR)
	$(CC) -MM $(snd_alsa_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/snd_alsa/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(snd_alsa_CFLAGS) -o $$@ $$<' > $@
endif

$(OBJ_DIR)/snd_oss/.depend: $(patsubst %,$(SRC_DIR)/%,$(snd_oss_SRCS))
	@mkdir -p $(OBJ_DIR)/snd_oss $(BIN_DIR)
	$(CC) -MM $(snd_oss_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/snd_oss/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(snd_oss_CFLAGS) -o $$@ $$<' > $@

$(OBJ_DIR)/snd_sdl/.depend: $(patsubst %,$(SRC_DIR)/%,$(snd_sdl_SRCS))
	@mkdir -p $(OBJ_DIR)/snd_sdl $(BIN_DIR)
	$(CC) -MM $(snd_sdl_CFLAGS) $^ | sed -e 's|^\(..*\.o\)|$(OBJ_DIR)/snd_sdl/\1|' | awk -f rules.awk -v rule='\t$$(CC) -c $$(snd_sdl_CFLAGS) -o $$@ $$<' > $@

# Object lists relative to $(OBJ_DIR) and without path.
QuDos_OL=	$(patsubst %,$(OBJ_DIR)/QuDos/%,$(notdir $(QuDos_OBJS)))
QuDos_ded_OL=	$(patsubst %,$(OBJ_DIR)/QuDos_ded/%,$(notdir $(QuDos_ded_OBJS)))
ref_glx_OL=	$(patsubst %,$(OBJ_DIR)/ref_glx/%,$(notdir $(ref_glx_OBJS)))
ref_sdlgl_OL=	$(patsubst %,$(OBJ_DIR)/ref_sdlgl/%,$(notdir $(ref_sdlgl_OBJS)))
ifeq ($(OSTYPE),Linux)
snd_alsa_OL=	$(patsubst %,$(OBJ_DIR)/snd_alsa/%,$(notdir $(snd_alsa_OBJS)))
endif
snd_oss_OL=	$(patsubst %,$(OBJ_DIR)/snd_oss/%,$(notdir $(snd_oss_OBJS)))
snd_sdl_OL=	$(patsubst %,$(OBJ_DIR)/snd_sdl/%,$(notdir $(snd_sdl_OBJS)))

# Assembly rules.
$(OBJ_DIR)/QuDos/snd_mixa.o: $(SRC_DIR)/unix/snd_mixa.s
	$(CC) -c $(CFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<

# Linking rules.
$(BIN_DIR)/$(QuDos_BIN): $(QuDos_OL)
	$(CC) -o $@ $^ $(QuDos_LDFLAGS)

$(BIN_DIR)/$(QuDos_ded_BIN): $(QuDos_ded_OL)
	$(CC) -o $@ $^ $(QuDos_ded_LDFLAGS)

$(BIN_DIR)/$(ref_glx_BIN): $(ref_glx_OL)
	$(CC) -o $@ $^ $(ref_glx_LDFLAGS)

$(BIN_DIR)/$(ref_sdlgl_BIN): $(ref_sdlgl_OL)
	$(CC) -o $@ $^ $(ref_sdlgl_LDFLAGS)

ifeq ($(OSTYPE),Linux)
$(BIN_DIR)/$(snd_alsa_BIN): $(snd_alsa_OL)
	$(CC) -o $@ $^ $(snd_alsa_LDFLAGS)
endif

$(BIN_DIR)/$(snd_oss_BIN): $(snd_oss_OL)
	$(CC) -o $@ $^ $(snd_oss_LDFLAGS)

$(BIN_DIR)/$(snd_sdl_BIN): $(snd_sdl_OL)
	$(CC) -o $@ $^ $(snd_sdl_LDFLAGS)

# Include make modules (for mods).
-include $(patsubst %,$(SRC_DIR)/%/module.mk,$(TARGETS_GAME))

# Include dependencies.
-include $(DEPEND_TARGETS)
