extern int spin1();
extern int spin2();
extern int spin3();

extern int mutex1();
extern int mutex2();
extern int mutex3();

int main()
{
    // spin1();   //checks if basic locking and unlocking works with spinlock
    // spin2();   //simple syncronization with spinlock 
    // spin3();   //syncronization with two spinlocks

    mutex1();     //checks if basic locking and unlocking works with mutexes
    mutex2();     //simple syncronization with mutexes
    mutex3();     //syncronization with two mutexes
}
