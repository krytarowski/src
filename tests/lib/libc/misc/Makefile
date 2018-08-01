# $NetBSD$

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/misc

TESTS_C+=	t_ubsan
TESTS_CXX+=	t_ubsanxx

.PATH:		${NETBSDSRCDIR}/common/lib/libc/misc
SRCS.t_ubsan=	t_ubsan.c ubsan.c
SRCS.t_ubsanxx=	t_ubsanxx.cpp ubsan.c

.if ${MKSANITIZER:Uno} == "yes" || ${MKLIBCSANITIZER:Uno} == "yes"
# These tests are designed to be used against micro-UBSan only.
CPPFLAGS+=		-DENABLE_TESTS
UBSAN_FLAGS=		-fsanitize=undefined
UBSAN_FLAGS+=		${${ACTIVE_CC} == "clang" :? -fsanitize=integer :}
UBSAN_FLAGS+=		${${ACTIVE_CC} == "clang" :? -fsanitize=nullability :}
CFLAGS+=		${UBSAN_FLAGS}
CXXFLAGS+=		${UBSAN_FLAGS}
CWARNFLAGS+=		-Wno-return-type
CWARNFLAGS.clang+=	-Wno-incompatible-pointer-types-discards-qualifiers
CWARNFLAGS.clang+=	-Wno-nullability-completeness 
.endif

.include <bsd.test.mk>
