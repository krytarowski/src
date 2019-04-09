#	$NetBSD$
#
.include "../Makefile.inc"

.PATH:	${S}/kern

CPPFLAGS+=	-DPINSPECT

KMOD=	pinspect
SRCS=	sys_pinspect.c

.include <bsd.kmodule.mk>
