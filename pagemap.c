#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
void parse(long, long);

int fd;       // file descriptor of pagemap

int main(int argc, char* argv[])
{
  if (argc != 4) {
    printf("Usage: ./pagemap pagemap start-virt-addr end-virt-addr\n");
    return 1;
  }

  if ((fd = open(argv[1], O_RDONLY)) < -1) {
    printf("Open the pagemap file error\n");
    return 2;
  }

  long start = strtol(argv[2], NULL, 16);
  long end = strtol(argv[3], NULL, 16);
  //  printf("The address is %ld--%ld\n", start, end);
  parse(start, end);
  return 0;
}

void parse(long start, long end)
{
  long startpage = start >> 12;
  long endpage = end >> 12;
  long page_entry;
  lseek(fd, startpage * PAGEMAP_ENTRY, SEEK_SET);    // 定位文件的读写指针到虚拟地址开始处
  for (long pagen = startpage; pagen <= endpage; pagen++) {
    read(fd, &page_entry, PAGEMAP_ENTRY);
    if (page_entry & 0x8000000000000000) {
      printf("The virtual address is in the memory, and the page frame number is: ");
      printf("%ld\n", GET_PFN(page_entry));
    }
  }
}
