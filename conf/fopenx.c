#include <errno.h>
#include <stdio.h>
int main(void) {
#if defined EEXIST && __STDC_VERSION__ >= 201112L
	FILE *f = fopen("fopenx.txt", "wbx");
	if(errno == EEXIST) return 0;
	if(f) fclose(f);
#endif
	return 1;
}
