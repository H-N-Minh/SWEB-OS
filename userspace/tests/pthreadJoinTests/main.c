extern int pj1();
extern int pj2();
extern int pj3();
extern int pj4();
extern int pj5();
extern int pj6();

int main()
{
    pj1();      //pthread_join for function that has already finished 
    pj2();      //pthread_join where function is still running
    //pj3();      //starting 2000 threads after each other and join them - takes forever
    pj4();      //try to join the same thread twice
    pj5();
    pj6();
}

                
                            

                    