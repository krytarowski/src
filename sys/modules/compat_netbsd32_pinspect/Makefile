#	$NetBSD$

.include "../Makefile.inc"
.include "../Makefile.assym"

KMOD=	compat_netbsd32_pinspect

CPPFLAGS+=	-DCOMPAT_NETBSD32
CPPFLAGS+=	-DPINSPECT
CPPFLAGS+=	-I${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

.PATH:	${S}/compat/netbsd32
SRCS+=	netbsd32_pinspect.c

.if !empty(MACHINE_ARCH:Mmips64*)
WARNS=	3
.endif

.include <bsd.kmodule.mk>
