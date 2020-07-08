/* PHYVMS.c v1.3    Physical I/O module for Unix */

/*
        This is part of ODS2 written by Paul Nankervis,
        email address:  Paulnank@au1.ibm.com

        ODS2 is distributed freely for all members of the
        VMS community to use. However all derived works
        must maintain comments in their source to acknowledge
        the contibution of the original author.
*/

/*
	If the user mounts  cd0   we open up /dev/cd0 for access.
*/

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "phyio.h"
#include "ssdef.h"

#if defined(__digital__) && defined(__unix__)
#define DEV_PREFIX "/devices/rdisk/%s"
#else
#ifdef sun
#define DEV_PREFIX "/dev/dsk/%s"
#else
#define DEV_PREFIX "/dev/%s"
#endif
#endif

unsigned init_count = 0;
unsigned read_count = 0;
unsigned write_count = 0;
unsigned SCacheEna = 1;

#define SCACHE_BLKS 256
#define SCACHE_BSZ 512
struct {
    unsigned sect;
    unsigned age;
    char *buff;
} SCache[SCACHE_BLKS][4];
unsigned SCacheTime = 0;

void scache_init(void)
{
    for(int i=0;i<SCACHE_BLKS;i++) {
	for(int j=0;j<4;j++) {
	    SCache[i][j].sect = 0xFFFFFFFF;
	    SCache[i][j].age = 0;
	    SCache[i][j].buff = NULL;
	}
    }
}


unsigned scache_read(unsigned block,unsigned length,char *buffer)
{
    unsigned tblk;
    unsigned tlen;
    unsigned seg;
    unsigned flag;

    SCacheTime++;
    for(tblk = block, tlen = length; tlen > 0; tblk++, tlen-=SCACHE_BSZ) {
	seg = block & (SCACHE_BLKS-1);
	flag = 0;
	for(int i=0;i<4;i++) {
	    if (SCache[seg][i].sect == block) {
		SCache[seg][i].age = SCacheTime;
		memcpy(buffer, SCache[seg][i].buff, SCACHE_BSZ);
		buffer += SCACHE_BSZ;
		flag = 1;
		break;
	    }
	}
	if (flag == 0) {
	    return 0;	// Block not found
	}
    }
    return 1;	// All data in cache
}


void scache_write(unsigned block,unsigned length,char *buffer)
{
    unsigned tblk;
    unsigned tlen;
    unsigned seg;
    unsigned flag;
    unsigned minage;
    unsigned mindex;

    if (SCacheEna == 0) return;
    for(tblk = block, tlen = length; tlen > 0; tblk++, tlen-=SCACHE_BSZ) {
	seg = block & (SCACHE_BLKS-1);
	// Find in cache or oldest slot
	for(int i=0;i<4;i++) {
	    if (SCache[seg][i].sect == block) {
		minage = SCache[seg][i].age;
		mindex = i;
		break;		// Found, overwrite this slot
	    }
	    if ((i==0) || (SCache[seg][i].age < minage)) {
		minage = SCache[seg][i].age;
		mindex = i;	// Else overwrite the oldest slot
	    }
	}
	// Alloc buffer if needed
	if ( SCache[seg][mindex].buff == NULL ) {
	    SCache[seg][mindex].buff = malloc(SCACHE_BSZ);
	}
	if ( SCache[seg][mindex].buff == NULL ) {
            perror("No memory for sector cache");
	    return;
	}
	// Update slot
	SCache[seg][mindex].sect == block;
	SCache[seg][mindex].age = SCacheTime;
	memcpy(SCache[seg][mindex].buff, buffer, SCACHE_BSZ);
	buffer += SCACHE_BSZ;
    }
}


void phyio_show(void)
{
    printf("PHYIO_SHOW Initializations: %d Reads: %d Writes: %d\n",
           init_count,read_count,write_count);
}


unsigned phyio_init(int devlen,char *devnam,unsigned *handle,struct phyio_info *info)
{
    int vmsfd;
    char *cp,*devbuf;

    scache_init();
    init_count++;
    devbuf = (char *)malloc(devlen + strlen(DEV_PREFIX) + 5);
    if (devbuf == NULL) return SS$_NOSUCHDEV;
    info->status = 0;           /* We don't know anything about this device! */
    info->sectors = 0;
    info->sectorsize = 0;
    sprintf(devbuf,"%s",devnam);
    cp = strchr(devbuf,':');
    if (cp != NULL) *cp = '\0';

    /* try to open file without '/dev/' prefix */
    vmsfd = open(devbuf,O_RDWR);
    if (vmsfd < 0) vmsfd = open(devbuf,O_RDONLY);

    if (vmsfd < 0)
    {
	/* try to open file with '/dev/' prefix */
        sprintf(devbuf,DEV_PREFIX,devnam);
        cp = strchr(devbuf,':');
        if (cp != NULL) *cp = '\0';
        vmsfd = open(devbuf,O_RDWR);
        if (vmsfd < 0) vmsfd = open(devbuf,O_RDONLY);
    }
    else
    {
	/* remove '.' from filename to prevent parsing problems */
	while ((cp = strchr(devnam,'.')) != NULL)
	    *cp = '$';
    }
    free(devbuf);
    if (vmsfd < 0) return SS$_NOSUCHDEV;
    *handle = vmsfd;
    return SS$_NORMAL;
}


unsigned phyio_close(unsigned handle)
{
    scache_init();
    close(handle);
    return SS$_NORMAL;
}


unsigned phyio_read(unsigned handle,unsigned block,unsigned length,char *buffer)
{
    int res;
#ifdef DEBUG
    printf("Phyio read block: %d into %p (%d bytes)\n",block,buffer,length);
#endif
    read_count++;
    if (scache_read(block,length,buffer)) {
	return SS$_NORMAL;
    }
    if ((res = lseek(handle,block*512,0)) < 0) {
        perror("lseek ");
	printf("lseek failed %d\n",res);
        return SS$_PARITY;
    }
    if ((res = read(handle,buffer,length)) != length) {
        perror("read ");
	printf("read failed %d\n",res);
        return SS$_PARITY;
    }
    scache_write(block,length,buffer);
    return SS$_NORMAL;
}


unsigned phyio_write(unsigned handle,unsigned block,unsigned length,char *buffer)
{
#ifdef DEBUG
    printf("Phyio write block: %d from %x (%d bytes)\n",block,buffer,length);
#endif
    write_count++;
    if (lseek(handle,block*512,0) < 0) return SS$_PARITY;
    if (write(handle,buffer,length) != length) return SS$_PARITY;
    return SS$_NORMAL;
}
