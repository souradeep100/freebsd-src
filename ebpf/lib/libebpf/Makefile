# $FreeBSD$

.include <bsd.init.mk>
.include <bsd.compiler.mk>

LIB= ebpf

SRCS = \
		ebpf_sys.c

CFLAGS+= -I${SRCTOP}/sys

CSTD=   c99

.include <bsd.lib.mk>
