/**********************************/
/*                                */
/*  Copyright 2000, 2001 David Grant   */
/*                                */
/*  see LICENSE for more details  */
/*                                */
/**********************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "coldfire.h"

TRACER_DEFAULT_CHANNEL(monitor);

int Monitor_SET(int argc, char **argv)
{
	char cmd[16]="", value[8]="";


	if(argc==1) {
		/* Show compatability */
		printf("dBUG output compatability: %s\n", 
			monitor_config.dbug_compatibility == 1 ? 
			"ON" : "OFF");
		printf("Disassemble lines: %d\n", 
			monitor_config.disassemble_lines);
		return 1;
	}

	if(argc != 3) {
		Monitor_show_help("set");
		return 1;
	}
	sscanf(argv[1], "%s", cmd);
	sscanf(argv[2], "%s", value);

	if(strcasecmp(cmd, "compat")==0) {
		if(strcasecmp(value,"off") == 0 || strcasecmp(value,"0") == 0 ||
	   	   strcasecmp(value,"no") == 0) {
			monitor_config.dbug_compatibility = 0; 
		} else {
			monitor_config.dbug_compatibility = 1;
		} 
		printf("dBUG output compatability has been turned %s\n", 
			monitor_config.dbug_compatibility == 1 ?  "ON" : "OFF");
	} else if(strcasecmp(cmd, "dislen")==0) {
		sscanf(value, "%d", &monitor_config.disassemble_lines);
		printf("Disassemble lines set to %d lines\n",
			monitor_config.disassemble_lines);
	}
	return 1;
}
