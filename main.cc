#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int resetFlag = 0;

int usage()
{
	printf("Usage is: \"sv_node [-reset] <.ini file>\"\n");
	return false;
}

int processCommandLine(int argc, char *argv[])
{
int iniFlag=0;

if(argc<2 || argc > 3)
	return usage();

for(int i=1;i<argc;i++)
{
argv++;
	if(*argv[0]=='-')
	{
		if(strcmp(*argv,"-reset")!=0)
			return usage();
		else
			resetFlag = 1;
	}
	else
	{
		if(strstr(*argv, ".ini")==NULL)
			return usage();
		else
			iniFlag = 1;
	}
}
if(iniFlag!=1)
	return usage();
else
	return true;
}


int main(int argc, char *argv[])
{
if(!processCommandLine(argc, argv))
	exit(0);

return 0;
}
