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
long phn_count(long phn_num);

int fd;       // file descriptor of pagemap
int kpagefd;  // file descriptor of kpagecount

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

  if ((kpagefd = open("/proc/kpagecount", O_RDONLY)) < -1) {
    printf("Open the /proc/kpagecount file error\n");
    return 3;
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
  
  long uss = 0;
  long pss = 0;  
  lseek(fd, startpage * PAGEMAP_ENTRY, SEEK_SET);    // 定位文件的读写指针到虚拟地址开始处
  for (long pagen = startpage; pagen < endpage; pagen++) {
    read(fd, &page_entry, PAGEMAP_ENTRY);
    if (page_entry & 0x8000000000000000) {
      printf("The virtual address %x is in the memory, and the page frame number is: ", pagen);
      long phn_num = GET_PFN(page_entry);
      printf("%ld\n", phn_num);
      long ref_cnts = 0;
      if ((ref_cnts = phn_count(phn_num)) == -1) {
	printf("Get information error\n");
	continue;
      }
      if (ref_cnts == 1)
	uss++;
      else
	pss+= ref_cnts;
      printf("The phn_num: %ld is referenced %ld times\n", phn_num, ref_cnts);
    } else {
      printf("This virtual address %x is not in the memory :(\n", pagen);
    }
  }
  printf("In this virtual address range, the uss is %ld K, and the pss is **K\n", uss * 4);
}

long phn_count(long ph_num)   // 根据物理号的编号，计算出该物理块被映射了几次，由此可以计算出USS， PSS
{
  long offset = ph_num * 8;
  long page_count = 0;
  if (lseek(kpagefd, offset, SEEK_SET) < 0) {
    printf("ph_num %d maybe is not a valid number\n", ph_num);
    return -1;
  }
  if (read(kpagefd, &page_count, 8) < 0) {
    return -1;
  }
  return page_count;
}

// 这个程序的结构非常的烂，最多只能用作教学用
