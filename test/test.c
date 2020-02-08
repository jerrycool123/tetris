#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

int calc(char a[10])  
{  
    printf("%d\n",strlen(a));  
    return 0;  
}  
  
int main()  
{  
    char a[10]={"hello"};  
    calc(a);  
    return 0;  
}  	
