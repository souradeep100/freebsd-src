.include <bsd.init.mk>

.PATH:		${LUASRC}
.PATH:		${LIBLUASRC}

.include	"${BOOTSRC}/lua.mk"

LIB=		lua
INTERNALLIB=

# Core Lua.
SRCS=	lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
	lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
        ltm.c lundump.c lvm.c lzio.c
SRCS+=	lauxlib.c lbaselib.c lstrlib.c loadlib.c

# These aren't yet included, but link now, omitting them saves 15k
#SRCS+=	lcorolib.c ldblib.c lutf8lib.c

# These aren't yet compatible with the boot environment, and some may never be
#SRCS+=	lbitlib.c liolib.c lmathlib.c loslib.c ltablib.c

# Our utilities.
SRCS+=	lerrno.c lpager.c lstd.c lutils.c
SRCS+=	gfx_utils.c

.PATH:	${FLUASRC}/modules
SRCS+=	lfs.c
.PATH:	${FLUALIB}/libhash
SRCS+=	lhash.c

WARNS?=	3

CFLAGS+= -DLUA_PATH=\"${LUAPATH}\" -DLUA_PATH_DEFAULT=\"${LUAPATH}/\?.lua\"
CFLAGS+= -ffreestanding -nostdlib -DLUA_USE_POSIX
CFLAGS+= -fno-stack-protector -D__BSD_VISIBLE
CFLAGS+= -I${BOOTSRC}/include -I${LIBLUASRC} -I${LUASRC} -I${LDRSRC}

CFLAGS.gfx_utils.c+= -I${SRCTOP}/sys/teken -I${SRCTOP}/contrib/pnglite
CFLAGS.lhash.c+= -I${FLUALIB}/libhash -I${SRCTOP}/sys/crypto/sha2

.if ${MACHINE_CPUARCH} == "amd64" && ${DO32:U0} == 0
CFLAGS+=	-fPIC
.endif

.include "${BOOTSRC}/veriexec.mk"

.include <bsd.lib.mk>
