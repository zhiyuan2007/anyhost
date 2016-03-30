#include"head.h"
#define N 15
int main(void) {
    AvlTree T=NULL;
    int i;
    int j = 0;
    T = MakeEmpty( NULL );
    for( i = 0; i < N; i++, j = ( j + 7 ) % 50 )
    {
        printf("j=%d \n",j);
        T = Insert( j, T );
    }
    puts("插入 4 \n");
    T = Insert( 4, T );
    Display(T);
   for( i = 0; i < N; i += 2 )
   {
       printf("delelte: %d \n",i);
        T = Delete( i, T );
   }
   printf("detele:\n");
   printf("height=%d \n",T->Height);
   Display(T);

    printf( "Min is %d, Max is %d\n", Retrieve( FindMin( T ) ),
               Retrieve( FindMax( T ) ) );
    return EXIT_SUCCESS;
}
