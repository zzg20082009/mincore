#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  char* page_in_m;
  int pages;
  struct stat statbuf;
  void* mappedaddress;
  int fd;

  if (argc != 2) {
    printf("Usage: inmemory filetomap");
    return 1;
  }

  if (stat(argv[1], &statbuf) < 0) {
    printf("Error to get the meta data of file\n");
    return 2;
  }
  pages = statbuf.st_size / sysconf(_SC_PAGESIZE) + 1;
  page_in_m = (char* ) malloc(pages);

  if ((fd = open(argv[1], O_RDWR)) < 0) {
    printf("Error to open the file\n");
    return 3;
  }

  mappedaddress = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (mappedaddress == MAP_FAILED) {
    printf("mmap error \n");
    return 4;
  }
  mappedaddress = (void* ) ((unsigned int) mappedaddress & ~0xFFF);
  if (mincore(mappedaddress, statbuf.st_size, page_in_m) < 0) {
    printf("error while execute mincore\n");
    return 5;
  }

  for (int i = 0; i < pages; i++) {
    if (page_in_m[i] & 0x01)
      printf("The %d th page is in memory\n", i);
  }
}
  
