#	$NetBSD: Makefile.inc,v 1.2 2013/01/29 19:14:02 matt Exp $
#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

.PATH: ${ARCHDIR}/misc ${.CURDIR}/misc

# other sources shared with the kernel
.if ${MKLIBCSANITIZER:Uno} == "yes"
SRCS+=	ubsan.c
.endif

# constructor
SRCS+=	initfini.c

# for -fstack-protector
SRCS+=	stack_protector.c
