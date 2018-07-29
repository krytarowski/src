# $NetBSD$

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/misc

TESTS_C+=	t_ubsan
TESTS_CXX+=	t_ubsanxx

.if ${MKSANITIZER} == "yes" || ${MKLIBCSANITIZER} == "yes"
# These tests are designed to be used against micro-UBSan only.
CPPFLAGS+=	-DENABLE_TESTS
CPPFLAGS+=	-I${NETBSDSRCDIR}/common/lib/libc/misc
CFLAGS+=	-fsanitize=undefined -fsanitize=integer
CXXFLAGS+=	-fsanitize=undefined -fsanitize=integer
.endif

.include <bsd.test.mk>
