#include <stdio.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mnttab.h>
#include <unistd.h>
#include "format_floppy.h"

#define BUFSIZE  512

/* Forward declarations */
void rename_dos_label();
void run_command();

extern char *optarg;

/* Globals */
char *raw_device = NULL;
char *mnt_point = NULL;
char *popup = NULL;
char *label = NULL;
int x_position = 0;
int y_position = 0;
int rename_floppy = 0;

Popup_Type popup_type;

void
Usage(program)
char *program;
{
	fprintf(stdout, (char *)dgettext(MSGFILE, "Usage: %s -d raw_device -m mount_point [-p popup] [-x x_position] [-y y_position]\n"), program);
	fprintf(stdout, (char *)dgettext(MSGFILE, "where popup = {format, unlabeled, or unformatted}\n"));
	fprintf(stdout, (char *)dgettext(MSGFILE, "%s -r -d raw_device -m mount_point -n label_name\n"), program);
	fprintf(stdout, (char *)dgettext(MSGFILE, "default for popup is format\n"));
}

/* 
 * Returns 1 if the file system type at mnt_point is pcfs.
 * Returns 0 otherwise.
 */
int
dos_floppy()
{
	FILE *fp;
	struct mnttab *mp;

	if ( (fp = fopen(MNTTAB, "r")) == NULL ) {
		perror("fopen");
	} else {
		mp = (struct mnttab *)malloc(sizeof(struct mnttab));
		while ( getmntent(fp, mp) != -1 ) {
			if ( strcmp(mp->mnt_mountp, mnt_point) == 0 ) {
				if ( strcmp(mp->mnt_fstype, "pcfs") == 0 ) {
					free(mp);
					fclose(fp);
					return 1;
				}
			}
		}
		free(mp);
		fclose(fp);
	}
	return 0;
}

void
init_program(argc, argv)
int argc;
char *argv[];
{
	int arg;
	char buf[MAXPATHLEN];
	char *dir;

	if ( argc < 5 ) {
		Usage(argv[0]);
		exit(1);
	}
	while ( (arg = getopt(argc, argv, "d:m:n:p:rx:y:")) != EOF ) {
		switch (arg) {
			case 'd':
				raw_device = (char *)strdup(optarg);
				break;
			case 'm':
				mnt_point = (char *)strdup(optarg);
				break;
			case 'n':
				label = (char *)strdup(optarg);
				break;
			case 'p':
				popup = (char *)strdup(optarg);
				break;
			case 'r':
				rename_floppy = 1;
				break;
			case 'x':
				x_position = atoi(optarg);
				break;
			case 'y':
				y_position = atoi(optarg);
				break;
			case '?':
				Usage(argv[0]);
				exit(1);
		}
	}
	if ( popup == NULL ) {
		/* Default */
		popup = (char *)strdup((char *)FORMAT);
		popup_type = Format;
	} else if ( strcmp(popup, (char *)FORMAT) == 0 ) {
		popup_type = Format;
	} else if ( strcmp(popup, (char *)UNFORMAT) == 0 ) {
		popup_type = Unformatted;
	} else if ( strcmp(popup, (char *)UNLABEL) == 0 ) {
		popup_type = Unlabeled;
	} else {
		popup_type = Unformatted;
	}
	if ( (raw_device == NULL) || (mnt_point == NULL) ) {
		Usage(argv[0]);
		exit(1);
	}
	if ( rename_floppy ) {
		if ( dos_floppy() ) {
			rename_dos_label(raw_device, label);
		}
		if ( (dir = (char *)getenv("DESKSETHOME")) != NULL ) {
			sprintf(buf, "%s/ff.core -r %s %s %s", dir, raw_device, label, mnt_point);
		} else if ( (dir = (char *)getenv("OPENWINHOME")) != NULL ) {
			sprintf(buf, "%s/bin/ff.core -r %s %s %s", dir, raw_device, label, mnt_point);
		} else {
			return;
		}
		run_command(buf, 0);
		exit(0);
	}
}

void
run_command(command, root)
char *command;
int root;
{
	int pid;
	int stat_loc;

    switch (pid = fork()) {
        case -1:   /* Error */
            perror("fork");
            _exit(1);
        case 0:    /* Inside child */
            if ( root ) {
                setuid(0);
            }
            execl("/bin/sh", "sh", "-c", command, (char *)0);
            _exit(2);
        default:   /* Back in parent */
            if ( wait(&stat_loc) < 0 ) {
                perror("wait");
            }
    }
}

void
format_floppy(format_type)
int format_type;
{
	char buf[MAXPATHLEN];
	char *dir;

	if ( (dir = (char *)getenv("DESKSETHOME")) != NULL ) {
		sprintf(buf, "%s/ff.core %d %d %s %s ", dir, popup_type, format_type, raw_device, mnt_point);
	} else if ( (dir = (char *)getenv("OPENWINHOME")) != NULL ) {
		sprintf(buf, "%s/bin/ff.core %d %d %s %s ", dir, popup_type, format_type, raw_device, mnt_point);
	} else {
		return;
	}
	if ( (label != NULL) && (strlen(label) > 0) ) {
		strcat(buf, label);
	}	
	run_command(buf, 0);
}

/* Rename DOS floppy label
 */
void
rename_dos_label(device, label)
char *device;
char *label;
{
	int fd;
	int k;
	int j;
	char buffer[BUFSIZE];
	char oldlabel[12];
	char newlabel[11];

	if ( (fd = open(device, O_RDWR)) == -1 ) {
		fprintf(stderr, (char *)dgettext(MSGFILE, "Cannot open %s\n"), device);
		exit(1);
	}
	if ( (k = read(fd, buffer, BUFSIZE)) != BUFSIZE ) {
		/* DEBUG
		fprintf(stderr, "Read only %d\n", k);
		*/
	}
	for ( k = 0, j = 43;  k < 11;  k++, j++ ) {
		oldlabel[k] = buffer[j];
	}
	oldlabel[k] = '\0';
	strncpy(newlabel, label, 11);
	for ( k = 0, j = 43;  k < 11;  k++, j++ ) {
		buffer[j] = newlabel[k];
	}
	lseek(fd, 0, SEEK_SET);
	if ( (k = write(fd, buffer, BUFSIZE)) != BUFSIZE ) {
		/* DEBUG
		fprintf(stderr, "Wrote only %d\n", k);
		*/
	}
	close(fd);
}
