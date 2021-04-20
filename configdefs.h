#ifndef CONFIGDEFS_H
#define CONFIGDEFS_H

#ifndef HAVE_SNPRINTF 
	#ifndef HAVE__SNPRINTF
		#define snprintf(S,N,F,X...) sprintf(S,F,X)
	#endif
#endif

#endif
