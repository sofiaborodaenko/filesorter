#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PATH_MAX 1024
#define WORKER_COUNT 4
#define MAX_FILES 10

#include "parentWorker.h"



int main (int argc, char **argv) {

    char dir_to_use[PATH_MAX];
    DIR *d;
    struct dirent *entry; // to hold the current file in the directory

    char **valid_files = malloc(sizeof(char *) * (MAX_FILES + 1));
    int valid_file_count = 0;

    if (argc != 1) {
        fprintf(stderr, "Please provide the directory you want to use.");
        exit(1);
    }
    
    printf("Enter the directory to organize: ");
    scanf("%s", dir_to_use);
    printf("\n");

    d = opendir(dir_to_use);

    if (d == NULL) {
        fprintf(stderr, "Unable to open directory");
        free(valid_files);
        exit(1);
    }

    // go through the files in the given directoy, check if they are valid, and add them to the array
    int max_files = MAX_FILES;

    while ((entry = readdir(d)) != NULL) {
        // skip "." (current directory) and ".." (parent directory)
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (entry->d_name[0] == '.') {
            continue; // skip hidden files
        }

        // checks if its a regular file
        if (entry->d_type == DT_REG) {
            if (check_file_name(entry->d_name)) {
                // add to the array of valid files
                add_valid_file_to_array(&valid_files, &valid_file_count, &max_files, entry->d_name);
            
            }
        }
    }

    // if there are no valid files, exit the program
    if (valid_file_count == 0) {
        printf("No valid files found in the directory.\n");
        closedir(d);
        free(valid_files);
        return 0;
    }

    int job_pipe[WORKER_COUNT][2]; // for the parents to send to the workers
    int result_pipe[WORKER_COUNT][2]; // for the workers to send back to the parent

    for (int i = 0; i < WORKER_COUNT; i++) {
        if (pipe(job_pipe[i]) == -1) {
            perror("pipe");
            exit(1);
        }
        if (pipe(result_pipe[i]) == -1) {
            perror("pipe");
            exit(1);
        }


        int result = fork();
        if (result < 0) {
            perror("fork");
            exit(1);
        } else if (result == 0) { // in the children processes (workers)
            // closes the writing end of the job pipe (only reads)
            if (close(job_pipe[i][1])) {
                perror("close");
                exit(1);
            }

            // closes the reading end of the result pipe (only writes)
            if (close(result_pipe[i][0])) {
                perror("close");
                exit(1);
            }

            job_msg job;

            while (read(job_pipe[i][0], &job, sizeof(job_msg)) > 0) {
                result_msg result;
                // initialize the result struct to all 0s and NULLs to avoid garbage values
                memset(&result, 0, sizeof(result_msg));  

                char *clean_name = clean_filename(job.filename);
                char **target_path = create_target_path(job.filename);

                char *category = categorize_file(job.filename);
                int lines;
                int words;

                if (strcmp(category, "plain text") == 0) {
                    lines = count_lines(job.filename, dir_to_use);
                    words = count_words(job.filename, dir_to_use);
                } else {
                    lines = 0;
                    words = 0;
                }

                long size = count_size(job.filename, category, dir_to_use);

                char *new_directory = create_go_directory(dir_to_use, target_path, clean_name, job.filename);
                char new_path[PATH_MAX];
                strcpy(new_path, new_directory);

                // create the result struct to send back
                create_result(&result, job.job_id, job.filename, clean_name, new_path, category, lines, words, size);

                // write the result back to the parent
                if (write(result_pipe[i][1], &result, sizeof(result_msg)) == -1) {
                    fprintf(stderr, "Worker %d: skipping %s: %s\n", i, job.filename, strerror(errno));
                    free(clean_name);
                    for (int k = 0; target_path[k] != NULL; k++) {
                        free(target_path[k]);
                    }
                    free(target_path);
                    free(new_directory);
                    continue;
                }

                // free allocated memory after writing to 
                free(clean_name);
                for (int k = 0; target_path[k] != NULL; k++) free(target_path[k]);
                free(target_path);
                free(new_directory);

            }

             // parent had opened the previously closed ends of the previous forked children 
            for (int j = 0; j < i; j++) {
                if (job_pipe[j][1] != -1) {
                    if(close(job_pipe[j][1])){
                        perror("close");
                        exit(1);
                    }
                    job_pipe[j][1] = -1; // mark as closed
                }

                // closes the reading end of the result pipe (only writes)
                if (result_pipe[j][0] != -1) {
                    if(close(result_pipe[j][0])){
                        perror("close");
                        exit(1);
                    }
                    result_pipe[j][0] = -1; // mark as closed
                }
            }
            
            if (job_pipe[i][0] != -1) {
                if(close(job_pipe[i][0])){
                    perror("close");
                    exit(1);
                }
                job_pipe[i][0] = -1; // mark as closed
            }

            if (result_pipe[i][1] != -1) {
                if(close(result_pipe[i][1])){
                    perror("close");
                    exit(1);
                }
                result_pipe[i][1] = -1; // mark as closed
            }
        
            exit(0);

        } else { // parents proccess

            // closes the reading end of the job pipe (only writes)
            if (job_pipe[i][0] != -1) {
                if (close(job_pipe[i][0])) {
                    perror("close");
                    exit(1);
                }
                job_pipe[i][0] = -1; // mark as closed
            }

            // closes the writing end of the result pipe (only reads)
            if (result_pipe[i][1] != -1) {
                if (close(result_pipe[i][1])) {
                    perror("close");
                    exit(1);
                }
                result_pipe[i][1] = -1; // mark as closed
            }

        }

    } // only the parent gets here

    // parent distributes all jobs round robin style
    for (int i = 0; i < valid_file_count; i++) {
        int worker = i % WORKER_COUNT;
        job_msg job;
        // initialize the job struct to all 0s and NULLs to avoid garbage values
        memset(&job, 0, sizeof(job_msg));
        create_job(&job, valid_files[i], worker);
        
        if (write(job_pipe[worker][1], &job, sizeof(job_msg)) == -1) {
            fprintf(stderr, "Parent: skipping %s, failed to send to worker %d: %s\n", 
            valid_files[i], worker, strerror(errno));
            continue;
        }
    }

    // close all write ends so workers get EOF and exit
    for (int i = 0; i < WORKER_COUNT; i++) {
        close(job_pipe[i][1]); job_pipe[i][1] = -1;
    }

    char *original_filenames[valid_file_count];
    char *clean_filenames[valid_file_count];
    char *target_paths[valid_file_count];
    char *categories[valid_file_count];
    int lines[valid_file_count];   
    int words[valid_file_count];
    long sizes[valid_file_count];
    int result_count = 0;

    fd_set readfds;
    int max_fd = 0;

    // find max fd
    for (int i = 0; i < WORKER_COUNT; i++) {
        if (result_pipe[i][0] > max_fd) {
            max_fd = result_pipe[i][0];
        }
    }

    int active_pipes = WORKER_COUNT;

    // read until all workers are done
    while (active_pipes > 0) {
        FD_ZERO(&readfds); // clear the set before adding the fds

        for (int i = 0; i < WORKER_COUNT; i++) {
            if (result_pipe[i][0] != -1) {
                FD_SET(result_pipe[i][0], &readfds); // add the fd to the set
            }
        }

        int ready = select(max_fd + 1, &readfds, NULL, NULL, NULL); // wait for any of the fds to be ready for reading

        if (ready < 0) {
            perror("select");
            exit(1);
        }

        for (int i = 0; i < WORKER_COUNT; i++) {
            int fd = result_pipe[i][0];

            if (fd != -1 && FD_ISSET(fd, &readfds)) {
                result_msg result;

                int bytes = read(fd, &result, sizeof(result_msg));

                if (bytes > 0) {

                    if (result_count < valid_file_count) {
                        printf("Parent got result from worker %d: %s\n",
                            i, result.original_name);

                        //store the result in the arrays to print the summary later
                        original_filenames[result_count] = strdup(result.original_name);
                        clean_filenames[result_count] = strdup(result.clean_name);
                        target_paths[result_count] = strdup(result.target_path);
                        categories[result_count] = strdup(result.category);
                        lines[result_count] = result.lines;
                        words[result_count] = result.words;
                        sizes[result_count] = result.size;
                        result_count++;
                    }

                } else {
                    // pipe closed means worker done
                    close(fd);
                    result_pipe[i][0] = -1;
                    active_pipes--;
                }
            }
        }

        original_filenames[result_count] = NULL; // null terminate the arrays
    }

    for (int i = 0; i < WORKER_COUNT; i++) {
        int status;
        wait(&status);  // wait for each child to exit
    }

    // close all the remaining open fds
    for (int i = 0; i < WORKER_COUNT; i++) {
        if (result_pipe[i][0] != -1) {
            close(result_pipe[i][0]);
        }
    }

    printf("\n");
    print_summary(original_filenames, clean_filenames, target_paths, categories, lines, words, sizes, result_count);

    // clean up the allocated memory for the result arrays
    for (int i = 0; i < result_count; i++) {
        free(original_filenames[i]);
        free(clean_filenames[i]);
        free(target_paths[i]);
        free(categories[i]);
    }

    // clean up the allocated memory for the valid files array
    for (int i = 0; i < valid_file_count; i++) {
        free(valid_files[i]);
    }

    free(valid_files);

    return 0;

}
