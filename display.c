#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "terminal.h"
#include "display.h"

pthread_mutex_t lock;
// These variables should only be accessed/modified through locks.
static struct top_data top;
static struct middle_data middle;
static struct bottom_data bottom;
static int message;

void initialize_display(){
    if(pthread_mutex_init(&lock, NULL) != 0){
        printf("Failed to create lock\r\n");
        exit(1);
    }
    middle.first_data = NULL;
    middle.last_data = NULL;
    message = 0;
}

/**
 * Determines the amount of lines need to print the given string to the terminal.
 * @param width Number of columns of terminal
 * @param string String to be printed
 */
int lines_needed_to_print(int width, struct chat_data *to_display){
    int length = strlen(to_display->data);
    int size_message_body = (length / width) + 1;
    return size_message_body + 1;  // add one for the line showing username and date
}

void set_bottom_text(bool on_command_mode, char *text){
    pthread_mutex_lock(&lock);
    bottom.on_command_mode = on_command_mode;
    if(on_command_mode){
        strncpy(bottom.text, text, MAX_LENGTH_COMMAND);
    }else{
        strncpy(bottom.text, text, MAX_LENGTH_MESSAGE);
    }
    pthread_mutex_unlock(&lock);
}

void append_message(struct chat_data *new_data){
    pthread_mutex_lock(&lock);

    new_data->next = NULL;
    if(middle.first_data == NULL){
        // First message
        middle.first_data = new_data;
        middle.last_data = new_data;
    }else{
        // all other message order
        middle.last_data->next = new_data;
        new_data->previous = middle.last_data;
        middle.last_data = new_data;
    }
    message++;

    pthread_mutex_unlock(&lock);
}

void print_top_data(int width, int height, char *buffer){
    pthread_mutex_lock(&lock);
    // Print top bar at top left
    char *to_print = "\x1b[;1HTop bar\r\n";
    strcat(buffer, to_print);
    pthread_mutex_unlock(&lock);
}

void print_middle_data(int width, int height, char *buffer){
    pthread_mutex_lock(&lock);

    int lines_available = height - TOP_LINES - BOTTOM_LINES;
    if(lines_available <= 0){
        char *to_print = "Not enough space to print everything\r\n";
        printf("%s\r\n", to_print);
        exit(1);
    }

    // find the first (oldest) message that cannot fit on the screen
    struct chat_data *oldest_msg = middle.last_data;
    while(oldest_msg != NULL && (lines_available -= lines_needed_to_print(width, oldest_msg)) >= 0){
        oldest_msg = oldest_msg->previous;
    }

    // If oldest_msg is NULL, we have space for all chat_data, otherwise start at the following chat_data
    struct chat_data *current_msg = oldest_msg == NULL ? middle.first_data : oldest_msg->next;
    while(current_msg != NULL){
        // print the username
        char *heading = "Username: x Time: y\r\n";
        strcat(buffer, heading);

        // print actual message
        strcat(buffer, current_msg->data);
        strcat(buffer, "\r\n");

        current_msg = current_msg->next;
    }

    // Fill rest of lines available with empty text
    while(lines_available > 0){
        strcat(buffer, "\r\n");
        lines_available--;
    }

    pthread_mutex_unlock(&lock);
}

void print_bottom_data(int width, int height, char *buffer){
    pthread_mutex_lock(&lock);
    char *text = bottom.text;
    int length = strlen(text);
    int lines_required = length / width + 1;
    // Byte layout:
    // \x1b -- escape character (1 byte)
    // [???;???H -- each question mark is one byte. 9 bytes
    // end of string character (1 byte)
    char cursor_reposition[11] = {0};

    if(lines_required == 1){
        strcat(buffer, text);
        strcat(buffer, "\n\r");
        // add one to length since cursor needs its own cell
        sprintf(cursor_reposition, "\x1b[%d;%dH", height - 1, length + 1);
    }else if(lines_required == 2){
        strcat(buffer, text);
        strcat(buffer, "\r");
        // add one to length since cursor needs its own cell
        sprintf(cursor_reposition, "\x1b[%d;%dH", height, length - width + 1);
    }else{
        // More than 2 lines required, but there is space only for two, so show last x characters that can fit
        text += length - (width * 2) + 1; // reserve one space for cursor
        strcat(buffer, text);
        strcat(buffer, "\r");
        sprintf(cursor_reposition, "\x1b[%d;%dH", height, width);
    }

    strcat(buffer, cursor_reposition);
    pthread_mutex_unlock(&lock);
}

void update_screen(){
    int dimensions[2];
    get_terminal_dimensions(dimensions);
    int width = dimensions[0];
    int height = dimensions[1];
    // Multiply by four since each cell of terminal can have additional information (like background color)
    char *buffer = calloc(width * height * 4, sizeof(char));

    print_top_data(width, height, buffer);
    print_middle_data(width, height, buffer);
    print_bottom_data(width, height, buffer);

    write(STDOUT_FILENO, buffer, strlen(buffer));

    free(buffer);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void *display(void *param){
    struct timespec nano_time;
    nano_time.tv_nsec = 100000000; // .1 seconds

    while(1){
        nanosleep(&nano_time, &nano_time);
        clear_terminal();
        update_screen();
    }
}
#pragma clang diagnostic pop