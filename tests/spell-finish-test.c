/* codespell reads a three letter local in the Finish handler as
 * the word and. That local must be gone. */

#include <stdio.h>
#include <string.h>

#ifndef SOURCE_FILE
#error SOURCE_FILE
#endif

int main(void)
{
	char line[1024];
	char needle[64];
	FILE *f = fopen(SOURCE_FILE, "r");

	if (!f) {
		perror(SOURCE_FILE);
		return 2;
	}
	snprintf(needle, sizeof needle, "struct capn_rpc_answer *%s%s", "an", "s");
	while (fgets(line, sizeof line, f)) {
		char *hit = strstr(line, needle);
		char after;

		if (hit == NULL)
			continue;
		after = hit[strlen(needle)];
		if (after == '\0' || after == ';' || after == ',' ||
		    after == ')' || after == ' ' || after == '\t' ||
		    after == '\n') {
			fclose(f);
			return 1;
		}
	}
	fclose(f);
	return 0;
}
