/*
 *	@(#)conflib.c	7.1	10/22/97	12:21:46
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#ifdef SVR42
#include <string.h>
#else
#include <strings.h>
#endif
#include <fcntl.h>
#include <sys/wait.h>
#include "confdata.h"

menu *menus;
node *nodes;
int  nmenus, nnodes;

char *descr[MAXDESCR];

static filehdr fh;

static void
dump_resource(int indent, resource *res)
{
  int i;

  for (i=0;i<indent;i++)
	fprintf(stderr, "\t");

	fprintf(stderr, "\t\trestype=%d, num=%d",res->type, res->num);
	fprintf(stderr, ", default=%x", res->default_value);
	fprintf(stderr, ", mask=%x", res->choices.mask);
	fprintf(stderr, "\n");

  if (res->descr != -1)
  {
	fprintf(stderr, "Descr(%d): '%s'\n", res->descr, descr[res->descr]);
  }
}

static void
dump_node(int indent, node *nd)
{
  int i;

    for (i=0;i<indent;i++)
  	fprintf(stderr, "\t");
  
    fprintf(stderr, "\t%s: num=%d, bus=%d, type=%d, opt=%x, nres=%d",
  	 nd->name, nd->number, nd->bustype, nd->type, nd->options, nd->nres);
    if (nd->dl.type)
  	fprintf(stderr, ", dnld=%s,%d,%s", nd->dl.driver, nd->dl.type, nd->dl.filename);
    fprintf(stderr, "\n");

  if (nd->descr != -1)
  {
	fprintf(stderr, "Descr(%d): '%s'\n", nd->descr, descr[nd->descr]);
  }

  for (i=0;i<nd->nref;i++)
	dump_node(indent+1, &nodes[nd->ref[i]]);
  for (i=0;i<nd->nres;i++)
	dump_resource(indent, &nd->res[i]);
}

void
dump_confdata(void)
{
	int i, n;

	for (i=0;i<fh.nmenu;i++)
	{
		n = menus[i].nodenum;
		fprintf(stderr, "Menu %s/%s = '%s'\n", nodes[n].name, 
						       menus[i].key,
						       menus[i].name);

		dump_node(0, &nodes[n]);
	}
}

static void
uncompress(char *name, int writefd)
{
/*
 * This routine reads and unscrambles a file and feeds it to uncompress.
 */
	int fd, i, n, l, sts;
	int pid;
	int pipes[2];
	int buf[512];
	unsigned int seed;

	struct stat st;

	if (pipe(pipes)==-1)
	{
		perror("pipe2");
		exit(-1);
	}

	if ((pid=fork())==-1)
	{
		perror("fork2");
		exit(-1);
	}

	if (pid == 0)	/* Child */
	{
	   close(0);
	   dup(pipes[0]);
	   close(1);
	   dup(writefd);
	   close(pipes[1]);
	   if (execlp("uncompress", "uncompress", NULL)==-1)
	   {
		fprintf(stderr, "Can't execute 'uncompress'.\n");
		fprintf(stderr, "Ensure the PATH variable is set properly\n");
		exit(-1);
	   }
	   exit(-1);
	}

	close(pipes[0]);
	close(writefd);
	writefd = pipes[1];

	if ((fd=open(name, O_RDONLY, 0))==-1)
	{
		perror(name);
		exit(-1);
	}

	if (fstat(fd, &st)==-1)
	{
		perror("fstat");
		exit(-1);
	}

	seed = st.st_size;

	while ((l=read(fd, buf, sizeof(buf)))>0)
	{
		
		if (write(writefd, buf, l)!=l)
		{
			perror("write");
			exit(-1);
		}
	}

	if (l==-1)
	{
		perror("read");
		exit(-1);
	}

	close(fd);
	close(writefd);

	if (wait(&sts) == -1 || WEXITSTATUS(sts)) /* Wait for "uncompress" to finish */
        {
		fprintf(stderr, "Failed to uncompress config.dat\n");
		exit(-1);
        }
	exit(0);	/* This is a child process so we have to exit */
}

void
load_confdata(char *name)
{
	int i, l, pid, n, sts;
	int pipes[2];
	int fd=0;

	int dptr[MAXDESCR];
	char *descrdata;

	char *nodep, *menup;

	if (pipe(pipes)==-1)
	{
		perror("pipe");
		exit(-1);
	}

	if ((pid=fork())==-1)
	{
		perror("fork");
		exit(-1);
	}

	if (pid != 0) 	/* Father */
	{
	   fd = pipes[0];
	   close(pipes[1]);
	}
	else
	   uncompress(name, pipes[1]);

	if (read(fd, (char*)&fh, sizeof(filehdr))!=sizeof(filehdr))
	{
		perror("config.dat (header)");
		exit(-1);
	}

	if (strcmp(fh.id, "USSCONF") != 0)
	{
		fprintf(stderr, "%s: Invalid file header\n", name);
		exit(-1);
	}

	if (fh.endian != 0x12345678)
	{
		fprintf(stderr, "%s: Invalid file format.\n", name);
		exit(-1);
	}

	if (fh.version != HDR_VERSION)
	{
		fprintf(stderr, "%s: Invalid file version.\n", name);
		exit(-1);
	}

	l = fh.nmenu * sizeof(menu);
	nmenus = fh.nmenu;

	menus = (menu*)malloc(l);
	if (menus==NULL)
	{
		fprintf(stderr, "malloc(menu) failed\n");
		exit(-1);
	}

	menup = (char *)menus;
	
	while (l)
	{
	   if ((n=read(fd, menup, l))<=0)
	   {
		perror("config.dat (menus)");
		exit(-1);
	   }

	   l -= n;
	   menup += n;
	}

	l = fh.nnode * sizeof(node);
	nodes = (node*)malloc(l);
	nnodes = fh.nnode;

	if (nodes==NULL)
	{
		fprintf(stderr, "malloc(node) failed\n");
		exit(-1);
	}

	nodep=(char*)nodes;

	while (l)
	{
	   if ((n=read(fd, nodep, l))<=0)
	   {
		perror("config.dat (nodes)");
		exit(-1);
	   }

	   l -= n;
	   nodep += n;
	}

	nodep = (char*)dptr;
	l = fh.ndescr * 4;

	while (l)
	{
	   if ((n=read(fd, nodep, l))<=0)
	   {
		perror("config.dat (descrtab)");
		exit(-1);
	   }

	   l -= n;
	   nodep += n;
	}

	l = fh.stringsz;

	if ((descrdata=malloc(l))==NULL)
	{
		fprintf(stderr, "malloc(descr) failed\n");
		exit(-1);
	}
	
	nodep = descrdata;

	while (l)
	{
	   if ((n=read(fd, nodep, l))<=0)
	   {
		perror("config.dat (strings)");
		exit(-1);
	   }

	   l -= n;
	   nodep += n;
	}

	for (i=0;i<fh.ndescr;i++)
	{
	    descr[i] = descrdata + dptr[i];
	}

	if (wait(&sts) == -1 || WEXITSTATUS(sts)) /* Wait for "uncompress" to finish */
        {
		fprintf(stderr, "Failed to load config.dat\n");
		exit(-1);
        }
	close(fd);
}
