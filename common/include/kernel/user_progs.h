#pragma once

//DO NOT CHANGE THE NAME OR THE TYPE OF THE user_progs VARIABLE!
char const *user_progs[] = {
// for reasons of automated testing
                            "/usr/shell.sweb",
                            "/usr/forktests.sweb",
                            // "/usr/pc4.sweb",
//                            "/usr/smallpthreadcreate.sweb",
//                            "/usr/small_cancel_test.sweb",
//                            "/usr/stateTypeTest.sweb",
//                            "/usr/spinTest.sweb",
                        //    "/usr/fock.sweb",
                            // "/usr/pthreadCreate.sweb",

                            // pthread_create
                            
                            //"/usr/pc1.sweb",                      //sanity checks
                            //"/usr/pc2.sweb",                      //simple pthread_create and check if thread id gets set
                            //"/usr/pc3.sweb",                      //starting 250 threads
                           // "/usr/pc4.sweb",                      //check if to running threads have different id
                           // "/usr/pc5.sweb",                      //pthread create with simple argument
                            

                            // pthreadExit
                            
                           // "/usr/pe1.sweb",                      //pthreadExit in main
                            
                           // exit
                            
                           // "/usr/e1.sweb",                      //exit in main

                            // thread_join

                            
                           // "/usr/pj1.sweb",                       //pthread_join for function that has already finished
                           // "/usr/pj2.sweb",                       //pthread_join where function is still running
                           // "/usr/pj3.sweb",                       //starting 2000 threads after each other and join them - takes forever
                           // "/usr/pj4.sweb",                       //try to join the same thread twice
                            


                            //pthread_cancel
                            
                             //"/usr/pca1.sweb",                     //Cancel running thread
                             //"/usr/pca2.sweb",                     //Try to cancel already dead thread
                             //"/usr/pca3.sweb",                     //Deffered cancel in while should not work
                            
                            // multithreading
                            
                            //"/usr/p1.sweb",                       //pthread_create with pthread exit in function
                            //"/usr/p2.sweb",                       //thread that called join gets canceled the joined thread should not be detached
                            

                            //execv        
                            // "/usr/exec1.sweb",                  //exec without arguments
                            // "/usr/exec2.sweb",                  //exec with wrong path     

                            //fork
                            //"/usr/fork1.sweb",    

                            0
                           };