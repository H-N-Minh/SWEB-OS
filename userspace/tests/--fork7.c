/*
--- # Test specification
category: base
description: "getThreadCount"

expect_exit_codes: [0, 69]
disabled: false
*/


#include "stdio.h"
#include "types.h"
#include "unistd.h"
#include "pthread.h"
#include "assert.h"
#include "sched.h"
#include "wait.h"

#define PARENT_SUCCESS7 0    // parent process returns 0 on success
#define CHILD_SUCCESS7 69    // child process returns 69 on success

// Test fork with waitpid
int main()
{
    int status0;
    pid_t pid0 = fork();
    if (pid0 == 0) {
        int status1;
        pid_t pid1 = fork();
        if (pid1 == 0) {
            int status2;
            pid_t pid2 = fork();
            if (pid2 == 0) {
                int status3;
                pid_t pid3 = fork();
                if (pid3 == 0) {
                    int status4;
                    pid_t pid4 = fork();
                    if (pid4 == 0) {
                        int status5;
                        pid_t pid5 = fork();
                        if (pid5 == 0) {
                            int status6;
                            pid_t pid6 = fork();
                            if (pid6 == 0) {
                                int status7;
                                pid_t pid7 = fork();
                                if (pid7 == 0) {
                                    int status8;
                                    pid_t pid8 = fork();
                                    if (pid8 == 0) {
                                        int status9;
                                        pid_t pid9 = fork();
                                        if (pid9 == 0) {
                                            int status10;
                                            pid_t pid10 = fork();
                                            if (pid10 == 0) {
                                                exit(CHILD_SUCCESS7);
                                            }
                                            pid_t retval10 = waitpid(pid10, &status10, 0);
                                            if (retval10 < 0)
                                            {
                                                exit(-1);
                                            }
                                            exit(status10);
                                        }
                                        pid_t retval9 = waitpid(pid9, &status9, 0);
                                        if (retval9 < 0)
                                        {
                                            exit(-1);
                                        }
                                        exit(status9);
                                    }
                                    pid_t retval8 = waitpid(pid8, &status8, 0);
                                    if (retval8 < 0)
                                    {
                                        exit(-1);
                                    }
                                    exit(status8);
                                }
                                pid_t retval7 = waitpid(pid7, &status7, 0);
                                if (retval7 < 0)
                                {
                                    exit(-1);
                                }
                                exit(status7);
                            }
                            pid_t retval6 = waitpid(pid6, &status6, 0);
                            if (retval6 < 0)
                            {
                                exit(-1);
                            }
                            exit(status6);
                        }
                        pid_t retval5 = waitpid(pid5, &status5, 0);
                        if (retval5 < 0)
                        {
                            exit(-1);
                        }
                        exit(status5);
                    }
                    pid_t retval4 = waitpid(pid4, &status4, 0);
                    if (retval4 < 0)
                    {
                        exit(-1);
                    }
                    exit(status4);
                }
                pid_t retval3 = waitpid(pid3, &status3, 0);
                if (retval3 < 0)
                {
                    exit(-1);
                }
                exit(status3);
            }
            pid_t retval2 = waitpid(pid2, &status2, 0);
            if (retval2 < 0)
            {
                exit(-1);
            }
            exit(status2);
        }
        pid_t retval1 = waitpid(pid1, &status1, 0);
        if (retval1 < 0)
        {
            exit(-1);
        }
        exit(status1);
    }
    pid_t retval0 = waitpid(pid0, &status0, 0);
    if (retval0 < 0)
    {
        return -1;
    }
    
    if (status0 == CHILD_SUCCESS7)
    {
        return PARENT_SUCCESS7;
    }
    
    return -1;
}