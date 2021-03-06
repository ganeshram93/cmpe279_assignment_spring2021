/* Server side C/C++ program to demonstrate Socket programming */

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>

/* Global Macro Definitions */
#define PORT 80
#define BUFFER_LEN 1024
#define NEWLY_CREATED_USER "user1"

/* Function Declarations */
int socket_init(struct sockaddr_in *, int *, int *);
int child_process_data(struct sockaddr_in, int, int);

int socket_init(struct sockaddr_in *address, int *server_fd, int *addrlen)
{
    int realuid = 0, euid = 0, realgid = 0, egid = 0, opt = 1;
    /* Creating socket file descriptor */
    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /* Attaching socket to port 80 */
    if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    /* Forcefully attaching socket to the port 80 */

    if (bind(*server_fd, (struct sockaddr *)&(*address),
                                 sizeof(*address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(*server_fd, 3) < 0)
    {
        perror("Error: Unable to listen. Exiting");
        exit(EXIT_FAILURE);
    }

    printf("=====In parent process=====\n");
    realuid = getuid();
    euid = geteuid();
    realgid = getgid();
    egid = getegid();
    printf("Parent Real UID: %d\n", realuid);
    printf("Parent Effective UID: %d\n", euid);
    printf("Parent Real GID: %d\n", realgid);
    printf("Parent Effective GID: %d\n", egid);

    return 0;
}

int child_process_data(struct sockaddr_in address, int addrlen, int server_fd)
{
    struct group *grp = getgrnam(NEWLY_CREATED_USER);
    struct passwd *pwd = getpwnam(NEWLY_CREATED_USER);
    int new_socket, valread, groupid, userid;
    int realuid = 0, euid = 0, realgid = 0, egid = 0;
    char *hello = "Hello from server";
    char buffer[BUFFER_LEN] = {0};

    if (grp == NULL)
    {
       printf("Error: No group ID found for user %s. Exiting\n", NEWLY_CREATED_USER);
       exit(EXIT_FAILURE);
    }
    if (pwd == NULL)
    {
       printf("Error: No user ID found for user %s. Exiting\n", NEWLY_CREATED_USER);
       exit(EXIT_FAILURE);
    }

    groupid = grp->gr_gid;
    userid = pwd->pw_uid;

    printf("\n=====In child process=====\n");
    printf("GID = %d | UID = %d\n", groupid, userid);

    /* Drop privileges by running the process with given GID and UID */
    if (setgid(groupid) != 0)
    {
        printf("Error: setgid failed. Unable to drop group privileges. Exiting\n");
        exit(EXIT_FAILURE);
    }
    if (setuid(userid) != 0)
    {
        printf("Error: setuid failed. Unable to drop group privileges. Exiting\n");
        exit(EXIT_FAILURE);
    }

    realuid = getuid();
    euid = geteuid();
    realgid = getgid();
    egid = getegid();
    printf("Child Real UID: %d\n", realuid);
    printf("Child Effective UID: %d\n", euid);
    printf("Child Real GID: %d\n", realgid);
    printf("Child Effective GID: %d\n", egid);

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    printf("\nReceived message from socket:");
    valread = read(new_socket, buffer, BUFFER_LEN);
    printf("\n%s\n", buffer);
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");
    exit(0);
}

int main(int argc, char const *argv[])
{
    struct sockaddr_in address;
    int server_fd, addrlen = sizeof(address);
    int status = 0;
    pid_t process_pid, wait_pid;

    printf("execve=0x%p\n", execve);

    socket_init(&address, &server_fd, &addrlen);

    /* Located the split: Data processing logic begins below */
    if ((process_pid = fork()) < 0)
    {
        printf("Error: Unable to fork\n");
    }

    if(process_pid == 0)
    {
        child_process_data(address, addrlen, server_fd);
    }

    /* Wait for child process to exit */
    while ((wait_pid = wait(&status)) > 0);
    printf("\nChild process has exited\n");
    return 0;
}
