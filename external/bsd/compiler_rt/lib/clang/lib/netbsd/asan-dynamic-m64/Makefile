#	$NetBSD$

NOSANITIZER=    # defined

.include "../common.mk"
.include "../sources.mk"

.include <bsd.own.mk>

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

LIB=	libclang_rt.asan-x86_64

.PATH: ${TOPDIR}/lib/asan

SRCS+=	${ASAN_SOURCES}
SRCS+=	${ASAN_CXX_SOURCES}

LIBDIR=		/usr/lib/clang/${CLANG_VER}/lib/netbsd
LIBISCXX=	yes

LIBISMODULE=	yes

CPPFLAGS+=	-I${TOPDIR}/lib

.include "../m64.mk"
.include "../shared_deps.mk"

.include <bsd.lib.mk>
