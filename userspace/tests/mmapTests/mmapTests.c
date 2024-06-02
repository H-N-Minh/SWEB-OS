#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


int main2() {
    int fd = open("usr/testmmn.txt", O_WRONLY | O_CREAT , 0644);


    // Write "hello world" to the file
    const char* message = "hello world";
    ssize_t bytes_written = write(fd, message, 20);


    // Close the file
    close(fd);

    fd = open("usr/testmmn.txt", O_RDONLY);


    // Read the content of the file
    char buffer[100];
    ssize_t bytes_read = read(fd, buffer, 20);

    buffer[bytes_read] = '\0';

    // Close the file
    close(fd);

    // Print the content of the file
    printf("Content of the file: %s\n", buffer);

    return 0;
}