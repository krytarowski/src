#!/bin/sh

cp /usr/pkg/bin/mg /public/netbsd-llvm/destdir.amd64/bin/mg
mount -t null /dev /public/netbsd-llvm/destdir.amd64/dev
mount -t null /tmp /public/netbsd-llvm/destdir.amd64/tmp
