#include "common.h"
#include "string.h"

int
main(int argc, char *argv[])
{
	int val;
	int factorial = 1;
	for(int i = 1; i < argc; i++) {
		for(int j = 0; j < strlen(argv[i]); j++) {
			if(argv[i][j] == '.') {
				val = 0;
			}
		}
	}
	if(val != 0) {
		val = atoi(argv[1]);
	}
	if(val > 12) {
		printf("Overflow\n");
	}
	else if(val > 0 && val <= 12) {
		for(int i = 1; i <= val; i++) {
			factorial = factorial * i;
		}
		printf("%d\n", factorial);
	}
	else {
		printf("Huh?\n");
	}
	return 0;
}
