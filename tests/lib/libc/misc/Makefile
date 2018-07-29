# $NetBSD$

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/misc

TESTS_C+=	t_ubsan
#TESTS_CXX+=	t_ubsanxx

.if ${MKSANITIZER:Uno} == "yes" || ${MKLIBCSANITIZER:Uno} == "yes"
# These tests are designed to be used against micro-UBSan only.
CPPFLAGS+=	-DENABLE_TESTS
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/lib/libc/misc
UBSAN_FLAGS=	-fsanitize=undefined
UBSAN_FLAGS+=	${${ACTIVE_CC} == "clang" :? -fsanitize=integer :}
UBSAN_FLAGS+=	${${ACTIVE_CC} == "clang" :? -fsanitize=nullability :}
CFLAGS+=	${UBSAN_FLAGS}
CXXFLAGS+=	${UBSAN_FLAGS}
.endif

.include <bsd.test.mk>
