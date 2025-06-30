#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <byteswap.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>

#include <sys/types.h>
#include <sys/mman.h>

#include "arm_pcie_protocol.h"
#include "element_process.h"
#include "link_margin.h"

/* ltoh: little to host */
/* htol: little to host */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x) (x)
#define ltohs(x) (x)
#define htoll(x) (x)
#define htols(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x) __bswap_32(x)
#define ltohs(x) __bswap_16(x)
#define htoll(x) __bswap_32(x)
#define htols(x) __bswap_16(x)
#endif

#define FATAL                                                                                                 \
	do                                                                                                        \
	{                                                                                                         \
		fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno)); \
		exit(1);                                                                                              \
	} while (0)

#define MAP_SIZE (1024UL * 1024UL)
#define MAP_MASK (MAP_SIZE - 1)

void *map_base;
int fd;
/// @brief
/// @param file
/// @return
uint32_t open_xdma_dev(const char *file)
{
	// int fd;
	if ((fd = open(file, O_RDWR | O_SYNC)) == -1)
	{
		FATAL;
		return -1;
	}
	else
	{
		//printf("character device %s opened.\n", file);
		fflush(stdout);
	}

	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map_base == (void *)-1)
	{
		FATAL;
		return -1;
	}
	else
	{
		//printf("Memory mapped at address %p.\n", map_base);
		fflush(stdout);
	}
	return 0;
}
/// @brief
/// @param addr
/// @return
uint32_t xdma_reg_read(uint32_t addr)
{
	uint32_t Data;
	Data = *((uint32_t *)(map_base + addr));
	Data = ltohl(Data);

	/* printf("Read 32-bit value at address 0x%08x (%p): 0x%08x\n",
		   (unsigned int)addr, map_base,
		   (unsigned int)Data); */
	return Data;
}
/// @brief
/// @param addr
/// @param value
/// @return
uint32_t xdma_reg_write(uint32_t addr, uint32_t value)
{
	// uint32_t Data;
	value = htoll(value);
	*((uint32_t *)(map_base + addr)) = (uint32_t)value;

	/* 	printf("Write 32-bits value 0x%08x to 0x%08x (0x%p)\n",
			   (unsigned int)value, (unsigned int)addr,
			   map_base); */
	return 0;
}

void rw_test(int argc, char **argv)
{
	uint32_t addr, value;
	if (4 == argc)
	{
		addr = strtoul(argv[3], 0, 0);
		xdma_reg_read(addr);
	}
	else if (5 == argc)
	{
		addr = strtoul(argv[3], 0, 0);
		value = strtoul(argv[4], 0, 0);
		xdma_reg_write(addr, value);
	}
}

/// @brief
/// @return
uint32_t close_xdma_dev()
{
	if (munmap(map_base, MAP_SIZE) == -1)
	{
		FATAL;
		return -1;
	}
	close(fd);
	return 0;
}

char input[256];
int main(int argc, char **argv)
{
	uint32_t result;
	uint32_t result1;
	uint32_t channel, index;

	if (argc < 4)
	{
		fprintf(stderr,
				"\nUsage:\t%s <device> <channel>\n"
				"\tdevice  : character device(xdma_user) to access\n"
				"\tchannel : serdes channel to find\n\n",
				argv[0]);
		exit(1);
	}

	//printf("device: %s\n", argv[1]);
	channel = strtoul(argv[2], 0, 0);
	printf("%s(%d) channel: %d\n", __func__, __LINE__, channel);
	index = strtoul(argv[3], 0, 0);

	result = open_xdma_dev(argv[1]);
	if (0 != result)
	{
		printf("open %s failed, error code(%d)\n", argv[1], result);
		exit(1);
	}

	// cfg serdes .......
	// send_cmd_to_a53_sync((const char *)argv[3]);
	//
	if (100 == channel)
	{
		// printf("cmd lenght:%ld\n", strlen(argv[3]));
		// memset(input, 0, 256);
		// memcpy(input, argv[3] + 1, strlen(argv[3]) - 2);
		// printf("cmd:%s\n", input);
		// printf("cmd:%s\n", argv[3]);
		result = send_cmd_to_a53_sync(argv[3]);
		if(result !=0)
		{
			printf("a53 error\n");
		}
		result = get_cmd_result();
		printf("result: 0x%x\n",result);
	}
	else if(101 == channel)
	{
		result = send_cmd_to_a53_sync(argv[3]);
		if(result !=0)
			printf("a53 error\n");	
		result = get_cmd_result();
		printf("fps: %.2f\n", (float)300*1000000 / (float)result);
	}
	else if(102 == channel)
	{
		result = send_cmd_to_a53_sync(argv[3]);
		if(result !=0)
			printf("a53 error\n");	
		result = get_cmd_result();
		printf("%d\n", result);
		sleep(2);
		result1 = send_cmd_to_a53_sync(argv[3]);
		if(result1 !=0)
			printf("a53 error\n");
		result1 = get_cmd_result();
		printf("%d\n", result1);
	}
	else if(channel > 100)
	{
		printf("%s(%d) enter\n", __func__, __LINE__);
		link_margin_test(map_base, channel-100, index);
	}
	else
	{
		printf("%s(%d) enter\n", __func__, __LINE__);
		serdes_channel_init(map_base, channel, index);
	}


	result = close_xdma_dev();
	return 0;
}
