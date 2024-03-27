/*  Dowse - DNSCrypt proxy plugin for DNS management
 *
 *  (c) Copyright 2016-2024 Dyne.org foundation, Amsterdam
 *  Written by Denis Roio aka jaromil <jaromil@dyne.org>
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Public License as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This source code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * Please refer to the GNU Public License for more details.
 *
 * You should have received a copy of the GNU Public License along with
 * this source code; if not, write to:
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>

#include <dnscrypt/plugin.h>

#include <hiredis/hiredis.h>

#include <dnscrypt-dowse.h>


// Stores the trimmed input string into the given output buffer, which must be
// large enough to store the result.  If it is too small, the output is
// truncated.
size_t trim(char *out, size_t len, const char *str) {
	if(len == 0)
		return 0;

	const char *end;
	size_t out_size;

	// Trim leading space
	while(isspace(*str)) str++;

	if(*str == 0)  // All spaces?
	{
		*out = 0;
		return 1;
	}

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace(*end)) end--;
	end++;

	// Set output size to minimum of trimmed string length and buffer size minus 1
	out_size = (end - str) < len-1 ? (end - str) : len-1;

	// Copy trimmed string and add null terminator
	memcpy(out, str, out_size);
	out[out_size] = 0;

	return out_size;
}

void load_domainlist(plugin_data_t *data) {
	char line[MAX_LINE];
	char trimmed[MAX_LINE];
	DIR *listdir = 0;
	struct dirent *dp;
	FILE *fp;

	data->domainlist = hashmap_new();

	// parse all files in directory
	listdir = opendir(data->listpath);
	if(!listdir) {
		err("domainlist cannot open dir: %s",strerror(errno));
		exit(1); }

	// read file by file
	dp = readdir (listdir);
	while (dp) {
		char fullpath[MAX_LINE];
		snprintf(fullpath,MAX_LINE,"%s/%s",data->listpath,dp->d_name);
		// open and read line by line
		fp = fopen(fullpath,"r");
		if(!fp) {
      err("domainlist cannot open file %s: %s",fullpath,strerror(errno));
			continue; }
		while(fgets(line,MAX_LINE, fp)) {
			// save lines in hashmap with filename as value
			if(line[0]=='#') continue; // skip comments
			trim(trimmed, strlen(line), line);
			if(trimmed[0]=='\0') continue; // skip blank lines
			// logerr("(%u) %s\t%s", trimmed[0], trimmed, dp->d_name);

			// here valgrind complains about dereferencing but the
			// hashmap holds the addresses so it is not a problem
			hashmap_put(data->domainlist, strdup(trimmed), strdup(dp->d_name));
		}
		fclose(fp);
		dp = readdir (listdir);
	}
	closedir(listdir);
	fprintf(stderr,"size of parsed domain-list: %u\n", hashmap_length(data->domainlist));
}


char *extract_domain(plugin_data_t *data) {
	// extracts the last two or three strings of a dotted domain string

	int c;
	int dots = 0;
	int first = 1;
	char *last;
	int positions = 2; // minimum, can become three if sld too short
	int len;
	char address[MAX_QUERY];

	strncpy(address, data->query, MAX_QUERY);
	len = strlen(address);

	/* logerr("extract_domain: %s (%u)",address, len); */

	if(len<3) return(NULL); // a.i

	data->domain[len+1]='\0';
	for(c=len; c>=0; c--) {
		last=address+c;
		if(*last=='.') {
			dots++;
			// take the tld as first dot hits
			if(first) {
				strncpy(data->tld,last,MAX_TLD);
				first=0; }
		}
		if(dots>=positions) {
			char *test = strtok(last+1,".");
			if( strlen(test) > 3 ) break; // its not a short SLD
			else positions++;
		}
		data->domain[c]=*last;
	}

	// logerr("extracted: %s (%p) (dots: %u)", domain+c+1, domain+c+1, dots);

	return(data->domain+c+1);
}


// free all buffers of a loaded domainlist
int free_domainlist_f(any_t arg, any_t element) {
	free(element);
	return MAP_OK; }

void free_domainlist(plugin_data_t *data) {
	if(!data) {
		fprintf(stderr, "ERROR: data NULL in domainlist\n");
		return;
	}

	hashmap_iterate(data->domainlist, free_domainlist_f, NULL);
	hashmap_free(data->domainlist);
}
