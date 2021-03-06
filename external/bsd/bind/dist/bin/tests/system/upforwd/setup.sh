#!/bin/sh
#
# Copyright (C) 2004, 2007, 2011, 2012, 2014, 2016  Internet Systems Consortium, Inc. ("ISC")
# Copyright (C) 2000, 2001  Internet Software Consortium.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
# OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

cp -f ns1/example1.db ns1/example.db
rm -f ns1/example.db.jnl ns2/example.bk ns2/example.bk.jnl
rm -f ns1/example2.db.jnl ns2/example2.bk ns2/example2.bk.jnl
cp -f ns3/nomaster.db ns3/nomaster1.db
rm -f Ksig0.example2.*

#
# SIG(0) required cryptographic support which may not be configured.
#
test -r $RANDFILE || $GENRANDOM 400 $RANDFILE 
keyname=`$KEYGEN  -q -r $RANDFILE -n HOST -a RSASHA1 -b 1024 -T KEY sig0.example2 2>/dev/null | $D2U`
if test -n "$keyname"
then
	cat ns1/example1.db $keyname.key > ns1/example2.db
	echo $keyname > keyname
else
	cat ns1/example1.db > ns1/example2.db
	rm -f keyname
fi
