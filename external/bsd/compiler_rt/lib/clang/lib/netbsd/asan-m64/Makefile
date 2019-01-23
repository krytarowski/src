#	$NetBSD$

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.asan-x86_64

.PATH: ${TOPDIR}/lib/asan

SRCS+=	${ASAN_SOURCES}

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

_LIB_PREFIX=
MKDEBUGLIB=	no
MKLINT=		no
MKPICINSTALL=	no
MKPROFILE=	no
MKSTATICLIB=	yes

CPPFLAGS+=	-I${TOPDIR}/lib

COPTS+=		-fPIC

.include "../m64.mk"

.include "../syms.mk"

FILES=		libclang_rt.asan-x86_64.a.syms
FILESDIR=	${LIBDIR}

.include <bsd.lib.mk>
