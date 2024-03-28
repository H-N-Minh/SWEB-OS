extern int pc1();
extern int pc2();
extern int pc3();
extern int pc4();
extern int pc5();

int main()
{
    pc1();  //sanity checks
    pc2();  //simple pthread_create and check if thread id gets set
    pc3();  //starting 250 threads
    pc4();  //check if to running threads have different id
    pc5();  //pthread create with simple argument


    //Todos: running 250 simultaniously
}
                    