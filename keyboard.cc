#include "main.h"
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>

void *keyboard_thread(void *arg){
	char *inp = new char[512] ;
	memset(inp, '\0', 512) ;	
	while(1){
		printf("servant:PORT> ") ;
		fgets(inp, 511, stdin) ;



		memset(inp, '\0', 512) ;	
	}

	return 0;

}

