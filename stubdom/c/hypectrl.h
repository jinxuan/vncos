#include <mini-os/sched.h>
#include <mini-os/lib.h>
#include <mini-os/xs.h>
#include <mini-os/posix/err.h>
#include <mini-os/xenbus.h>

#define STRING_MAX XENSTORE_ABS_PATH_MAX+1024
static int show_whole_path=0;
static int desired_width = 60;
static int max_width=80;
#define TAG " = \"...\""
#define TAG_LEN strlen(TAG)
#define MIN(a, b) (((a) < (b))? (a) : (b))

char *expanding_buffer_ensure(struct expanding_buffer *ebuf, int min_avail)
{
        int want;
        char *got;

        if (ebuf->avail >= min_avail)
                return ebuf->buf;

        if (min_avail >= INT_MAX/3)
                return 0;

        want = ebuf->avail + min_avail + 10;
        got = realloc(ebuf->buf, want);
        if (!got)
                return 0;

        ebuf->buf = got;
        ebuf->avail = want;
        return ebuf->buf;
}

char *sanitise_value(struct expanding_buffer *ebuf,const char *val, unsigned len)
{
        int used, remain, c;
        unsigned char *ip;

#define ADD(c) (ebuf->buf[used++] = (c))
#define ADDF(f,c) (used += sprintf(ebuf->buf+used, (f), (c)))

        ASSERT(len < INT_MAX/5);

        ip = (unsigned char *)val;
        used = 0;
        remain = len;

        if (!expanding_buffer_ensure(ebuf, remain + 1))
                return NULL;

        while (remain-- > 0) {
                c= *ip++;

                if (c >= ' ' && c <= '~' && c != '\\') {
                        ADD(c);
                        continue;
                }

                if (!expanding_buffer_ensure(ebuf, used + remain + 5))
                        /* for "<used>\\nnn<remain>\0" */
                        return 0;

                ADD('\\');
                switch (c) {
                case '\t':  ADD('t');   break;
                case '\n':  ADD('n');   break;
                case '\r':  ADD('r');   break;
                case '\\':  ADD('\\');  break;
                default:
                        if (c < 010) ADDF("%03o", c);
                        else         ADDF("x%02x", c);
                }
        }

        ADD(0);
        ASSERT(used <= ebuf->avail);
        return ebuf->buf;

#undef ADD
#undef ADDF
}


void do_lsxenstore(struct xs_handle *h, char *path, int cur_depth, int show_perms)
{
    static struct expanding_buffer ebuf;
    char **e;
    char newpath[STRING_MAX], *val;
    int newpath_len;
    int i;
    unsigned int num, len;

    e = xs_directory(h, XBT_NIL, path, &num);
    if (e == NULL)
	printf("xs_directory(%s) is not available! Maybe you have no permissions!\n",path);	

    for (i = 0; i<num; i++) {
        int linewid;

        /* Compose fullpath */
        newpath_len = snprintf(newpath, sizeof(newpath), "%s%s%s", path, 
                path[strlen(path)-1] == '/' ? "" : "/", 
                e[i]);

        /* Print indent and path basename */
        linewid = 0;
        if (show_whole_path) {
            fputs(newpath, stdout);
        } else {
            for (; linewid<cur_depth; linewid++) {
                putchar(' ');
            }
            linewid += printf("%.*s",(int) (max_width - TAG_LEN - linewid), e[i]);
        }

	/* Fetch value */
        if ( newpath_len < sizeof(newpath) ) {
            val = xs_read(h, XBT_NULL, newpath, &len);
        }
        else {
            /* Path was truncated and thus invalid */
            val = NULL;
            len = 0;
        }

        /* Print value */
        if (val == NULL) {
            printf(":\n");
        }
        else {
            if (max_width < (linewid + len + TAG_LEN)) {
                printf(" = \"%.*s\\...\"",
                       (int)(max_width - TAG_LEN - linewid),
		       sanitise_value(&ebuf, val, len));
            }
            else {
                linewid += printf(" = \"%s\"",
				  sanitise_value(&ebuf, val, len));
                if (show_perms) {
                    putchar(' ');
                    for (linewid++;
                         linewid < MIN(desired_width, max_width);
                         linewid++)
                        putchar((linewid & 1)? '.' : ' ');
                }
            }
        }
        free(val);

        /*if (show_perms) {
            perms = xenbus_get_perms(XBT_NIL, newpath, val_perms);
            if (perms == NULL) {
                warn("\ncould not access permissions for %s", perm);
            }
            else {
                int i;
                fputs("  (", stdout);
                for (i = 0; i < nperms; i++) {
                    if (i)
                        putchar(',');
                    xs_perm_to_string(perms+i, buf, sizeof(buf));
                    fputs(buf, stdout);
                }
                putchar(')');
            }
        }*/

        putchar('\n');
            
        do_lsxenstore(h, newpath, cur_depth+1, show_perms); 
    }
    free(e);
}

void ls_xenstore(char *command[],int s,int argc)
{
	struct xs_handle *xsh;
	int prefix=0,target_len;
	int optind=s+1;
	domid_t dom_id;
	char target_path_self[STRING_MAX],target_path_dom0[STRING_MAX];

	dom_id=xenbus_get_self_id();

	target_len=snprintf(target_path_self,sizeof(target_path_self),"/local/domain/%d",dom_id);
	target_len=snprintf(target_path_dom0,sizeof(target_path_dom0),"/local/domain/0");

	xsh=xs_daemon_open();
	if(xsh==NULL)
	{
		printf("lsxenstore error!\n");
		return;
	}
	
	if(optind == argc)    // 0 param
	{
		printf("----------------print %s ---------------------\n",target_path_self);
		do_lsxenstore(xsh,target_path_self,0,prefix);	
		printf("\n");
		/*printf("----------------print %s ---------------------\n",target_path_dom0);
		do_lsxenstore(xsh,target_path_dom0,0,prefix);*/
		return ;
	}
	
	do   // >=1 self-define param
	{
		printf("----------------print %s ---------------------\n",command[optind]);
		do_lsxenstore(xsh,command[optind],0,prefix);
		printf("\n");
		optind++;
	}while(optind < argc);
}

