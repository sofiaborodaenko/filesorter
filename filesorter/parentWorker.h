#include <stdbool.h>

#ifndef PARENTWORKER_H
#define PARENTWORKER_H
#define MAX_CHAR 256


typedef struct {
    int job_id;
    char filename[MAX_CHAR];

} job_msg;

typedef struct {
    int job_id;
    char original_name[MAX_CHAR];
    char clean_name[MAX_CHAR];
    char target_path[MAX_CHAR];
    char category[MAX_CHAR];
    int lines;
    int words;
    long size; 

} result_msg;

// worker functions 
void create_result(result_msg *result, int job_id, char *original_name, char *clean_name, char *target_path, char *category, int lines, int words, long size);
char *clean_filename( char *filename);
char **create_target_path(char *filename);
char *categorize_file(char *filename);
int count_lines(char *filename, char *home_directory);
int count_words(char *filename, char *home_directory);
long count_size(char *filename, char *category, char *home_directory);

bool check_file_name(const char *filename); // used to check if the file is valid before sending it to the worker
void add_valid_file_to_array(char ***valid_files, int *valid_file_count, int *max_files, char *filename); // add the valid file to an array that the parent will send to the workers
char *create_go_directory(char * main_dir, char *dir_name[], char *clean_file_name, char *old_file_name);

// parent functions
void create_job(job_msg *job, const char *filename, int job_id);
void print_summary(char *original_filenames[], char *clean_filenames[], char *target_paths[], char *categorys[], int lines[], int words[], long sizes[], int num_files);


#endif
