#ifndef CHAT_H
#define CHAT_H
// char* length constants. These include end of string character.
#define MAX_LENGTH_USERNAME 26
#define MAX_LENGTH_MESSAGE 256
#define MAX_LENGTH_COMMAND 8

/**
 * Every message is either a:
 * - TEXT: What someone types in to send to everyone
 * - NOTIFICATION: Information on the current state of the chat (notify everyone if someone connects / leaves)
 */
enum MessageType {
    TEXT,
    NOTIFICATION
};
struct message {
    char username[MAX_LENGTH_USERNAME];  // Who sent the message. Empty string if message is a NOTIFICATION type.
    enum MessageType message_type;
    char content[MAX_LENGTH_MESSAGE];  // Notification or text body
    long millisecond_time;  // Time the message was received by the server
    struct message *next;  // The following message. NULL if this message is the last one.
    struct message *previous;  // The previous message. NULL if this message is the first.
};

/**
 * Must call this every time this application is launched.
 * @param chat_name File name that contains the chat log
 */
void initialize_chat(char *chat_name);

/**
 * Add a new message to the chat.
 * @param new_message Should pointer to struct stored on heap.
 */
void append_message(struct message *new_message);

/**
 * Clears the chat locally. Used only when the user is exiting the chat (exiting the program)
 */
void clear_chat();

/**
 * Gain access to the entire chat log. Since the thread calling this now has ownership of the thread, it must also call
 * release_message_lock when done working with the chat log. Accessing the chat log should be read only.
 * @param first_message_buff
 * @param last_message_buff
 * @param message_length_buff
 */
void get_message_lock(struct message **first_message_buff, struct message **last_message_buff, size_t *message_length_buff);

/**
 * When you are done reading the chat log from get_message_lock, you must call this to relese the lock so other threads
 * can access it.
 */
void release_message_lock();

/**
 * Gets the number of messages sent.
 * @return number of messages sent.
 */
size_t get_message_length();

/**
 * Reads the content of a string containing the entire chat log into memory. See stringify_chat_log() to convert chat
 * to a string.
 */
char *parse_chat_log();

/**
 * Converts the entire chat log into a formatted string. To convert back to memory, use parse_chat_log(char *chat_log)
 * @return String representing the entire chat log.
 */
char *stringify_chat_log();
#endif