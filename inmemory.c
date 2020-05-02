#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
  char page_in_m = 0;
  void* addr = malloc(40);
  addr = (void *) ((int) addr & (~0xFFF));
  printf("The address of addr is %p\n", addr);
  if (mincore(addr, 4096, &page_in_m) < 0) {
    printf("error to get the memory usage information\n");
    return 1;
  }
  if (page_in_m & 0x01)
    printf("Yes! in memory\n");
  else
    printf("No!, not in memory\n");
}
