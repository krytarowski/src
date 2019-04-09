#	$NetBSD$
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPINSPECT

KMOD=	pinspect_common
SRCS=	sys_pinspect_common.c

WARNS=	3

.include <bsd.kmodule.mk>
