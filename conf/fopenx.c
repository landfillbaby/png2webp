#include <errno.h>
#include <stdio.h>
int main(void) {
#if defined EEXIST && __STDC_VERSION__ >= 201112L
	FILE *const f = fopen("fopenx.txt", "wbx");
	if(f) fclose(f);
	else if(errno == EEXIST) return 0;
#endif
	return 1;
}
