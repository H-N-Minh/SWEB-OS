#pragma once

//DO NOT CHANGE THE NAME OR THE TYPE OF THE user_progs VARIABLE!
char const *user_progs[] = {
// for reasons of automated testing
                              // "/usr/shell.sweb",
                          
// Threads -------------------------------------------------------------------
                            // "/usr/pthreadCreateTests.sweb",           //
                            // "/usr/pthreadJoinDetachTests.sweb",       //

                            // "/usr/pthreadCancelTests.sweb",
                            // "/usr/pthreadExitTests.sweb",

                            // "/usr/exitTests.sweb",

//Fork/Exec -------------------------------------------------------------------
                            // "/usr/waitpidTests.sweb",                //
                            // "/usr/forkTests.sweb",
                            // "/usr/execTests.sweb",                   //
                            // "/usr/forkExecTests.sweb",  

// UserspaceLocks ------------------------------------------------------------
                            // "/usr/userspaceLocksTests.sweb",
                            // "/usr/condTests.sweb",
                            // "/usr/semTests.sweb",

// FORK-------------------------------------------------
                            // "/usr/forkTests.sweb",


                        
                            //"/usr/notest.sweb",
                            //"/usr/shell.sweb",

                            "/usr/forklfds3.sweb",
                           
                          
                            // "/usr/pthreadCancelTests.sweb",
                            // "/usr/pthreadExitTests.sweb",
                            // "/usr/pthreadDetachTests.sweb",                                        

                            //"/usr/execTests.sweb",


                            //"/usr/forkExecTests.sweb",  

                          //  "/usr/userspaceLocksTests.sweb",

                            // "/usr/sleepAndClockTests.sweb",
//Other tests -----------------------------------------------------------------
                            // "/usr/sleepAndClockTests.sweb",          //
                            //"/usr/growingStackTests.sweb",
                            // "/usr/no_test.sweb",
                            // "/usr/topG.sweb",               // Test top of stack that is reserved for userspace locking 

//Exit !!!!!!! ------------------------
                            0
                           };