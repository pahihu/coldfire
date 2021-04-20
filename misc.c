/**********************************/
/*				*/
/*  Copyright 2002, David Grant   */
/*				*/
/*  see LICENSE for more details  */
/*				*/
/**********************************/

#include <stdio.h>
#include <string.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(misc);


u32 arg_split_chars(char **argv, char *buffer, int max_args, char *split)
{
	char *ptr = buffer;
	u32 argc = 0;
	TRACE("going to split '%s', max %d buffer\n", buffer, max_args);
	
	/* Still return a (which will be empty) if there are no buffer */
	if(!buffer) return 0;

	while(1) {
		/* Use up all the characters we don't want */
		while(strchr(split, *ptr) != NULL && *ptr != 0) {
			*ptr=0;
			ptr++;
		}
		
		if(*ptr == 0) return argc;

		/* Record the arg start position */
		argv[argc] = ptr;

		TRACE("found arg start= [%s]\n", ptr);
		
		/* Check for max args before quotes, everything goes
		 * into the last arg */
		if(max_args > 0 && argc == max_args-1) {
			char *end_ptr = &ptr[strlen(ptr)-1];
			if(*ptr=='"' && *end_ptr=='"') {
				*argv[argc]++ = 0;
				*end_ptr = 0;
			}
			break;
		}

		/* If the arg starts with a ", find the closing " */
		if(*ptr == '"') {
			*argv[argc]++ = 0;
			ptr = strchr(ptr+1, '"');
			*ptr++=0;
		} else {
			/* Advance to the next split char, or \0 */
			ptr = strpbrk(ptr, split);
			if(!ptr) break;
		}
		argc++;
	}

	argc++;
	return argc;
}


u32 arg_split(char **argv, char *buffer, int max_args) 
{
	return arg_split_chars(argv, buffer, max_args, " \t\n\r");
}

