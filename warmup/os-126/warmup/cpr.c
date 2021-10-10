#include "common.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

/* make sure to use syserror() when a system call fails. see common.h */

void
usage()
{
	fprintf(stderr, "Usage: cpr srcdir dstdir\n");
	exit(1);
}

void mkf(char *src, char *dst) {
	int fd_src, fd_dst;
	fd_src = open(src, O_RDONLY, S_IRUSR); //source read only, and user read permission for the owner
	if(fd_src < 0) {
		syserror(open, src);
	}
	fd_dst = creat(dst, S_IRWXU); //destination read, write, and execute permissions for the owner
	if(fd_dst < 0) {
		syserror(creat, dst);
	}

	char buf[4096];
	int ret;

	ret = read(fd_src, buf, 4096);
	if(ret < 0) {
		syserror(read, src);
	}
	while (ret != 0) {
		ret = write(fd_dst, buf, ret);
		if(ret < 0) {
			syserror(write, dst);
		}
		ret = read(fd_src, buf, 4096);
		if(ret < 0) {
			syserror(read, src);
		}
	}

	//close directories
	if(close(fd_src) < 0) {
		syserror(read, src);
	}
	if(close(fd_dst) < 0) {
		syserror(read, dst);
	}	

}

void cpr(char *src, char *dst) {
	
	struct stat stats; //used to get the properties on a file
	if(stat(src, &stats) < 0) {
		syserror(stat, src); //generate the error of stat
	}

	if(S_ISREG(stats.st_mode)) {
		//it is a file
		mkf(src, dst);
		if(chmod(dst, stats.st_mode)) {
			syserror(chmod, dst);
		}
	}
	else if(S_ISDIR(stats.st_mode)) {
		//it is a directory
		char *dirsrc = src;
		char *dirdst = dst;

		DIR *srcptr = opendir(dirsrc);
		
		if(mkdir(dirdst, S_IRWXU)) {
			syserror(mkdir, dirdst);
		}

		struct dirent *srcent = readdir(srcptr);

		char src_path[1000], dst_path[1000];

		while(srcent != NULL) {
			//skips self and prev
			if(strcmp(srcent->d_name, ".") == 0 || strcmp(srcent->d_name, "..") == 0) {
				srcent = readdir(srcptr);
				continue;
			}


			strcpy(dst_path, dirdst);
			strcat(dst_path, "/");
			strcat(dst_path, srcent->d_name);

			strcpy(src_path, dirsrc);
			strcat(src_path, "/");
			strcat(src_path, srcent->d_name);
			
			//recursive call
			cpr(src_path, dst_path);
			srcent = readdir(srcptr);
		
		}

		//copy permissions
		if(chmod(dirdst, stats.st_mode)) {
			syserror(chmod, dst);
		}
		if(closedir(srcptr) < 0) {
			syserror(closedir, dirsrc);
		}
	}
	return;
}

int
main(int argc, char *argv[])
{
	if (argc != 3) {
		usage();
	}
	
	//get the src and dst directories
	char *srcdir = argv[1];
	char *dstdir = argv[2];

	cpr(srcdir, dstdir);

	return 0;
}
