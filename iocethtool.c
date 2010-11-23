/*
 * File: iocethtool
 * Implements: generic ethtool ioctl access
 *
 * Copyright: Jens Låås, UU 2010
 * Copyright license: According to GPL, see file COPYING in this directory.
 *
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <net/if.h>

#include <linux/sockios.h>  
#include <sys/socket.h>     
#include <arpa/inet.h>      

#include "jelist.h"
#include "jelopt.h"

#ifndef SIOCETHTOOL         
#define SIOCETHTOOL     0x8946
#endif                

struct {
	int list, verbose;
} conf;

enum {
	CHUNK_NOP, CHUNK_STRING, CHUNK_SIGNED, CHUNK_BITS, CHUNK_DECIMAL
};

struct chunk {
	int bytes;
	int flags; /* CHUNK_STRING, CHUNK_SIGNED, CHUNK_BITS */
	uint8_t value[32];
};

struct ethtool_generic {
	uint32_t cmd;
	uint8_t data[1024*2];
};

const char *names[]= { "ETHTOOL_GSET",
		       "ETHTOOL_SSET",
		       "ETHTOOL_GDRVINFO",
		       "ETHTOOL_GREGS",
		       "ETHTOOL_GWOL",
		       "ETHTOOL_SWOL",
		       "ETHTOOL_GMSGLVL",
		       "ETHTOOL_SMSGLVL",
		       "ETHTOOL_NWAY_RST",
		       "ETHTOOL_GLINK",
		       "ETHTOOL_GEEPROM",
		       "ETHTOOL_SEEPROM",
		       "ETHTOOL_NOTUSED",
		       "ETHTOOL_GCOALESCE",
		       "ETHTOOL_SCOALESCE",
		       "ETHTOOL_GRINGPARAM",
		       "ETHTOOL_SRINGPARAM",
		       "ETHTOOL_GPAUSEPARAM",
		       "ETHTOOL_SPAUSEPARAM",
		       "ETHTOOL_GRXCSUM",
		       "ETHTOOL_SRXCSUM",
		       "ETHTOOL_GTXCSUM",
		       "ETHTOOL_STXCSUM",
		       "ETHTOOL_GSG",
		       "ETHTOOL_SSG",
		       "ETHTOOL_TEST",
		       "ETHTOOL_GSTRINGS",
		       "ETHTOOL_PHYS_ID",
		       "ETHTOOL_GSTATS",
		       "ETHTOOL_GTSO",
		       "ETHTOOL_STSO",
		       "ETHTOOL_GPERMADDR",
		       "ETHTOOL_GUFO",
		       "ETHTOOL_SUFO",
		       "ETHTOOL_GGSO",
		       "ETHTOOL_SGSO",
		       "ETHTOOL_GFLAGS",
		       "ETHTOOL_SFLAGS",
		       "ETHTOOL_GPFLAGS",
		       "ETHTOOL_SPFLAGS",
		       "ETHTOOL_GRXFH",
		       "ETHTOOL_SRXFH",
		       "ETHTOOL_GGRO",
		       "ETHTOOL_SGRO",
		       "ETHTOOL_GRXRINGS",
		       "ETHTOOL_GRXCLSRLCNT",
		       "ETHTOOL_GRXCLSRULE",
		       "ETHTOOL_GRXCLSRLALL",
		       "ETHTOOL_SRXCLSRLDEL",
		       "ETHTOOL_SRXCLSRLINS",
		       "ETHTOOL_FLASHDEV",
		       "ETHTOOL_RESET",
		       "ETHTOOL_SRXNTUPLE",
		       "ETHTOOL_GRXNTUPLE",
		       "ETHTOOL_GSSET_INFO",
		       "ETHTOOL_GRXFHINDIR",
		       "ETHTOOL_SRXFHINDIR",
		       NULL };


static int iocethtool(const char *devname, int cmd, struct jlhead *chunks)
{
        struct ifreq ifr;    
        int fd, i, j;
	int err;
	uint8_t *p;
	struct ethtool_generic arg;
	struct chunk *c;
	
        /* Setup our control structures. */
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, devname);
                             
        /* Open control socket. */
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (fd < 0) {        
                perror("Cannot get control socket");
                return 70;   
        }                    
	arg.cmd = cmd;

	/* fill arg.data according to format */
	p = &arg.data[0];
	jl_foreach(chunks, c) {
		memcpy(p, c->value, c->bytes);
		p += c->bytes;
	}
	
        ifr.ifr_data = (caddr_t)&arg;
        err = ioctl(fd, SIOCETHTOOL, &ifr);
        if (err < 0) {       
                perror("Cannot get driver information");
                return 71;   
        }

	/* display arg.data according to format */
	p = &arg.data[0];
	jl_foreach(chunks, c) {
		if(c->flags & CHUNK_STRING) {
			int val;
			for(i=0;i<c->bytes;i++) {
				val = *p++;
				if(val) printf("%c", val);
			}
			printf("\n");
			continue;
		}
		if(c->flags & CHUNK_BITS) {
			for(i=0;i<c->bytes;i++) {
				for(j=0;j<8;j++)
					if((*p) & (1<<j) )
						printf("1");
					else
						printf("0");
				p++;				
			}
			printf("\n");
			continue;
		}
		if(c->flags & CHUNK_DECIMAL) {
			if(c->bytes == 8) {
				printf("%llu\n", *((uint64_t*)p));
				p+=8;
				continue;
			}
			if(c->bytes == 4) {
				printf("%u\n", *((uint32_t*)p));
				p+=4;
				continue;
			}
			if(c->bytes == 2) {
				printf("%u\n", *((uint16_t*)p));
				p+=2;
				continue;
			}
			for(i=0;i<c->bytes;i++) {
				printf("%02x", *p++);
			}
			printf("\n");
			continue;
		}
		if(c->bytes == 8) {
			printf("%016llx\n", *((uint64_t*)p));
			p+=8;
			continue;
		}
		if(c->bytes == 4) {
			printf("%08x\n", *((uint32_t*)p));
			p+=4;
			continue;
		}
		if(c->bytes == 2) {
			printf("%04x\n", *((uint16_t*)p));
			p+=2;
			continue;
		}
		for(i=0;i<c->bytes;i++) {
			printf("%02x", *p++);
		}
		printf("\n");
	}
	
	return 0;
}

int add(struct jlhead *fmtlist, char *fmt)
{
	struct chunk *c;
	char *v;
	uint32_t ival, *ivalp;
	uint64_t lival, *livalp;
	uint16_t *i16p;

	c = malloc(sizeof(struct chunk));
	c->flags = 0;
	if(*fmt == 's') {
		c->flags |= CHUNK_STRING;
		fmt++;
	}
	if(*fmt == 'b') {
		c->flags |= CHUNK_BITS;
		fmt++;
	}
	if(*fmt == 'd') {
		c->flags |= CHUNK_DECIMAL;
		fmt++;
	}
	c->bytes = atoi(fmt);
	v = strchr(fmt, ':');
	if(!v) {
		printf("Value missing!\n");
		return -1;
	}
	v++;
	jl_append(fmtlist, c);

	if(c->flags & CHUNK_STRING) {
		memcpy(c->value, v, strlen(v));
		return 0;
	}
	
	if(c->bytes == 1) {
		c->value[0] = strtol(v, NULL, 0);
	}
	if(c->bytes == 2) {
		ival = strtol(v, NULL, 0);
		i16p = (uint16_t*)&c->value[0];
		*i16p = ival;
	}
	if(c->bytes == 4) {
		ival = strtol(v, NULL, 0);
		ivalp = (uint32_t*)&c->value[0];
		*ivalp = ival;
	}
	if(c->bytes == 8) {
		lival = strtoll(v, NULL, 0);
		livalp = (uint64_t*)&c->value[0];
		*livalp = lival;
	}

	return 0;
}

int main(int argc, char **argv)
{
  int err, rc;
  struct chunk *c;
  int i;
  char *devname = NULL;
  int cmd;
  struct jlhead *fmtlist = jl_new();

  conf.verbose = 0;

  if(jelopt(argv, 'h', "help",NULL, &err))
    {
    usage:
      printf("iocethtool [-vh] ETHDEV CMD [FORMAT]*\n"
	     " Version 0.1 101122\n"
	     " -h --help\n"
	     " --list     List known ethtool ioctl names.\n"
	     " -v --verbose\n"
	     " CMD = number or name of ethtool ioctl.\n"
	     "       Ex. 0x13, ETHTOOL_GPAUSEPARAM\n"
	     " FORMAT = [s|b|d]N:[V]\n"
	     " N = number of bytes\n"
	     " V = value\n"
	     " s = string, b = bitfield, d = decimal\n"
	     "\n"
	     "Examples:\n"
	     " Reading ETHTOOL_GPAUSEPARAM\n"
	     " $ iocethtool eth0 0x12 4: 4: 4:\n"
	     " Setting ETHTOOL_SPAUSEPARAM\n"
	     " $ iocethtool eth0 ETHTOOL_SPAUSEPARAM 4:1 4:1 4:1\n"
	     " Reading ETHTOOL_GDRVINFO\n"
	     " $ iocethtool eth0 0x3 s32: s32:\n"
	     );
      exit(0);
    }

  if(jelopt(argv, 'v', "verbose", NULL, &err))
                conf.verbose = 1;

  if(jelopt(argv, 0, "list", NULL, &err)) {
	  for(i=0;;i++) {
		  if(!names[i]) break;
		  printf("%s\n", names[i]);
	  }	  
	  exit(1);
  }

  argc = jelopt_final(argv, &err);

  if(argc < 3) {
	  goto usage;
  }

  devname = argv[1];

  cmd = -1;
  for(i=0;;i++) {
	  if(!names[i]) break;
	  if(!strcmp(argv[2], names[i])) {
		  cmd = i+1;
		  break;
	  }
  }

  if(cmd == -1)
	  cmd = strtol(argv[2], NULL, 0);
  if(cmd == 0) {
	  printf("cmd must be greater than 0!\n");
	  exit(1);
  }

  if(conf.verbose)
	  printf("CMD=%d\n", cmd);
  for(i=3;i<argc;i++) {
	    if(conf.verbose)
		    printf("arg %d: %s\n", i, argv[i]);
	    add(fmtlist, argv[i]);
  }

  if(conf.verbose)
	  jl_foreach(fmtlist, c) {
		  printf("bytes: %d\n", c->bytes);
	  }

  rc = iocethtool(devname, cmd, fmtlist);
  exit(rc);
}



/*
ETHTOOL_GSET		0x00000001
ETHTOOL_SSET		0x00000002
ETHTOOL_GDRVINFO	0x00000003
ETHTOOL_GREGS		0x00000004
ETHTOOL_GWOL		0x00000005
ETHTOOL_SWOL		0x00000006
ETHTOOL_GMSGLVL		0x00000007
ETHTOOL_SMSGLVL		0x00000008
ETHTOOL_NWAY_RST	0x00000009
ETHTOOL_GLINK		0x0000000a
ETHTOOL_GEEPROM		0x0000000b
ETHTOOL_SEEPROM		0x0000000c
ETHTOOL_GCOALESCE	0x0000000e
ETHTOOL_SCOALESCE	0x0000000f
ETHTOOL_GRINGPARAM	0x00000010
ETHTOOL_SRINGPARAM	0x00000011
ETHTOOL_GPAUSEPARAM	0x00000012
ETHTOOL_SPAUSEPARAM	0x00000013
ETHTOOL_GRXCSUM		0x00000014
ETHTOOL_SRXCSUM		0x00000015
ETHTOOL_GTXCSUM		0x00000016
ETHTOOL_STXCSUM		0x00000017
ETHTOOL_GSG		0x00000018
ETHTOOL_SSG		0x00000019
ETHTOOL_TEST		0x0000001a
ETHTOOL_GSTRINGS	0x0000001b
ETHTOOL_PHYS_ID		0x0000001c
ETHTOOL_GSTATS		0x0000001d
ETHTOOL_GTSO		0x0000001e
ETHTOOL_STSO		0x0000001f
ETHTOOL_GPERMADDR	0x00000020
ETHTOOL_GUFO		0x00000021
ETHTOOL_SUFO		0x00000022
ETHTOOL_GGSO		0x00000023
ETHTOOL_SGSO		0x00000024
ETHTOOL_GFLAGS		0x00000025
ETHTOOL_SFLAGS		0x00000026
ETHTOOL_GPFLAGS		0x00000027
ETHTOOL_SPFLAGS		0x00000028
ETHTOOL_GRXFH		0x00000029
ETHTOOL_SRXFH		0x0000002a
ETHTOOL_GGRO		0x0000002b
ETHTOOL_SGRO		0x0000002c
ETHTOOL_GRXRINGS	0x0000002d
ETHTOOL_GRXCLSRLCNT	0x0000002e
ETHTOOL_GRXCLSRULE	0x0000002f
ETHTOOL_GRXCLSRLALL	0x00000030
ETHTOOL_SRXCLSRLDEL	0x00000031
ETHTOOL_SRXCLSRLINS	0x00000032
ETHTOOL_FLASHDEV	0x00000033
ETHTOOL_RESET		0x00000034
ETHTOOL_SRXNTUPLE	0x00000035
ETHTOOL_GRXNTUPLE	0x00000036
ETHTOOL_GSSET_INFO	0x00000037
ETHTOOL_GRXFHINDIR	0x00000038
ETHTOOL_SRXFHINDIR	0x00000039
*/
