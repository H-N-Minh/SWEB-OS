extern int spin1();
extern int spin2();

int main()
{
    spin1();   //Test: checks if basic locking and unlocking works with spinlock
    spin2();   //simple syncronization with spinlock
}
