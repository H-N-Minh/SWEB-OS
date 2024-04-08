extern int spin1();
extern int spin2();
extern int spin3();


int main()
{
    // spin1();   //Test: checks if basic locking and unlocking works with spinlock
    spin2();   //simple syncronization with spinlock 
    spin3();      //syncronization with two spinlocks

    // mutex1();
    // mutex2();
    // mutex3();
}
