#pragma once

//DO NOT CHANGE THE NAME OR THE TYPE OF THE user_progs VARIABLE!
char const *user_progs[] = {
// for reasons of automated testing
                            // "/usr/shell.sweb",

                            // "/usr/topG.sweb",               // Test top of stack that is reserved for userspace locking 
                            // "/usr/pc4.sweb",
                            // "/usr/smallpthreadcreate.sweb",
                            // "/usr/small_cancel_test.sweb",
                            // "/usr/stateTypeTest.sweb",
                            // "/usr/spinTest.sweb",
                            // "/usr/pthreadCreate.sweb",

// SEM & COND -------------------------------------------------
                            // "/usr/condTests.sweb",
                            // "/usr/semTests.sweb",

// FORK-------------------------------------------------
                            //"/usr/forkTests.sweb",


                        
                            //"/usr/notest.sweb",
                            //"/usr/shell.sweb",

                            "/usr/pthreadCreateTests.sweb",
                            "/usr/pthreadJoinTests.sweb",
                            "/usr/pthreadCancelTests.sweb",
                            "/usr/pthreadExitTests.sweb",
                            // "/usr/pthreadDetachTests.sweb",                                        

                            //"/usr/execTests.sweb",
                            // "/usr/exec3.sweb",           //Exec with many arguments
                            // "/usr/exec4.sweb",          //Exec with arguments
                            // "/usr/exec5.sweb",           //Exec without arguments
                            // "/usr/exec6.sweb",           //Exec with multiple threads

                            //"/usr/forkExecTests.sweb",  

                            // "/usr/userspaceLocksTests.sweb",                                    

                            // "/usr/sleepAndClockTests.sweb", 

                            0
                           };