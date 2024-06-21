#pragma once

//DO NOT CHANGE THE NAME OR THE TYPE OF THE user_progs VARIABLE!
char const *user_progs[] = {
// for reasons of automated testing
//                               "/usr/shell.sweb",

//mmap-----------------------------------------------------------------------
//                             "/usr/mmapPrivateTests.sweb",
//                             "/usr/mmapSharedTests.sweb", //
//                             "usr//shmTest.sweb",

//Swapping-------------------------------------------------------------------

//                           "usr/pageSelectionTests.sweb",
                            // "/usr/praTests.sweb",  //(basically swapping tests)
                            // "/usr/SwappingForkTests.sweb",

                            // "usr/NFUBetterThanRandom.sweb",
                            // "usr/SecondChangeBetterThanRandom.sweb",

//                              "/usr/execTests.sweb",     //Changed so that the use swapping
//                              "/usr/forkTests.sweb",     //When you run all some of the use swapping as well

                            // "usr/randomTest.sweb",
                            

// Threads -------------------------------------------------------------------
                            // "/usr/pthreadCreateTests.sweb",
                            // "/usr/pthreadJoinDetachTests.sweb", 

                            // "/usr/pthreadCancelTests.sweb",
                            // "/usr/pthreadExitTests.sweb",

                            // "/usr/exitTests.sweb",

                            // "/usr/exitSleep.sweb",
                            // "/usr/pe1.sweb",


//Fork/Exec -------------------------------------------------------------------
                            // "/usr/waitpidTests.sweb",                
                            // "/usr/forkTests.sweb",

                            // "/usr/fork7.sweb",
                            // "/usr/fork8.sweb",
                            // "/usr/fork9.sweb",
                            // "/usr/fork10.sweb",


                           
                           

// UserSpace Memory -------------------------------------------------
                            // "/usr/growingStackTests.sweb",
                            // "/usr/mallocTests.sweb",
                            // "/usr/reallocTests.sweb",

// Userspace Locking -------------------------------------------------
                            // "/usr/condTests.sweb",
                            // "/usr/semTests.sweb",
                            // "/usr/userspaceLocksTests.sweb",

//Other tests -----------------------------------------------------------------
                            // "/usr/sleepAndClockTests.sweb",   
                            // "/usr/localFdTests.sweb",
                            // "/usr/pipeTest.sweb",  
                            // "/usr/fdsPipe.sweb",     
                            // "/usr/fdsPipe2.sweb", 
                            // "/usr/fdsPipe3.sweb",
                            // "/usr/fdsPipe4.sweb",
                            // "/usr/largePipe.sweb",
                            //  "/usr/dupTest.sweb",
                            // "/usr/dupInvalidTest.sweb",
                            // "/usr/no_test.sweb",
                            0
                           };
