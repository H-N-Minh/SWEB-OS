#pragma once

//DO NOT CHANGE THE NAME OR THE TYPE OF THE user_progs VARIABLE!
char const *user_progs[] = {
// for reasons of automated testing
                             //"/usr/shell.sweb",

                            // pthread_create
                            /*
                            "/usr/pc1.sweb",                      //sanity checks
                            "/usr/pc2.sweb",                      //simple pthread_create and check if thread id gets set
                            "/usr/pc3.sweb",                      //starting 250 threads
                            "/usr/pc4.sweb",                      //check if to running threads have different id
                            "/usr/pc5.sweb",                      //pthread create with simple argument
                            */

                            // pthread_exit
                            /*
                            "/usr/pe1.sweb",                      //pthread_exit in main
                            */

                            // pthread_create and pthread_exit
                            /*
                            "/usr/p1.sweb",                       //pthread_create with pthread exit in function
                            */

                            // thread_join

                            /*
                            "/usr/pj1.sweb",                       //pthread_join for function that has already finished
                            "/usr/pj2.sweb",                       //pthread_join where function is still running
                            "/usr/pj3.sweb",                       //starting 2000 threads after each other and join them - tekes forever
                            */


                            //pthread_cancel
                            /*
                             "/usr/pca1.sweb",
                             */
                             "/usr/pca2.sweb",

                            


                            // "/usr/exec1.sweb",
                            // "/usr/exec2.sweb",             

                            0
                           };


