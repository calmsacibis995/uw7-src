#ident	"@(#)pdi.cmds:fixroot.h	1.1"

typedef struct io_template {
    char *dir;
    char *device;
    char *node;
    char *global;
    char *linkdir;
    char *linknode;
} io_template_t;

io_template_t  specials[] = {
"/.io","bootdisk","root","rootdev","/dev","root",
"/.io","bootdisk","swap","swapdev","/dev","swap",
"/.io","bootdisk","dump","dumpdev","/dev","dump",
"/.io","bootdisk","stand","standdev","/dev","stand",
};
