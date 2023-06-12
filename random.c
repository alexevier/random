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
	bool recursive;
	bool expand;
};

// globalls :)
static struct Options OPTION = {
	.dir = true,
	.file = true,
	.hidden = false,
	.recursive = false,
	.expand = false,
};
static const char HELP[];
static char PATH[PATH_MAX] = {0};
static char** ITEM = NULL;
static size_t ITEM_COUNT = 0;
static byte ERROR = OK;

static void addItem(const char* item);
static byte parseDir(const char* dirPath);
static byte formatPath(char out[PATH_MAX], const char* in1, const char* in2);
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
					case 'r':
						OPTION.recursive = true;
						break;
					case 'd':
						OPTION.file = false;
						OPTION.dir = true;
						break;
					case 'f':
						OPTION.file = true;
						OPTION.dir = false;
						break;
					case 'e':
						OPTION.expand = true;
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
		OPTION.expand = false;
	}
	
	if(OPTION.expand){
		char pathtmp[PATH_MAX];
		char* res = realpath(PATH, pathtmp);
		if(!res){
			perror("can't expand path");
			return ERR_MAJOR;
		}
		memcpy(PATH, pathtmp, PATH_MAX);
	}
	
	size_t pathlen = strlen(PATH);
	if(pathlen >= (PATH_MAX-1)){
		printf("max path length reached: %u", PATH_MAX);
		return ERR_MAJOR;
	}
	
	if(PATH[pathlen-1] != '/'){
		PATH[pathlen] = '/';
	}
	
	if(parseDir(NULL)){
		perror(PATH);
		return ERR_MAJOR;
	}
	
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
	
	return ERROR;
}

static void addItem(const char* item){
	if(ITEM){
		char** newmem = realloc(ITEM, (ITEM_COUNT+1) * sizeof(char**));
		if(!newmem){
			perror("realloc()");
			exit(ERR_MAJOR);
		}
		ITEM = newmem;
	} else {
		ITEM = malloc(sizeof(char**));
		if(!ITEM){
			perror("malloc()");
			exit(ERR_MAJOR);
		}
	}
	
	size_t size = strlen(item)+1; // this will include the null char
	ITEM[ITEM_COUNT] = malloc(size);
	if(!ITEM[ITEM_COUNT]){
		perror("malloc()");
		exit(ERR_MAJOR);
	}
	memcpy(ITEM[ITEM_COUNT], item, size);
	ITEM_COUNT++;
}

static byte parseDir(const char* dirPath){
	char pathbuf[PATH_MAX];
	char path[PATH_MAX];
	
	if(dirPath){
		snprintf(path, PATH_MAX, "%s%s", PATH, dirPath);
	} else {
		snprintf(path, PATH_MAX, "%s", PATH);
	}
	
	DIR* dir = opendir(path);
	if(!dir){
		ERROR = ERR_MINOR;
		return ERR_MINOR;
	}
	
	for(;;){
		struct dirent* dire = readdir(dir);
		if(!dire)
			break;
		
		if(dire->d_name[0] == '.'){
			if(!OPTION.hidden){
				continue;
			}
			if((dire->d_name[1] == '.' || dire->d_name[1] == '\0')){
				continue;
			}
		}
		
		if((dire->d_type == DT_REG) && !OPTION.file){
			continue;
		}
		
		if(dire->d_type == DT_DIR){
			if(OPTION.recursive){
				formatPath(pathbuf, dirPath, dire->d_name);
				parseDir(pathbuf);
			}
			if(!OPTION.dir)
				continue;
		}
		
		if(dire->d_type == DT_LNK){
			formatPath(pathbuf, path, dire->d_name);
			
			struct stat stt;
			if(stat(pathbuf, &stt) != 0){
				ERROR = ERR_MINOR;
				continue;
			}
			
			if(S_ISDIR(stt.st_mode)){
				if(OPTION.recursive){
					formatPath(pathbuf, dirPath, dire->d_name);
					parseDir(pathbuf);
				}
				if(!OPTION.dir)
					continue;
			}
			
			if(S_ISREG(stt.st_mode) && !OPTION.file){
				continue;
			}
		}
		
		formatPath(pathbuf, dirPath, dire->d_name);
		addItem(pathbuf);
	}
	
	closedir(dir);
	return OK;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation="
static byte formatPath(char out[PATH_MAX], const char* in1, const char* in2){
	if(in1 && in2){
		if(in1[strlen(in1)-1] != '/')
			snprintf(out, PATH_MAX, "%s/%s", in1, in2);
		else
			snprintf(out, PATH_MAX, "%s%s", in1, in2);
	} else if(in1) {
		snprintf(out, PATH_MAX, "%s", in1);
	} else if(in2){
		snprintf(out, PATH_MAX, "%s", in2);
	} else {
		out[0] = '\0';
		return ERR_MINOR;
	}
	return OK;
}
#pragma GCC diagnostic pop

// seems like not all systems have a random() function in libc but do have a syscall
static unsigned int lrandom(void){
	unsigned int num = 0;
	ssize_t err = syscall(SYS_getrandom, &num, sizeof(unsigned int), GRND_NONBLOCK);
	if(err == -1){
		// fallback to rand() in case of error
		ERROR = ERR_MINOR;
		srand(time(NULL));
		num = rand();
	}
	return num;
}

static const char HELP[] =
"Usage: random [OPTION]... [DIR]...\n"
"Selects random file/dir from a directory (the current directory by default).\n"
"\n"
"Options:\n"
"  -a, include all\n"
"  -r, recursive\n"
"  -d, dirs only\n"
"  -f, files only\n"
"  -e, expand relative path\n"
"  -h, print this help and exit\n"
"  --, stop parsing options\n"
"\n"
"Notes:\n"
". and .. are always ignored\n"
"if -a is not used, hidden directories are ignored in recursion\n"
"\n"
"Exit status:\n"
"  0  Ok\n"
"  1  minor problem\n"
"  2  problematic problem\n"
"\n"
"Version: "VERSION"\n"
"Author: alexevier <alexevier@proton.me>\n"
"License: GPLv2\n";
