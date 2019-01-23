#	$NetBSD$

TOPDIR=	${NETBSDSRCDIR}/sys/external/bsd/compiler_rt/dist

NOOBJ=	# defined

.PATH:	${TOPDIR}/lib/asan
INCS+=	asan_blacklist.txt

.PATH:	${TOPDIR}/lib/msan
INCS+=	msan_blacklist.txt

.include <bsd.own.mk>

INCSDIR=	/usr/lib/clang/7.0.0/share

.include <bsd.prog.mk>
