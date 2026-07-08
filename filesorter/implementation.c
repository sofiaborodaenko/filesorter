#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

#include "parentWorker.h"

/* Plain text / structured text */
#define TEXT_EXTENSIONS \
"txt", "log", "md", "csv", "tsv", "json", "xml", "yaml", "yml", "ini", "cfg", "conf", \
"html", "htm", "css", "js", "py", "c", "h", "cpp"

/* Document / formatted files */
#define DOC_EXTENSIONS \
"pdf", "doc", "docx", "odt", "rtf", "pages", \
"xls", "xlsx", "ods", "numbers", \
"ppt", "pptx", "odp", "key"

/* Audio formats */
#define AUDIO_VIDEO_EXTENSIONS \
"mp3", "aac", "ogg", "wav", "flac", "alac", "m4a", "opus", "wma", "aiff", "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "mpeg", "mpg", "m4v"


void create_result(result_msg *result, int job_id, char *original_name, char *clean_name, char *target_path, char *category, int lines, int words, long size) {

    result->job_id = job_id;
    strcpy(result->original_name, original_name);
    strcpy(result->clean_name, clean_name);
    strcpy(result->target_path, target_path);
    strcpy(result->category, category);
    result->lines = lines;
    result->words = words;
    result->size = size;

}


// cleans up the filename 
char *clean_filename(char *filename) {

    char *first_underscore = strchr(filename, '_'); // find the first occurence of the underscore
    char *period = strchr(filename, '.'); // find the first occurence of the period (file extension)

    int diff = strlen(filename) - strlen(first_underscore) ; // find length of the filename 

    char *clean_name = malloc(sizeof(char) * (diff + strlen(period) + 1));
    strncpy(clean_name, filename, diff);
    clean_name[diff] = '\0';
    strcat(clean_name, period);
    return clean_name;

}

char **create_target_path(char *filename) {

    char *copy = malloc(strlen(filename) + 1); // for the test with string literals 
    strcpy(copy, filename);

    char *tokens[10]; // temporary array to hold the broken up filename
    int n = 0;

    char *tok = strtok(copy, "_."); // break up the filename by _ and . and store in the array

    while (tok != NULL) {
        tokens[n++] = tok;
        tok = strtok(NULL, "_."); 
    }

    int words = n - 2; // subtract first and last tocken (acc file name and file extension) 

    char **target_path = malloc(sizeof(char *) * (words + 1));

    for (int j = 0; j < words; j++) {
        target_path[j] = malloc(sizeof(char) * (strlen(tokens[j + 1]) + 1));
        strcpy(target_path[j], tokens[j + 1]);
    }

    target_path[words] = NULL;

    return target_path;

}


char *categorize_file(char *filename){

    char *file_extension = strrchr(filename, '.') + 1; // find the file extension 

    char *text_extensions[] = {TEXT_EXTENSIONS, NULL};
    char *doc_extensions[] = {DOC_EXTENSIONS, NULL};
    char *audio_video_extensions[] = {AUDIO_VIDEO_EXTENSIONS, NULL};

    for (int i = 0; text_extensions[i] != NULL; i++) {
        if (strcmp(file_extension, text_extensions[i]) == 0) {
            return "plain text";
        }
    }
    
    for (int i = 0; doc_extensions[i] != NULL; i++) {
        if (strcmp(file_extension, doc_extensions[i]) == 0) {
            return "document";
        }
    }

    for (int i = 0; audio_video_extensions[i] != NULL; i++) {
        if (strcmp(file_extension, audio_video_extensions[i]) == 0) {
            return "audio/video";
        }
    }

    return "other";

}

int count_lines(char *filename, char *home_directory){

    char * full_path = malloc(strlen(home_directory) + strlen(filename) + 2); // for the slash and null terminator
    sprintf(full_path, "%s/%s", home_directory, filename);
    
    int lines = 0;

    FILE *open_file = fopen(full_path, "r");

    if (open_file == NULL) {
        fprintf(stderr, "Worker: failed to open %s for counting lines: %s\n", full_path, strerror(errno));
        free(full_path);
        return 0;
    }

    
    char buffer[1024]; // buffer to hold each line

    while (fgets(buffer, sizeof(buffer), open_file) != NULL) {
        lines++;
    }

    fclose(open_file);
    free(full_path);
    return lines;

}


int count_words(char *filename, char *home_directory){

    char *full_path = malloc(strlen(home_directory) + strlen(filename) + 2); // for the slash and null terminator
    sprintf(full_path, "%s/%s", home_directory, filename);

    FILE *open_file = fopen(full_path, "r");

    if (open_file == NULL) {
        fprintf(stderr, "Worker: failed to open %s for counting words: %s\n", full_path, strerror(errno));
        free(full_path);
        return 0;
    }

    int words = 0;
    char *word = malloc(sizeof(char) * 100); // buffer to hold each word

    while (fscanf(open_file, "%99s", word) == 1) {
        words++;
    }

    fclose(open_file);
    free(word);
    free(full_path);
    return words;
}


long count_size(char *filename, char *category, char *home_directory) {

    char *full_path = malloc(strlen(home_directory) + strlen(filename) + 2); // for the slash and null terminator
    sprintf(full_path, "%s/%s", home_directory, filename);

    FILE *open_file;
    
    if (strcmp(category, "plain text") == 0) {
        open_file = fopen(full_path, "r");
    } else {
        open_file = fopen(full_path, "rb");
    }

    if (open_file == NULL) {
        fprintf(stderr, "Worker: failed to open %s for counting size: %s\n", full_path, strerror(errno));
        free(full_path);
        return -1;
    }

    if (fseek(open_file, 0, SEEK_END) == -1) {
        perror("fseek");
        fclose(open_file);
        exit(1);
    }

    long size = ftell(open_file); // get the current file position 

    if (size == -1) {
        perror("ftell");
        fclose(open_file);
        free(full_path);
        return -1;
    }

    fclose(open_file);
    free(full_path);
    return size;

}

bool check_file_name(const char *filename){

    // whitespace characters to check
    const char *whitespace_chars = " \t\n\r\v\f";

    if (strchr(filename, '_') == NULL) {
        return false; // no underscore found
    }

    char *period = strchr(filename, '.'); // find first period

    if (period == NULL) {
        return false; // no period found
    }

    if (strpbrk(filename, whitespace_chars) != NULL) {
        return false; // whitespace character found
    }

    char *period_two = strchr(period + 1, '.'); // check if there is another period after the first one

    if (period_two != NULL) {
        return false; // more than one period found
    }

    return true;

} // can be used to check if the file is valid before sending it to the worker

char *create_go_directory(char *main_dir, char *dir_name[], char *clean_file_name, char *old_file_name){

    int size_of_dir = 0;
    for (int i = 0; dir_name[i] != NULL; i++) {
        size_of_dir += strlen(dir_name[i]) + 1; // add 1 for the slash
    }

    size_of_dir += strlen(main_dir) + 1; // add 1 for the slash
    size_of_dir += strlen(clean_file_name) + 1; // add 1 for the null terminator

    char *complete_directory = malloc(sizeof(char) * size_of_dir);
    strcpy(complete_directory, main_dir); // add the main directory 
    strcat(complete_directory, "/");

    char temp_path[256];
    strcpy(temp_path, main_dir);
    

    for (int i = 0; dir_name[i] != NULL; i++) {
        strcat(temp_path, "/");
        strcat(temp_path, dir_name[i]);

        struct stat st;
        if (stat(temp_path,&st) == -1) {
            if (mkdir(temp_path, 0755) == -1 && errno != EEXIST) {
                    perror("mkdir");
                    free(complete_directory);
                    exit(1);
                
            }
        }
        strcat(complete_directory, dir_name[i]);
        strcat(complete_directory, "/");
    }

    strcat(complete_directory, clean_file_name);

    char *home_dir = malloc(strlen(main_dir) + 1 + strlen(old_file_name) + 1);
    sprintf(home_dir, "%s/%s", main_dir, old_file_name);

    if (rename(home_dir, complete_directory) == -1) {
        fprintf(stderr, "Worker: failed to move %s to %s: %s\n", home_dir, complete_directory, strerror(errno));
        free(home_dir);
        free(complete_directory);
        exit(1);
    }

    free(home_dir);
    return complete_directory;

}

void create_job(job_msg *job, const char *filename, int job_id) {
    job->job_id = job_id;
    strcpy(job->filename, filename);
}

void add_valid_file_to_array(char ***valid_files, int *valid_file_count, int *max_files, char *filename) {
     
    if (*valid_file_count >= *max_files) {
        
        int new_size = *max_files * 2;
        char **error = realloc(*valid_files, sizeof(char *) * (new_size+1));

        if (error == NULL) {
            perror("realloc");
            exit(1);
        }

        *valid_files = error;
        *max_files = new_size;
    }

    (*valid_files)[*valid_file_count] = malloc(strlen(filename) + 1);
    strcpy((*valid_files)[*valid_file_count], filename);
    (*valid_file_count)++;
    (*valid_files)[*valid_file_count] = NULL;

    return;
}


void print_summary(char *original_filenames[], char *clean_filenames[], char *target_paths[], char *categorys[], int lines[], int words[], long sizes[], int num_files) {

    printf("-----------------------------------------------------------\n");
    printf("FILE ORGANIZATION SUMMARY \n");
    printf("-----------------------------------------------------------\n\n");

    int category_counts[4] = {0, 0, 0, 0}; // 0 = plain text, 1 = document, 2 = audio/video, 3 = other

    int total_lines = 0;
    int total_words = 0;


    for (int i = 0; original_filenames[i] != NULL; i++) {
        printf("%s", original_filenames[i]);
        printf(" -> %s", clean_filenames[i]);
        printf(" (%ld bytes)", sizes[i]);
        printf(" -> %s", target_paths[i]);
        printf("\n");

        if (strcmp(categorys[i], "plain text") == 0) {
            category_counts[0]++;
        } else if (strcmp(categorys[i], "document") == 0) {
            category_counts[1]++;
        } else if (strcmp(categorys[i], "audio/video") == 0) {
            category_counts[2]++;
        } else {
            category_counts[3]++;
        }

        total_lines += lines[i];
        total_words += words[i];
    }

    printf("\n");
    printf("-----------------------------------------------------------\n");
    printf("Statistics \n");
    printf("-----------------------------------------------------------\n");

    printf("Files processed: %d \n", num_files);
    printf("Plain text files: %d \n", category_counts[0]);
    printf("Document files: %d \n", category_counts[1]);
    printf("Audio/Video files: %d \n", category_counts[2]);
    printf("Other files: %d \n", category_counts[3]);
    printf("Total lines in plain text files: %d \n", total_lines);
    printf("Total words in plain text files: %d \n\n", total_words);


}

