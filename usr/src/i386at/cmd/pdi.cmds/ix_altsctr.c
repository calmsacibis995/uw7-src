/*	copyright	"%c%"	*/

#ident	"@(#)pdi.cmds:ix_altsctr.c	1.8.1.1"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */


#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <stdio.h>
#include <string.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/altsctr.h>
#include <sys/param.h>
#include <sys/fdisk.h>
#include "badsec.h"


#include <errno.h>
#include <pfmt.h>

#define FAILURE	0
#define	SUCCESS	1

struct	badsec_lst *badsl_chain;
int	badsl_chain_cnt;

int	unix_srtsec = -1;

extern struct  	disk_parms      dp;     
extern struct	pdinfo		pdinfo;
extern struct	vtoc_ioctl		vtoc;

int	*alts_fd = NULL;
struct 	absio	alts_absio;		/* buf used for RDABS & WRABS ioctls */

struct	alts_mempart alts_part = { 0, NULL, 0 };
struct	alts_mempart *ap = &alts_part;	/* pointer to incore alts tables     */
struct  alt_info alttbl;		/* alternate sector and track tables */


/*
 * updatebadsec () -- update bad sector/track mapping tables
 */
updatebadsec()
{
	int	i;
	struct  partition *part;

	for (i=vtoc.v_nslices - 1; i >= 0; i--) {
	    part = &vtoc.v_slices[i];
/*
 *	    if the alternate sector/track partition exists
 *	    then use the combined sector/track mapping scheme
 */
	    if (part->p_tag == V_ALTSCTR) {
		get_badsec();
		read_altsctr(part);
		ent_sort(ap->ap_gbadp, ap->ap_gbadcnt);
		ent_compress(ap->ap_gbadp, ap->ap_gbadcnt);
		gen_alts_ent();
		compress_map();
		return(SUCCESS);
	    }	
	}
	return(FAILURE);
}

/*
 * read_altsctr( ptr to alternate sector partition )
 *		-- read the alternate sector partition tables
 */
read_altsctr(part)
struct 	partition *part;
{
	int	status;

	if (ap->ap_tblp == NULL) {
/*	    allocate buffer for the alts partition table (sector size)	*/
	    ap->ap_tbl_secsiz = byte_to_secsiz(ALTS_PARTTBL_SIZE,&dp);
	    ap->ap_tblp = (struct alts_parttbl *)malloc(ap->ap_tbl_secsiz);
	    if (ap->ap_tblp == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":134:Unable to malloc alternate partition table.\n");
		exit(69);
	    }

/*	    allocate buffer for the alts partition map (sector size)	*/
/*	    buffers include the disk image bit map 			*/
/*	    and the incore transformed char map				*/

	    if ((ap->ap_memmapp = (unchar *)malloc(part->p_size)) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":135:Unable to malloc incore alternate partition map.\n");
		exit(69);
	    }
	    ap->ap_tblp->alts_map_len = (part->p_size + 8 - 1) / 8;
	    ap->ap_map_secsiz=byte_to_secsiz(ap->ap_tblp->alts_map_len,&dp);
	    ap->ap_map_sectot = ap->ap_map_secsiz / (uint) dp.dp_secsiz;
	    if ((ap->ap_mapp = (unchar *)malloc(ap->ap_map_secsiz)) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":136:Unable to malloc alternate partition map.\n");
		exit(69);
	    }
/*	    clear the buffers to zero					*/
	    memset(ap->ap_memmapp,0,part->p_size);
	    memset(ap->ap_mapp,0,ap->ap_map_secsiz);
	    ap->part = *part;		/* struct copy			*/
	
/*
 *	    if add alternate partition flag is set, then install the partition
 *	    otherwise read the alts partition info from disk
 *	    if failed, then assume the first installation 
 */
	    if (ap->ap_flag & ALTS_ADDPART) 
		init_altsctr();
	    else {
	    	if ((status = get_altsctr()) == SUCCESS)
		    chk_badsec();	
	    	else
		    init_altsctr();
	    }
	}

}


/*
 *	checking duplicate bad sectors or bad sectors in ALTSCTR partition
 */
chk_badsec()
{
	daddr_t	badsec;
	daddr_t	altsp_srtsec = ap->part.p_start;
	daddr_t	altsp_endsec = ap->part.p_start + ap->part.p_size - 1;
	int	cnt;
	int	status;

	for (cnt=0; cnt < ap->ap_gbadcnt; cnt++) {
	    badsec = (ap->ap_gbadp)[cnt].bad_start;
/*	    if bad sector is within the ATLSCTR partition		*/
	    if ((badsec >= altsp_srtsec) && (badsec <= altsp_endsec)) {
		if ((ap->ap_memmapp)[badsec - altsp_srtsec] != ALTS_BAD) {
		    if ((badsec >= altsp_srtsec) && (badsec <= (altsp_srtsec +
			ap->ap_tbl_secsiz / (uint)dp.dp_secsiz - 1))) {
		    	(void) pfmt(stderr, MM_ERROR,
				":137:Alternate partition information table is bad.\n");
		    	exit(90);
	    	    }
		    if ((badsec >= altsp_srtsec+ap->ap_tblp->alts_map_base) && 
			(badsec <= (altsp_srtsec + ap->ap_tblp->alts_map_base +
			ap->ap_map_sectot - 1))) {
		    	(void) pfmt(stderr, MM_ERROR,
				":138:Alternate partition map is bad.\n");
		    	exit(90);
	    	    }
		    if ((badsec >= altsp_srtsec+ap->ap_tblp->alts_ent_base) && 
			(badsec <= (altsp_srtsec + ap->ap_tblp->alts_ent_base +
			ap->ap_ent_secsiz / (uint)dp.dp_secsiz - 1))) {
		    	(void) pfmt(stderr, MM_ERROR,
				":139:Alternate partition entry table is bad.\n");
		    	exit(90);
	    	    }
		    (ap->ap_memmapp)[badsec - altsp_srtsec] = ALTS_BAD;
	            (ap->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
		} else {
		    status = chk_bad_altsctr(badsec);
	            (ap->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
		}
	    } else {
/*
 *		binary search for bad sector in the alts entry table
 */
		status = ent_bsearch(ap->ap_entp, ap->ap_tblp->alts_ent_used,
					&((ap->ap_gbadp)[cnt]) );
/*
 *		if the bad sector had already been remapped(found in alts_entry)
 *		then ignore the bad sector
 */
		if (status != -1) {
	            (ap->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
		}
	    }
	}
	



}

/*
 *	initialize the alternate partition tables
 */
init_altsctr()
{
	daddr_t	badsec;
	daddr_t	altsp_srtsec = ap->part.p_start;
	daddr_t	altsp_endsec = ap->part.p_start + ap->part.p_size - 1;
	int	cnt;

	ap->ap_entp = NULL;
	ap->ap_ent_secsiz = 0;
	ap->ap_tblp->alts_sanity = ALTS_SANITY;
	ap->ap_tblp->alts_version= ALTS_VERSION1;
	ap->ap_tblp->alts_map_len = (ap->part.p_size + 8 - 1) / 8;
	ap->ap_tblp->alts_ent_used = 0;
	ap->ap_tblp->alts_ent_base = 0;
	ap->ap_tblp->alts_ent_end  = 0;
	ap->ap_tblp->alts_resv_base = ap->part.p_size - 1;
	for (cnt=0; cnt<5; cnt++)
	    ap->ap_tblp->alts_pad[cnt]=0;

	for (cnt=0; cnt < ap->ap_gbadcnt; cnt++) {
	    badsec = (ap->ap_gbadp)[cnt].bad_start;
	    if ((badsec >= altsp_srtsec) && (badsec <= altsp_endsec)) {
		if (badsec == altsp_srtsec) {
		    (void) pfmt(stderr, MM_ERROR,
				":140:First sector of alternate partition is bad.\n");
		    exit(90);
	    	}
		(ap->ap_memmapp)[badsec - altsp_srtsec] = ALTS_BAD;
	        (ap->ap_gbadp)[cnt].bad_start = ALTS_ENT_EMPTY;
	    }
	}
	
/*	allocate the alts_map on disk skipping possible bad sectors	*/
	ap->ap_tblp->alts_map_base = 
		altsmap_alloc(ap->ap_tbl_secsiz / (uint)dp.dp_secsiz, 
			ap->part.p_size, ap->ap_map_sectot, ALTS_MAP_UP);
	if (ap->ap_tblp->alts_map_base == NULL) {
	    (void) pfmt(stderr, MM_ERROR,
			":141:Unable to allocate alternate map on disk: %s",
			strerror(errno));
	    exit(90);
	}

}


/*
 * 	read the alternate partition tables from disk
 */
get_altsctr()
{
	int	mystatus = FAILURE;
	int	status;

/*	get alts partition table info					*/
	if ((status = absdsk_io(*alts_fd, ap->part.p_start, (char *)ap->ap_tblp, 
			ap->ap_tbl_secsiz,V_RDABS))==FAILURE) {
	    (void) pfmt(stderr, MM_ERROR,
			":142:Unable to read alternate sector partition: %s",
			strerror(errno));
	    exit(90);
	}
	if (ap->ap_tblp->alts_sanity != ALTS_SANITY)
	    return(mystatus);

/*	get the alts map						*/
	if ((status = absdsk_io(*alts_fd, 
		(ap->part.p_start+ap->ap_tblp->alts_map_base),
		(char *)ap->ap_mapp, ap->ap_map_secsiz, V_RDABS)) == FAILURE) {
	    (void) pfmt(stderr, MM_ERROR,
			":143:Unable to read alternate sector partition map: %s",
			strerror(errno));
	    exit(90);
	}

/*	transform the disk image bit-map to incore char map		*/
	expand_map();

	if (ap->ap_tblp->alts_ent_used == 0) {
	    ap->ap_entp = NULL;
	    ap->ap_ent_secsiz = 0;
	} else {
	    ap->ap_ent_secsiz = byte_to_secsiz(
			(ap->ap_tblp->alts_ent_used*ALTS_ENT_SIZE),&dp);
	    if ((ap->ap_entp = (struct alts_ent *)malloc(ap->ap_ent_secsiz)) == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":144:Unable to malloc alternate sector entry table.\n");
		exit(69);
	    }

	    if ((status = absdsk_io(*alts_fd,
				(ap->part.p_start+ap->ap_tblp->alts_ent_base),
	    			(char *)ap->ap_entp, ap->ap_ent_secsiz, 
				V_RDABS)) ==FAILURE){
		(void) pfmt(stderr, MM_ERROR,
			":145:Unable to read alternate sector entry table: %s",
			strerror(errno));
		exit(90);
	    }
	}

	return(mystatus=SUCCESS);

}


/*
 *	update the new alternate partition tables on disk
 */
wr_altsctr()
{
	int	mystatus = FAILURE;
	int	status;

	if (ap->ap_tblp == NULL)
		return;
	if ((status = absdsk_io(*alts_fd,ap->part.p_start,
	    ap->ap_tblp, ap->ap_tbl_secsiz, V_WRABS)) == FAILURE) {
	    (void) pfmt(stderr, MM_ERROR,
			":146:Unable to write alternate sector partition: ",
			strerror(errno));
	    exit(90);
	}

	if ((status = absdsk_io(*alts_fd,
			(ap->part.p_start+ap->ap_tblp->alts_map_base),
	    		ap->ap_mapp, ap->ap_map_secsiz, V_WRABS)) == FAILURE){
	    (void) pfmt(stderr, MM_ERROR,
			":147:Unable to write alternate sector partition map: ",
			strerror(errno));
	    exit(90);
	}

	if (ap->ap_tblp->alts_ent_used != 0) {
	    if ((status = absdsk_io(*alts_fd,
				(ap->part.p_start+ap->ap_tblp->alts_ent_base),
	    			(char *)ap->ap_entp, ap->ap_ent_secsiz,
				V_WRABS)) == FAILURE) {
		(void) pfmt(stderr, MM_ERROR,
			":148:Unable to write alternate sector entry table: %s",
			strerror(errno));
		exit(90);
	    }
	}



}


/*
 *	get a list of bad sector
 */
get_badsec()
{
	int	cnt;
	struct	badsec_lst *blc_p;
	daddr_t	curbad;
	long	maxsec = (long)dp.dp_heads *dp.dp_cyls *dp.dp_sectors;
	struct	alts_ent *growbadp;
	int	i;

	cnt = count_badsec();
	if (!cnt) {
	    ap->ap_gbadp = NULL;
	    ap->ap_gbadcnt = 0;
	}
	else {
	    ap->ap_gbadp = (struct alts_ent *) malloc(cnt*ALTS_ENT_SIZE);	
	    memset(ap->ap_gbadp,0,cnt*ALTS_ENT_SIZE);

	    for (growbadp=ap->ap_gbadp, cnt=0, blc_p=badsl_chain;
		blc_p; blc_p=blc_p->bl_nxt) {
		for (i=0; i<blc_p->bl_cnt; i++) {
		    curbad = blc_p->bl_sec[i];
		    if (curbad < (daddr_t)dp.dp_sectors) {
		    	(void) pfmt(stderr, MM_ERROR,
			   ":149:Ignoring bad sector %ld which is in first track of the drive.\n", curbad);
		    	continue;
		    }
		    if (curbad >= maxsec) {
		    	(void) pfmt(stderr, MM_ERROR,
			   ":150:Ignoring bad sector %ld which is past the end of the drive.\n", curbad);
		    	continue;
		    }
		    growbadp[cnt].bad_start = curbad;
		    growbadp[cnt].bad_end = curbad;
		    cnt++;
		}
	    }
	}
	ap->ap_gbadcnt = cnt;
}

/*
 *	count number of bad sector on list
 *	merging the bad sector list from surface analysis and the
 *	one given through the command line
 */
count_badsec()
{

	struct badsec_lst *blc_p;

	if (!badsl_chain) 
		badsl_chain_cnt = 0;
	else {
		for (blc_p = badsl_chain; blc_p->bl_nxt; blc_p=blc_p->bl_nxt)
			;
		blc_p->bl_nxt = NULL;
	}

	return(badsl_chain_cnt);
}


/*
 *	generate alternate entry table by merging the existing and
 *	the new entry list.
 */
gen_alts_ent() {
	int	status;
	int	ent_used;
	struct	alts_ent *entp;

	if (ap->ap_gbadcnt == 0)
	    return;

	ent_used = ap->ap_tblp->alts_ent_used + ap->ap_gbadcnt;
	ap->ap_ent_secsiz = byte_to_secsiz(ent_used*ALTS_ENT_SIZE,&dp);
	entp=(struct alts_ent *) malloc (ap->ap_ent_secsiz);
	ent_used = ent_merge(entp, ap->ap_entp, ap->ap_tblp->alts_ent_used,
			     ap->ap_gbadp, ap->ap_gbadcnt);
	if (ap->ap_entp) 
	    free(ap->ap_entp);
	if (ap->ap_gbadp) 
	    free(ap->ap_gbadp);

	/****
	** If we are left with no sectors to remap, let's return. This would
	** be a case where all the sector's we found exist in the Alternate
	** Sector slice.
	****/
	if( ent_used == 0 ) {
		return;
	}

	ap->ap_entp = entp;
	ap->ap_ent_secsiz = byte_to_secsiz(ent_used*ALTS_ENT_SIZE,&dp);
	ap->ap_tblp->alts_ent_used = ent_used;
	ap->ap_gbadp = NULL;
	ap->ap_gbadcnt = 0;

/*	assign alternate sectors to the bad sectors			*/
	assign_altsctr();

/*	allocate the alts_entry on disk skipping possible bad sectors	*/
	ap->ap_tblp->alts_ent_base = 
		altsmap_alloc(ap->ap_tblp->alts_map_base + ap->ap_map_sectot,
			ap->part.p_size, 
			ap->ap_ent_secsiz / (uint)dp.dp_secsiz, ALTS_MAP_UP);
	if (ap->ap_tblp->alts_ent_base == NULL) {
	    (void) pfmt(stderr, MM_ERROR,
			":151:Unable to allocate alternate entry table on disk: %s", strerror(errno));
	    exit(90);
	}

	ap->ap_tblp->alts_ent_end = ap->ap_tblp->alts_ent_base + 
			(ap->ap_ent_secsiz / (uint)dp.dp_secsiz) - 1; 
}


/*
 *	assign alternate sectors for bad sector mapping
 */
assign_altsctr()
{
	int	i;
	int	j;
	daddr_t	alts_ind;
	int	cluster;

	for (i=0; i<ap->ap_tblp->alts_ent_used; i++) {
	    if ((ap->ap_entp)[i].bad_start == ALTS_ENT_EMPTY)
		continue;
	    if ((ap->ap_entp)[i].good_start != 0)
		continue;
	    cluster = (ap->ap_entp)[i].bad_end-(ap->ap_entp)[i].bad_start +1; 
	    alts_ind = 
		altsmap_alloc(ap->part.p_size-1, ap->ap_tblp->alts_map_base + 
			ap->ap_map_sectot - 1, cluster, ALTS_MAP_DOWN);
	    if (alts_ind == NULL) {
	    	(void) pfmt(stderr, MM_ERROR,
			":152:Unable to allocate alternates for bad starting sector %ld.\n",
			(ap->ap_entp)[i].bad_start);
	    	exit(63);
	    }
	    alts_ind = alts_ind - cluster + 1;
	    (ap->ap_entp)[i].good_start =alts_ind +ap->part.p_start; 
	    for (j=0; j<cluster; j++) {
		(ap->ap_memmapp)[alts_ind+j] = ALTS_BAD;		
	    }

	}

}

/*
 *	transform the disk image alts bit map to incore char map
 */
expand_map()
{
	int 	i;

	for (i=0; i<ap->part.p_size; i++) {
	    (ap->ap_memmapp)[i] = altsmap_getbit(i);
	}
}

/*
 *	transform the incore alts char map to the disk image bit map
 */
compress_map()
{

	int 	i;
	int	bytesz;
	char	mask = 0;
	int	maplen=0;

	for (i=0, bytesz=7; i<ap->part.p_size; i++) {
	    mask |= ((ap->ap_memmapp)[i] << bytesz--);
	    if (bytesz < 0) {
		(ap->ap_mapp)[maplen++] = mask;
		bytesz = 7;
		mask = 0;
	    }
	}
/*
 *	if partition size != multiple number of bytes	
 *	then record the last partial byte			
 */
	if (bytesz != 7)
	    (ap->ap_mapp)[maplen] = mask;
	    
}

/*
 *	given a bad sector number, search in the alts bit map
 *	and identify the sector as good or bad
 */
altsmap_getbit(badsec)
daddr_t	badsec;
{
	int	slot = badsec / 8;
	int	field = badsec % 8;	
	unchar	mask;

	mask = ALTS_BAD<<7; 
	mask >>= field;
	if ((ap->ap_mapp)[slot] & mask)
	     return(ALTS_BAD);
	return(ALTS_GOOD);
}


/*
 *	allocate a range of sectors from the alternate partition
 */
altsmap_alloc(srt_ind, end_ind, cnt, dir)
daddr_t	srt_ind;
daddr_t	end_ind;
int	cnt;
int	dir;
{
	int	i;
	int	total;
	int	first_ind;

	for (i=srt_ind, first_ind=srt_ind, total=0; i!=end_ind; i+=dir) {
	    if ((ap->ap_memmapp)[i] == ALTS_BAD) {
		total = 0;
		first_ind = i + dir;
		continue;
	    }
	    total++;
	    if (total == cnt)
		return(first_ind);

	}
	return(NULL);
}



/*
 * 	bubble sort the entry table into ascending order
 */
ent_sort(buf, cnt)
struct	alts_ent buf[];
int	cnt;
{
struct	alts_ent temp;
int	flag;
int	i,j;

	for (i=0; i<cnt-1; i++) {
	    temp = buf[cnt-1];
	    flag = 1;
	    
	    for (j=cnt-1; j>i; j--) {
		if (buf[j-1].bad_start < temp.bad_start) {
		    buf[j] = temp;
		    temp = buf[j-1];
		} else {
		    buf[j] = buf[j-1];
		    flag = 0;
		}
	    }
	    buf[i] = temp;
	    if (flag) break;
	}

}


/*
 *	compress all the contiguous bad sectors into a single entry 
 *	in the entry table. The entry table must be sorted into ascending
 *	before the compression.
 */
ent_compress(buf, cnt)
struct	alts_ent buf[];
int	cnt;
{
int	keyp;
int	movp;
int	i;

	for (i=0; i<cnt; i++) {
	    if (buf[i].bad_start == ALTS_ENT_EMPTY)
		continue;
	    for (keyp=i, movp=i+1; movp<cnt; movp++) {
	    	if (buf[movp].bad_start == ALTS_ENT_EMPTY)
			continue;
		if (buf[keyp].bad_end+1 != buf[movp].bad_start)
		    break;
		buf[keyp].bad_end++;
		buf[movp].bad_start = ALTS_ENT_EMPTY;
	    }
	    if (movp == cnt) break;
	}
}


/*
 *	merging two entry tables into a single table. In addition,
 *	all empty slots in the entry table will be removed.
 */
ent_merge(buf, list1, lcnt1, list2, lcnt2)
struct	alts_ent buf[];
struct	alts_ent list1[];
int	lcnt1;
struct	alts_ent list2[];
int	lcnt2;
{
	int	i;
	int	j1,j2;

	for (i=0, j1=0, j2=0; j1<lcnt1 && j2<lcnt2;) {
	    if (list1[j1].bad_start == ALTS_ENT_EMPTY) {
		j1++;
		continue;	
	    }
	    if (list2[j2].bad_start == ALTS_ENT_EMPTY) {
		j2++;
		continue;
	    }
	    if (list1[j1].bad_start < list2[j2].bad_start)
		buf[i++] = list1[j1++];
	    else 
		buf[i++] = list2[j2++];
	}
	for (; j1<lcnt1; j1++) {
	    if (list1[j1].bad_start == ALTS_ENT_EMPTY) 
		continue;	
	    buf[i++] = list1[j1];
	}
	for (; j2<lcnt2; j2++) {
	    if (list2[j2].bad_start == ALTS_ENT_EMPTY) 
		continue;	
	    buf[i++] = list2[j2];
	}
	return (i);
}


/*
 *	binary search for bad sector in the alternate entry table
 */
ent_bsearch(buf, cnt, key)
struct	alts_ent buf[];
int	cnt;
struct	alts_ent *key;
{
	int	i;
	int	ind;
	int	interval;
	int	mystatus = -1;

	if (!cnt) return (mystatus);

	for (i=1; i<=cnt; i<<=1)
	    ind=i;

	for (interval=ind; interval; ) {
/*
	    printf("ind= %d, intv= %d; ",ind, interval);
*/
	    if ((key->bad_start >= buf[ind-1].bad_start) && 
		(key->bad_start <= buf[ind-1].bad_end)) {
		return(mystatus=ind-1);
	    } else {
	    	interval >>= 1;
		if (!interval) break;
		if (key->bad_start < buf[ind-1].bad_start) {
		    ind = ind - interval;
		} else {
 /* 		    if key is larger than the last element then break	*/
		    if (ind == cnt) break;	
		    if ((ind+interval) <= cnt)
		    	ind += interval;
		}
	    }
	}
	return(mystatus);
}

/*
 *	check for bad sector in assigned alternate sectors
 */
chk_bad_altsctr(badsec)
daddr_t	badsec;
{
	int	i;
	int	j;
	daddr_t	numsec;
	int	mystatus = FAILURE;
	int	cnt = ap->ap_tblp->alts_ent_used;
	daddr_t intv[3];

	for (i=0; i<cnt; i++) {
	    numsec = (ap->ap_entp)[i].bad_end - (ap->ap_entp)[i].bad_start;
	    if ((badsec >= (ap->ap_entp)[i].good_start) &&
		(badsec <= ((ap->ap_entp)[i].good_start + numsec))) {
		(void) pfmt(stderr, MM_ERROR,
			":153:Bad sector %ld is an assigned alternate sector.\n", badsec);
		exit(90);
/*
		if (!numsec) {
		    (ap->ap_entp)[i].good_start = 0;
		    return(mystatus);
		}
		intv[0] = badsec - (ap->ap_entp)[i].good_start;
		intv[1] = 1;
		intv[2] = (ap->ap_entp)[i].good_start + numsec - badsec;
*/
	    } 
	}
/*	the bad sector has already been identified as bad		*/
	return(mystatus=SUCCESS);

}


/*
 * print_altsec () -- print alternate sector information
 */
print_altsec()
{
	int	i;
	struct  partition *part;

	for (i=vtoc.v_nslices - 1; i >= 0; i--) {
	    part = &vtoc.v_slices[i];
/*
 *	    if the alternate sector/track partition exists
 *	    then use the combined sector/track mapping scheme
 */
	    if (part->p_tag == V_ALTSCTR) {
		ap->ap_tblp = NULL;
		ap->ap_flag &= ~ALTS_ADDPART;
		read_altsctr(part);
		print_altsctr();
		return(SUCCESS);
	    }	
	}
	return(FAILURE);
}

print_altsctr()
{
	int	i;
	int	totalloc;
	int	avail;

/*	find # of available alternate sectors				*/
	for (i=0, totalloc=0; i<ap->part.p_size; i++) {
	    if ((ap->ap_memmapp)[i])
		totalloc++;
	}
/*
 *	available = size of partition - allocated sectors/bad sectors
 *		    - partition table - partition map
 *		    - entry table
 */
	avail = ap->part.p_size - totalloc;
	avail = avail - (ap->ap_tbl_secsiz/(uint)dp.dp_secsiz) 
		- ap->ap_map_sectot;
	avail = avail-(ap->ap_tblp->alts_ent_end -ap->ap_tblp->alts_ent_base+1);
	if (avail < 0) avail = 0;

	(void) pfmt(stdout, MM_NOSTD,
		":154:\nALTERNATE SECTOR/TRACK MAPPING TABLE:\n");
	(void) pfmt(stdout, MM_NOSTD,
		":155:\nBad Sector Start\tAlternate Sector Start\t\tCount\n");

	for (i=0; i<ap->ap_tblp->alts_ent_used; i++) {
	     (void) pfmt(stdout, MM_NOSTD,
			":156:\t%ld\t     ->\t\t%ld\t\t\t   %ld\n",
		(ap->ap_entp)[i].bad_start,
		(ap->ap_entp)[i].good_start,
		((ap->ap_entp)[i].bad_end - (ap->ap_entp)[i].bad_start + 1));
	}
	(void) pfmt(stdout, MM_NOSTD,
		":157:\n      %ld alternate sector(s) left for allocation.\n", avail);

}

absdsk_io(fd, srtsec, bufp, len, ioflag)
int	fd;
uint	srtsec;
char	*bufp;
uint	len;
int	ioflag;
{
	int	cnt;
	int	i;

	cnt = len / (uint)dp.dp_secsiz;

	for (i=0; i<cnt; i++) {
		alts_absio.abs_sec = srtsec + i;
		alts_absio.abs_buf = bufp + (i * dp.dp_secsiz);
		if (ioctl(fd, ioflag, &alts_absio) != 0) 
			return(FAILURE);
	}
	return(SUCCESS);
}

updatealts()
{
	read_alts();
	get_badsec();
	ent_sort(ap->ap_gbadp, ap->ap_gbadcnt);
	assign_alts();
}

read_alts()
{
	int	status;
	int	alts_sec;

	if (unix_srtsec == -1)
		get_unix_srtsec();
	alts_sec = unix_srtsec + (pdinfo.alt_ptr/dp.dp_secsiz);
	if ((status =absdsk_io(*alts_fd, alts_sec, (char *)&alttbl,
			sizeof(alttbl),V_RDABS))==FAILURE) {
	    (void) pfmt(stderr, MM_ERROR,
			":158:Unable to read alternate table: %s",
			strerror(errno));
	    exit(90);
	}

	if ((alttbl.alt_sanity != ALT_SANITY) || 
	   (alttbl.alt_version != ALT_VERSION)) {
		(void) pfmt(stderr, MM_WARNING,
			":159:invalid alternates table found.\n");
		exit(8);
	}
}



assign_alts()
{
	struct	alts_ent *growbadp;
	daddr_t curbad;
	int	cnt;
	int	i;

	if (ap->ap_gbadcnt == 0)
		return;

	if (alttbl.alt_sec.alt_reserved == 0) {
		(void) pfmt(stderr, MM_WARNING,
			":160:No alternates partition in VTOC \n");
		(void) pfmt(stderr, MM_NOSTD,
			":161:Bad sectors will not be marked!\n");
		return;
	}

	for (growbadp=ap->ap_gbadp, cnt=0; cnt<ap->ap_gbadcnt; cnt++) {
		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

/* 		if unused alternate is bad, then excise it from the list. */
		for (i = alttbl.alt_sec.alt_used; i < (int)alttbl.alt_sec.alt_reserved; i++) {
			if (alttbl.alt_sec.alt_base + i == curbad) {
				alttbl.alt_sec.alt_bad[i] = -1;
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				if (i == alttbl.alt_sec.alt_used) {
					while (++alttbl.alt_sec.alt_used < alttbl.alt_sec.alt_reserved)
						if (alttbl.alt_sec.alt_bad[alttbl.alt_sec.alt_used] != -1)
							break;
				}
				break;
			}
		}
/* 		check for used alternate is bad				*/
		for (i=0; i < (int)alttbl.alt_sec.alt_used; i++) {
			if (alttbl.alt_sec.alt_base + i == curbad) {
				(void) pfmt(stderr, MM_ERROR,
					":162:Bad sector %ld is an assigned alternate!\n",
				    curbad);
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				break;
			}
/* 			check if bad block already mapped (already in list) */
			if (alttbl.alt_sec.alt_bad[i] == curbad) {
				growbadp[cnt].bad_start = ALTS_ENT_EMPTY;
				break;
			}
		}
	}

	for (growbadp=ap->ap_gbadp, cnt=0; cnt<ap->ap_gbadcnt; cnt++) {
		curbad = growbadp[cnt].bad_start;
		if (curbad == ALTS_ENT_EMPTY)
			continue;

		if (alttbl.alt_sec.alt_used >= alttbl.alt_sec.alt_reserved) {
			(void) pfmt(stderr, MM_ERROR,
				":163:Insufficient alternates available for bad sector %ld!\n",
				    curbad);
			break;
		}
		alttbl.alt_sec.alt_bad[alttbl.alt_sec.alt_used] = curbad;
		while (++alttbl.alt_sec.alt_used <alttbl.alt_sec.alt_reserved){
			if (alttbl.alt_sec.alt_bad[alttbl.alt_sec.alt_used]!=-1)
				break;
		}
	}

}


wr_alts()
{
	int	status;
	int	alts_sec;

	if (unix_srtsec == -1)
		get_unix_srtsec();

	alts_sec = unix_srtsec + (pdinfo.alt_ptr/dp.dp_secsiz);
	if ((status =absdsk_io(*alts_fd, alts_sec, (char *)&alttbl,
			sizeof(alttbl),V_WRABS))==FAILURE) {
	    (void) pfmt(stderr, MM_ERROR,
			":164:Unable to write alternate table: %s",
			strerror(errno));
	    exit(90);
	}
}



get_unix_srtsec()
{
	int i;
	struct mboot	mboot;
	struct ipart	*fdp, *unix_part;

	alts_absio.abs_sec = 0;
	alts_absio.abs_buf = (char *)&mboot;
	if (ioctl(*alts_fd, V_RDABS, &alts_absio) < 0) {
		(void) pfmt(stderr, MM_ERROR,
			":165:Unable to read fdisk table: %s", strerror(errno));
		exit(70);
	}

	/* find an active UNIX System partition */
	unix_part = NULL;
	fdp = (struct ipart *)mboot.parts;
	for (i = FD_NUMPART; i-- > 0; ++fdp) {
		if ((fdp->systid == UNIXOS) && (fdp->bootid == ACTIVE))
				unix_part = fdp;
	}
	if (unix_part == NULL) {
		(void) pfmt(stderr, MM_ERROR,
			":25:No active UNIX System partition in partition table!\n");
		exit(71);
	}
	unix_srtsec = unix_part->relsect;
}

