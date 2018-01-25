// RUN: %clangxx -O0 -g %s -o %t && %run %t

#include <fcntl.h>
#include <kvm.h>

int main(void) {
  kvm_t K;

  K = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, "kvm_open");
  kvm_close(&K);

  return 0;
}
