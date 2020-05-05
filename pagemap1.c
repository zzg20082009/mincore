/* 这个程序先查看进程的maps文件(/proc/PID/maps)，获取进程的逻辑地址空间的映射，然后打开/proc/PID/pagemap文件，
   由逻辑地址去获得该逻辑页面是否分配的有物理块，如果有物理块，则打印出物理块的编号。通过查看/proc/PID/smaps文件，本程序正确*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>
#include <string.h>

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF
void parse(long, long);
int virt_addr_range(char*);

int fd;       // file descriptor of pagemap

int main(int argc, char* argv[])
{
  if (argc != 3) {
    printf("Usage: ./pagemap /proc/PID/maps /proc/PID/pagemap\n");
    return 1;
  }

  if ((fd = open(argv[2], O_RDONLY)) < -1) {
    printf("Open the pagemap file error\n");
    return 2;
  }

  virt_addr_range(argv[1]);
  return 0;
}

int virt_addr_range(char* mapfile)
{
  FILE* fp;

  if ((fp = fopen(mapfile, "r")) == NULL) {
    perror("Open file: ");
    return 1;
  }

  char* line_in_map = malloc(512);
  size_t n;
  char* p1;        // p1 is used to locate the pointer of '-'
  char* p2;        // p2 is used to locate the pointer of the first ' ' 
  
  char begin_virt[17];
  char end_virt[17];
  bzero(begin_virt, 17);
  bzero(end_virt, 17);
  
  while (1) {
    if (getline(&line_in_map, &n, fp) == -1) {
      printf("End of the read\n");
      return 2;
    }
    p1 = strchr(line_in_map, '-');
    strncpy(begin_virt, line_in_map, p1 - line_in_map);
    p1++;          // p1 point beyond '-'
    p2 = strchr(line_in_map, ' ');
    strncpy(end_virt, p1, p2 - p1 + 1);
    printf("The start address is: %s, The end of address is: %s\n", begin_virt, end_virt);
    long start = strtol(begin_virt, NULL, 16);
    long end = strtol(end_virt, NULL, 16);
    parse(start, end);
    printf("--------------------------------------------------------------------------\n");
  }
}
    
void parse(long start, long end)
{
  long startpage = start >> 12;
  long endpage = end >> 12;
  long page_entry;
  lseek(fd, startpage * PAGEMAP_ENTRY, SEEK_SET);    // 定位文件的读写指针到虚拟地址开始处
  for (long pagen = startpage; pagen < endpage; pagen++) {
    read(fd, &page_entry, PAGEMAP_ENTRY);
    if (page_entry & 0x8000000000000000) {
      printf("The virtual address %x is in the memory, and the page frame number is: ", pagen);
      printf("%ld\n", GET_PFN(page_entry));
    } else {
      printf("This virtual address %x is not in the memory :(\n", pagen);
    }
  }
}
