# Copyright (C) 2005, 2007, 2010-2018  Internet Systems Consortium, Inc. ("ISC")
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

# Id

SYSTEMTESTTOP=..
. $SYSTEMTESTTOP/conf.sh

status=0
n=0

n=`expr $n + 1`
echo "I: checking that named-checkconf handles a known good config ($n)"
ret=0
$CHECKCONF good.conf > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf prints a known good config ($n)"
ret=0
awk 'BEGIN { ok = 0; } /cut here/ { ok = 1; getline } ok == 1 { print }' good.conf > good.conf.in
[ -s good.conf.in ] || ret=1
$CHECKCONF -p good.conf.in | grep -v '^good.conf.in:' > good.conf.out 2>&1 || ret=1
cmp good.conf.in good.conf.out || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -x removes secrets ($n)"
ret=0
# ensure there is a secret and that it is not the check string.
grep 'secret "' good.conf.in > /dev/null || ret=1
grep 'secret "????????????????"' good.conf.in > /dev/null 2>&1 && ret=1
$CHECKCONF -p -x good.conf.in | grep -v '^good.conf.in:' > good.conf.out 2>&1 || ret=1
grep 'secret "????????????????"' good.conf.out > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

for bad in bad*.conf
do
    n=`expr $n + 1`
    echo "I: checking that named-checkconf detects error in $bad ($n)"
    ret=0
    $CHECKCONF $bad > checkconf.out 2>&1
    if [ $? != 1 ]; then ret=1; fi
    grep "^$bad:[0-9]*: " checkconf.out > /dev/null || ret=1
    case $bad in
    bad-update-policy[123].conf)
	pat="identity and name fields are not the same"
	grep "$pat" checkconf.out > /dev/null || ret=1
	;;
    bad-update-policy*.conf)
	pat="name field not set to placeholder value"
	grep "$pat" checkconf.out > /dev/null || ret=1
	;;
    esac
    if [ $ret != 0 ]; then echo "I:failed"; fi
    status=`expr $status + $ret`
done

for good in good-*.conf
do
	n=`expr $n + 1`
	echo "I: checking that named-checkconf detects no error in $good ($n)"
	ret=0
	$CHECKCONF $good > /dev/null 2>&1
	if [ $? != 0 ]; then echo "I:failed"; ret=1; fi
	status=`expr $status + $ret`
done

n=`expr $n + 1`
echo "I: checking that named-checkconf -z catches missing hint file ($n)"
ret=0
$CHECKCONF -z hint-nofile.conf > hint-nofile.out 2>&1 && ret=1
grep "could not configure root hints from 'nonexistent.db': file not found" hint-nofile.out > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf catches range errors ($n)"
ret=0
$CHECKCONF range.conf > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf warns of notify inconsistencies ($n)"
ret=0
warnings=`$CHECKCONF notify.conf 2>&1 | grep "'notify' is disabled" | wc -l`
[ $warnings -eq 3 ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking named-checkconf dnssec warnings ($n)"
ret=0
$CHECKCONF dnssec.1 2>&1 | grep 'validation yes.*enable no' > /dev/null || ret=1
$CHECKCONF dnssec.2 2>&1 | grep 'auto-dnssec may only be ' > /dev/null || ret=1
$CHECKCONF dnssec.2 2>&1 | grep 'validation auto.*enable no' > /dev/null || ret=1
$CHECKCONF dnssec.2 2>&1 | grep 'validation yes.*enable no' > /dev/null || ret=1
# this one should have no warnings
$CHECKCONF dnssec.3 2>&1 | grep '.*' && ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: range checking fields that do not allow zero ($n)"
ret=0
for field in max-retry-time min-retry-time max-refresh-time min-refresh-time; do
    cat > badzero.conf << EOF
options {
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > /dev/null 2>&1
    [ $? -eq 1 ] || { echo "I: options $field failed" ; ret=1; }
    cat > badzero.conf << EOF
view dummy {
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > /dev/null 2>&1
    [ $? -eq 1 ] || { echo "I: view $field failed" ; ret=1; }
    cat > badzero.conf << EOF
options {
    $field 0;
};
view dummy {
};
EOF
    $CHECKCONF badzero.conf > /dev/null 2>&1
    [ $? -eq 1 ] || { echo "I: options + view $field failed" ; ret=1; }
    cat > badzero.conf << EOF
zone dummy {
    type slave;
    masters { 0.0.0.0; };
    $field 0;
};
EOF
    $CHECKCONF badzero.conf > /dev/null 2>&1
    [ $? -eq 1 ] || { echo "I: zone $field failed" ; ret=1; }
done
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking options allowed in inline-signing slaves ($n)"
ret=0
l=`$CHECKCONF bad-dnssec.conf 2>&1 | grep "dnssec-dnskey-kskonly.*requires inline" | wc -l`
[ $l -eq 1 ] || ret=1
l=`$CHECKCONF bad-dnssec.conf 2>&1 | grep "dnssec-loadkeys-interval.*requires inline" | wc -l`
[ $l -eq 1 ] || ret=1
l=`$CHECKCONF bad-dnssec.conf 2>&1 | grep "update-check-ksk.*requires inline" | wc -l`
[ $l -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check file + inline-signing for slave zones ($n)"
l=`$CHECKCONF inline-no.conf 2>&1 | grep "missing 'file' entry" | wc -l`
[ $l -eq 0 ] || ret=1
l=`$CHECKCONF inline-good.conf 2>&1 | grep "missing 'file' entry" | wc -l`
[ $l -eq 0 ] || ret=1
l=`$CHECKCONF inline-bad.conf 2>&1 | grep "missing 'file' entry" | wc -l`
[ $l -eq 1 ] || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking named-checkconf DLZ warnings ($n)"
ret=0
$CHECKCONF dlz-bad.conf 2>&1 | grep "'dlz' and 'database'" > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking for missing key directory warning ($n)"
ret=0
rm -rf test.keydir
l=`$CHECKCONF warn-keydir.conf 2>&1 | grep "'test.keydir' does not exist" | wc -l`
[ $l -eq 1 ] || ret=1
touch test.keydir
l=`$CHECKCONF warn-keydir.conf 2>&1 | grep "'test.keydir' is not a directory" | wc -l`
[ $l -eq 1 ] || ret=1
rm -f test.keydir
mkdir test.keydir
l=`$CHECKCONF warn-keydir.conf 2>&1 | grep "key-directory" | wc -l`
[ $l -eq 0 ] || ret=1
rm -rf test.keydir
if [ $ret != 0 ]; then echo "I:failed"; fi

n=`expr $n + 1`
echo "I: checking that named-checkconf -z catches conflicting ttl with max-ttl ($n)"
ret=0
$CHECKCONF -z max-ttl.conf > check.out 2>&1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
grep 'TTL 900 exceeds configured max-zone-ttl 600' check.out > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z catches invalid max-ttl ($n)"
ret=0
$CHECKCONF -z max-ttl-bad.conf > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z skips zone check with alternate databases ($n)"
ret=0
$CHECKCONF -z altdb.conf > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z skips zone check with DLZ ($n)"
ret=0
$CHECKCONF -z altdlz.conf > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z fails on view with ANY class ($n)"
ret=0
$CHECKCONF -z view-class-any1.conf > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z fails on view with CLASS255 class ($n)"
ret=0
$CHECKCONF -z view-class-any2.conf > /dev/null 2>&1 && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z passes on view with IN class ($n)"
ret=0
$CHECKCONF -z view-class-in1.conf > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: checking that named-checkconf -z passes on view with CLASS1 class ($n)"
ret=0
$CHECKCONF -z view-class-in2.conf > /dev/null 2>&1 || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that check-names fails as configured ($n)"
ret=0
$CHECKCONF -z check-names-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "near '_underscore': bad name (check-names)" checkconf.out$n > /dev/null || ret=1
grep "zone check-names/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that check-mx fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "near '10.0.0.1': MX is an address" checkconf.out$n > /dev/null || ret=1
grep "zone check-mx/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that check-dup-records fails as configured ($n)"
ret=0
$CHECKCONF -z check-dup-records-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "has semantically identical records" checkconf.out$n > /dev/null || ret=1
grep "zone check-dup-records/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that check-mx fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "failed: MX is an address" checkconf.out$n > /dev/null || ret=1
grep "zone check-mx/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that check-mx-cname fails as configured ($n)"
ret=0
$CHECKCONF -z check-mx-cname-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "MX.* is a CNAME (illegal)" checkconf.out$n > /dev/null || ret=1
grep "zone check-mx-cname/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that check-srv-cname fails as configured ($n)"
ret=0
$CHECKCONF -z check-srv-cname-fail.conf > checkconf.out$n 2>&1 && ret=1
grep "SRV.* is a CNAME (illegal)" checkconf.out$n > /dev/null || ret=1
grep "zone check-mx-cname/IN: loaded serial" < checkconf.out$n > /dev/null && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that named-checkconf -p properly print a port range ($n)"
ret=0
$CHECKCONF -p portrange-good.conf > checkconf.out$n 2>&1 || ret=1
grep "range 8610 8614;" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that named-checkconf -z handles in-view ($n)"
ret=0
$CHECKCONF -z in-view-good.conf > checkconf.out7 2>&1 || ret=1
grep "zone shared.example/IN: loaded serial" < checkconf.out7 > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that 'dnssec-lookaside auto;' generates a warning ($n)"
ret=0
$CHECKCONF warn-dlv-auto.conf > checkconf.out$n 2>/dev/null || ret=1
grep "dnssec-lookaside 'auto' is no longer supported" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that 'dnssec-lookaside . trust-anchor dlv.isc.org;' generates a warning ($n)"
ret=0
$CHECKCONF warn-dlv-dlv.isc.org.conf > checkconf.out$n 2>/dev/null || ret=1
grep "dlv.isc.org has been shut down" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that 'dnssec-lookaside . trust-anchor dlv.example.com;' doesn't generates a warning ($n)"
ret=0
$CHECKCONF good-dlv-dlv.example.com.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

n=`expr $n + 1`
echo "I: check that the 2010 ICANN ROOT KSK without the 2017 ICANN ROOT KSK generates a warning ($n)"
ret=0
$CHECKCONF check-root-ksk-2010.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] || ret=1
grep "trusted-key for root from 2010 without updated" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

echo "I: check that the 2010 ICANN ROOT KSK with the 2017 ICANN ROOT KSK does not warning ($n)"
ret=0
$CHECKCONF check-root-ksk-both.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

echo "I: check that the 2017 ICANN ROOT KSK alone does not warning ($n)"
ret=0
$CHECKCONF check-root-ksk-2017.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] && ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

echo "I: check that the dlv.isc.org KSK generates a warning ($n)"
ret=0
$CHECKCONF check-dlv-ksk-key.conf > checkconf.out$n 2>/dev/null || ret=1
[ -s checkconf.out$n ] || ret=1
grep "trusted-key for dlv.isc.org still present" checkconf.out$n > /dev/null || ret=1
if [ $ret != 0 ]; then echo "I:failed"; ret=1; fi
status=`expr $status + $ret`

echo "I:exit status: $status"
[ $status -eq 0 ] || exit 1
