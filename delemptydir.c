/*      oldfiles.c
 *
 *  Copyright 2011 Bob Parker <rlp1938@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <utime.h>
#include <libgen.h>
#include "fileutil.h"
static char *helpmsg =
  "NAME\n\tdelemptydir - deletes empty directories\n\n."
  "SYNOPSIS"
  "\n\tdelemptydir [option] [topdir]\n"
  "\n\tDeletes empty dirs under the user input topdir\n"
  "\nOPTIONS\n"
  "\t-h outputs this help message.\n"
;

static void recursedir(char *topdir);
static void dohelp(int forced);
static void dorealpath(char *givenpath, char *resolvedpath);
static int dirobjectcount(DIR *dp);

int main(int argc, char **argv)
{
    int opt;
    char topdir[PATH_MAX];
    struct stat sb;

	// initialise defaults
    strcpy(topdir, getenv("HOME"));

    while((opt = getopt(argc, argv, ":ha:o:")) != -1) {
        switch(opt){
        case 'h':
            dohelp(0);
        break;
        case ':':
            fprintf(stderr, "Option %c requires an argument\n",optopt);
            dohelp(1);
        break;
        case '?':
            fprintf(stderr, "Illegal option: %c\n",optopt);
            dohelp(1);
        break;
        } //switch()
    }//while()

    // now process the non-option arguments

    // 1.See if argv[1] exists.
    if ((argv[optind])) {
       strcpy(topdir, argv[optind]);  // default is /home/$USER
    }

    // Check that the top dir is legitimate.
    if (stat(topdir, &sb) == -1) {
        perror(topdir);
        exit(EXIT_FAILURE);
    }
    // It exists then, but is it a dir?
    if (!(S_ISDIR(sb.st_mode))) {
        fprintf(stderr, "%s is not a directory!\n", topdir);
        exit(EXIT_FAILURE);
    }

	// Convert relative path to absolute if needed.
	if (topdir[0] != '/') dorealpath(argv[optind], topdir);

    recursedir(topdir);

	return 0;
} // main()

void dohelp(int forced)
{
  fputs(helpmsg, stderr);
  exit(forced);
}

void recursedir(char *path)
{
    /*
     * Traverse dirs and delete empties.
    */
    struct dirent *de;
    DIR *dp;
    int objcount;
    //FILE *fpo = fopen("shit.dat", "a");

    dp = opendir(path);
    if (!(dp)) {
        perror(path);
        exit(EXIT_FAILURE);
    }
    //fprintf(fpo, "path: %s\n", path);
    //fclose(fpo);
    while ((de = readdir(dp))) {
        char newpath[PATH_MAX];
        if (strcmp(de->d_name, ".") == 0) continue;
        if (strcmp(de->d_name, "..") == 0) continue;
        strcpy(newpath, path);
        if (newpath[strlen(newpath)-1] != '/') strcat(newpath, "/");
        switch (de->d_type) {
            case DT_DIR:
            // process this dir
            strcat(newpath, de->d_name);
            recursedir(newpath);
            break;
			default:
            // ignore all other d_types
            break;
		} // switch()
	} // while()
	objcount = dirobjectcount(dp);
	if (!objcount) {
		if (rmdir(path) == -1) {
			perror(path);
		}
		sync();	// make the next higher dir aware.
	}
    closedir(dp);
} // recursedir()

void dorealpath(char *givenpath, char *resolvedpath)
{	// realpath() witherror handling.
	if(!(realpath(givenpath, resolvedpath))) {
		perror("realpath()");
		exit(EXIT_FAILURE);
	}
} // dorealpath()

int dirobjectcount(DIR *dp)
{
	struct dirent *de;
	int objcount = 0;
	while ((de = readdir(dp))) {
        if (strcmp(de->d_name, ".") == 0) continue;
        if (strcmp(de->d_name, "..") == 0) continue;
        objcount++;
	}
	return objcount;
} // count objects belonging to a dir
