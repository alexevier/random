#include <linux/limits.h>
#define _GNU_SOURCE
#include<sys/syscall.h>
#include<sys/random.h>
#include<sys/stat.h>
#include<stdbool.h>
#include<stddef.h>
#include<unistd.h>
#include<dirent.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<time.h>

#define OK 0
#define ERR_MINOR 1
#define ERR_MAJOR 2

typedef unsigned char byte;
struct Options {
	bool dir;
	bool file;
	bool hidden;
	bool all;
};

// globalls :)
static struct Options OPTION = {
	.dir = true,
	.file = true,
	.hidden = false,
	.all = true,
};
static const char HELP[];
static char PATH[PATH_MAX] = {0};
static char** ITEM = NULL;
static size_t ITEM_COUNT = 0;
static byte ERR = OK;

static void addItem(const char* item);
static unsigned int lrandom(void);

int main(int argc, char** argv){
	// TODO refactor arg parsing, this one is awfull
	bool parseArgs = true;
	for(int i = 1; i < argc; i++){
		size_t arglen = strlen(argv[i]);
		if(parseArgs && argv[i][0] == '-'){
			if(argv[i][1] == '-' && argv[i][2] == '\0'){
				parseArgs = false;
				continue;
			}
			for(unsigned int j = 1; j < arglen; j++){
				switch(argv[i][j]){
					case 'a':
						OPTION.hidden = true;
						break;
					case 'A':
						OPTION.hidden = true;
						OPTION.all = false;
						break;
					case 'd':
						OPTION.file = false;
						OPTION.dir = true;
						break;
					case 'f':
						OPTION.file = true;
						OPTION.dir = false;
						break;
					default:
						printf("unknown option: %c\n", argv[i][j]);
						//fallthrough
					case 'h':
						printf("%s", HELP);
						return OK;
				}
			}
			continue;
		}
		memcpy(PATH, argv[i], arglen);
	}
	
	if(!PATH[0]){
		if(!getcwd(PATH, PATH_MAX)){
			perror(NULL);
			return ERR_MAJOR;
		}
	}
	
	size_t pathlen = strlen(PATH);
	if(pathlen >= (PATH_MAX-1)){
		printf("max path len reached: %u", PATH_MAX);
		return ERR_MAJOR;
	}
	if(PATH[pathlen-1] != '/'){
		PATH[pathlen] = '/';
	}
	
	DIR* dir = opendir(PATH);
	if(!dir){
		perror(PATH);
		return ERR_MAJOR;
	}
	
	for(;;){
		struct dirent* dire = readdir(dir);
		if(!dire)
			break;
		
		if(dire->d_name[0] == '.'){
			if(!OPTION.hidden){
				continue;
			}
			if((dire->d_name[1] == '.' || dire->d_name[1] == '\0') && !OPTION.all){
				continue;
			}
		}
		
		if((dire->d_type == DT_REG) && !OPTION.file){
			continue;
		} else if((dire->d_type == DT_DIR) && !OPTION.dir){
			continue;
		} else if(dire->d_type == DT_LNK){
			char path[PATH_MAX];
			int fmtres = snprintf(path, PATH_MAX, "%s%s", PATH, dire->d_name);
			if(fmtres < 0){
				ERR = ERR_MINOR;
				continue;
			}
			
			struct stat stt;
			if(stat(path, &stt) != 0){
				ERR = ERR_MINOR;
				continue;
			}
			
			if(S_ISREG(stt.st_mode) && !OPTION.file){
				continue;
			}
			if(S_ISDIR(stt.st_mode) && !OPTION.dir){
				continue;
			}
		}
		
		addItem(dire->d_name);
	}
	
	closedir(dir);
	
	if(ITEM_COUNT){
		int ran = lrandom() % ITEM_COUNT;
		printf("%s%s\n", PATH, ITEM[ran]);
	}
	
	if(ITEM){
		for(size_t i = 0; i < ITEM_COUNT; i++){
			if(ITEM[i]){
				free(ITEM[i]);
			}
		}
		free(ITEM);
	}
	
	return ERR;
}

static void addItem(const char* item){
	if(ITEM){
		char** newmem = realloc(ITEM, (ITEM_COUNT+1) * sizeof(char**));
		if(!newmem){
			perror(NULL);
			exit(ERR_MAJOR);
		}
		ITEM = newmem;
	} else {
		ITEM = malloc(sizeof(char**));
		if(!ITEM){
			perror(NULL);
			exit(ERR_MAJOR);
		}
	}
	
	size_t size = strlen(item)+1; // this will include the null char
	ITEM[ITEM_COUNT] = malloc(size);
	if(!ITEM[ITEM_COUNT]){
		perror(NULL);
		exit(ERR_MAJOR);
	}
	memcpy(ITEM[ITEM_COUNT], item, size);
	ITEM_COUNT++;
}

// seems like not all unix systems have a random() function in libc but a syscall
static unsigned int lrandom(void){
	unsigned int num = 0;
	ssize_t err = syscall(SYS_getrandom, &num, sizeof(unsigned int), GRND_NONBLOCK);
	if(err == -1){
		// fallback to rand() in case of error
		ERR = ERR_MINOR;
		srand(time(NULL));
		num = rand();
	}
	return num;
}

static const char HELP[] =
"\
Usage: random [OPTION]... [DIR]...\n\
Selects random file/dir from a directory (the current directory by default).\n\
\n\
Options:\n\
  -a, include all\n\
  -A, include all but . and ..\n\
  -d, dirs only\n\
  -f, files only\n\
  -h, print help and exit\n\
\n\
Exit status:\n\
  0  Ok\n\
  1  minor problem\n\
  2  problematic problem\n\
\n\
Version: "VERSION"\n\
License: GPLv2\n";
