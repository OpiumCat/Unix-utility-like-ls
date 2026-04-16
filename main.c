#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>

struct Record {
	char *name,
		mode[11],
		links_count[10],
		*user, *group,
		memory_size[30],
		mtime[17];
};

int compareRecords(const void* record1, const void* record2) {
	return strcmp(((struct Record*)record1)->name,
					((struct Record*)record2)->name);
}

int max(int a, int b) {
	if(a > b) return a;
	return b;
}

void updateColumnsWidth(struct Record* record, int *links_count_width,
	int *user_width, int *group_width, int *memory_size_width) {
	*links_count_width = max(*links_count_width, strlen(record->links_count));
	*user_width = max(*user_width, strlen(record->user));
	*group_width = max(*group_width, strlen(record->group));
	*memory_size_width = max(*memory_size_width, strlen(record->memory_size));
}

void printRecord(struct Record* record, int is_detailed,
	int links_count_width, int user_width, int group_width, int memory_size_width) {
	if(is_detailed) {
		printf("%s %*s %-*s %-*s %*s %s %s\n",
			record->mode, links_count_width, record->links_count,
			user_width, record->user, group_width, record->group,
			memory_size_width, record->memory_size, record->mtime, record->name);
	} else {
		printf("%s\n", record->name);
	}
}

struct RecordsArray {
	struct Record* array;
	int count, capacity;
};

struct RecordsArray createRecordsArray() {
	const size_t START_CAPACITY = 8;
	struct RecordsArray records;
	records.array = malloc(START_CAPACITY * sizeof(struct Record));
	records.count = 0;
	records.capacity = START_CAPACITY;
	return records;
}

void destroyRecordsArray(struct RecordsArray* records) {
	for(int i = 0; i < records->count; i++) {
		free((struct Record*)(records->array + i)->name);
		free((struct Record*)(records->array + i)->user);
		free((struct Record*)(records->array + i)->group);
	}
	free(records->array);
}

void pushInRecordsArray(struct RecordsArray* records, struct Record* new_record) {
	if(records->count == records->capacity) {
		records->capacity *= 2;
		records->array = realloc(records->array, records->capacity * sizeof(struct Record));
	}
	memcpy(records->array + records->count, new_record, sizeof(struct Record));
	records->count++;
}

void sortRecordsArray(struct RecordsArray* records) {
	if(records->count > 0) {
		qsort(records->array, records->count, sizeof(struct Record), &compareRecords);
	}
}

void reverseRecordsArray(struct RecordsArray* records) {
	if(records->count > 0) {
		struct Record* temp = malloc(records->count * sizeof(struct Record));
		for(int i = 0; i < records->count; i++) {
			memcpy(temp + i, records->array + i, sizeof(struct Record));
		}
		for(int i = 0; i < records->count; i++) {
			memcpy(records->array + i, temp + (records->count - 1 - i), sizeof(struct Record));
		}
		free(temp);
	}
}

void printRecordsArray(struct RecordsArray* records, int is_detailed) {
	int links_count_width = 0, user_width = 0, group_width = 0, memory_size_width = 0;
	if(is_detailed) {
		for(int i = 0; i < records->count; i++) {
			updateColumnsWidth(records->array + i, &links_count_width,
				&user_width, &group_width, &memory_size_width);
		}
	}
	for(int i = 0; i < records->count; i++) {
		printRecord(records->array + i, is_detailed,
			links_count_width, user_width, group_width, memory_size_width);
	}
}

void modeToStr(mode_t mode, char* str) {
	if(S_ISREG(mode)) str[0] = '-';
	else if(S_ISDIR(mode)) str[0] = 'd';
	else if(S_ISLNK(mode)) str[0] = 'l';
	else if(S_ISCHR(mode)) str[0] = 'c';
	else if(S_ISBLK(mode)) str[0] = 'b';
	else if(S_ISFIFO(mode)) str[0] = 'p';
	else if(S_ISSOCK(mode)) str[0] = 's';
	else str[0] = '?';

	str[1] = ((mode & S_IRUSR) ? 'r' : '-');
	str[2] = ((mode & S_IWUSR) ? 'w' : '-');
	str[3] = ((mode & S_IXUSR) ? 'x' : '-');

	str[4] = ((mode & S_IRGRP) ? 'r' : '-');
	str[5] = ((mode & S_IWGRP) ? 'w' : '-');
	str[6] = ((mode & S_IXGRP) ? 'x' : '-');

	str[7] = ((mode & S_IROTH) ? 'r' : '-');
	str[8] = ((mode & S_IWOTH) ? 'w' : '-');
	str[9] = ((mode & S_IXOTH) ? 'x' : '-');

	if(mode & S_ISUID) str[3] = ((str[3] == 'x') ? 's' : 'S');
	if(mode & S_ISGID) str[6] = ((str[6] == 'x') ? 's' : 'S');
	if(mode & S_ISVTX) str[9] = ((str[9] == 'x') ? 't' : 'T');

	str[10] = '\0';
}

void setTwoDigitNumberInStr(char* str, int num) {
	if(num < 10) {
		sprintf(str, "0%d", num);
	} else {
		sprintf(str, "%d", num);
	}
}

void timeToStr(const time_t* t, char* str) {
	struct tm *t_data = localtime(t);
	int day = t_data->tm_mday;
	int month = t_data->tm_mon + 1;
	int year = t_data->tm_year + 1900;
	int hour = t_data->tm_hour;
	int minute = t_data->tm_min;
	setTwoDigitNumberInStr(str, day);
	str[2] = '.';
	setTwoDigitNumberInStr(str + 3, month);
	str[5] = '.';
	sprintf(str + 6, "%d", year);
	str[10] = ' ';
	setTwoDigitNumberInStr(str + 11, hour);
	str[13] = ':';
	setTwoDigitNumberInStr(str + 14, minute);
	str[16] = '\0';
}

void memoryToStr(long long count, char* str, size_t str_size, int useful_format) {
	if(!useful_format) {
		snprintf(str, str_size, "%lld", count);
		return;
	}
	if(count < 1024) {
		snprintf(str, str_size, "%d b", (int)count);
		return;
	}
	double fcount = (double)(count) / 1024;
	if(fcount < 1024) {
		snprintf(str, str_size, "%.1f Kb", fcount);
		return;
	}
	fcount /= 1024;
	if(fcount < 1024) {
		snprintf(str, str_size, "%.1f Mb", fcount);
		return;
	}
	fcount /= 1024;
	snprintf(str, str_size, "%.1f Gb", fcount);
}

int main(int argc, char **argv) {

	// set options
	const char *OPTIONS = "+lrh";
	int is_detailed_and_sorted = 0;
	int is_reversed = 0;
	int useful_format = 0;
	
	char opt;
	while((opt = getopt(argc, argv, OPTIONS)) != -1) {
		switch(opt) {
			case 'l':
				is_detailed_and_sorted = 1;
				break;
			case 'r':
				is_reversed = 1;
				break;
			case 'h':
				useful_format = 1;
				break;
		}
	}

	// set path
	const size_t MAX_PATH_LEN = 4096;
	char path[MAX_PATH_LEN];

	if(optind == argc - 1) {
		memcpy(path, argv[argc - 1], strlen(argv[argc - 1]) + 1);
	} else if(optind < argc - 1) {
		fprintf(stderr, "ERROR: Incorrect structure of arguments\n");
		fprintf(stderr, "The correct structure: ... [-l] [-r] [-h] [path]\n");
		exit(EXIT_FAILURE);
	} else {
		assert(optind == argc);
		if(!getcwd(path, MAX_PATH_LEN)) {
			perror("getcwd");
			exit(EXIT_FAILURE);
		}
	}

	// open directory and output content
	DIR *dir = opendir(path);
	if(!dir) {
		fprintf(stderr, "ERROR: The program can't open directory\n");
		exit(EXIT_FAILURE);
	}

	struct dirent *current_object;
	struct RecordsArray records = createRecordsArray();
	while(current_object = readdir(dir)) {
		if(strcmp(current_object->d_name, ".") == 0 || strcmp(current_object->d_name, "..") == 0) {
			continue;
		}
		struct Record current_record;
		current_record.name = strdup(current_object->d_name);
		char object_path[MAX_PATH_LEN];
		memcpy(object_path, path, strlen(path));
		object_path[strlen(path)] = '/';
		memcpy(object_path + strlen(path) + 1, current_object->d_name, strlen(current_object->d_name) + 1);
		struct stat object_stat;
		if(lstat(object_path, &object_stat) == -1) {
			perror("lstat");
			exit(EXIT_FAILURE);
		} 
		modeToStr(object_stat.st_mode, current_record.mode);
		snprintf(current_record.links_count, sizeof(current_record.links_count), "%d", (int)object_stat.st_nlink);
		struct passwd *pwd = getpwuid(object_stat.st_uid);
		current_record.user = strdup((pwd ? pwd->pw_name : "?"));
		struct group *grp = getgrgid(object_stat.st_uid);
		current_record.group = strdup((grp ? grp->gr_name : "?"));
		memoryToStr(object_stat.st_size, current_record.memory_size, sizeof(current_record.memory_size), useful_format);
		timeToStr(&object_stat.st_mtime, current_record.mtime);
		pushInRecordsArray(&records, &current_record);
	}

	if(is_detailed_and_sorted) {
		sortRecordsArray(&records);
	}
	if(is_reversed) {
		reverseRecordsArray(&records);
	}
	printRecordsArray(&records, is_detailed_and_sorted);
	destroyRecordsArray(&records);

	return 0;
}