/*  Dowse - public header for libdowse functions
 *
 *  (c) Copyright 2016 Dyne.org foundation, Amsterdam
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
 */

#ifndef __LIBDOWSE_H__
#define __LIBDOWSE_H__

#include <hiredis/hiredis.h>
#include <database.h>

// logging channel singleton in libdowse
extern redisContext *log_redis;


// to quickly return 404
#define HTTP404 err("HTTP 404, %s:%u, %s()",	  \
                    __FILE__, __LINE__, __func__);	  \
	http_response(req, 404, NULL, 0); \
	return(KORE_RESULT_OK)

// for use in debugging
#define FLAG func("reached: %s:%u, %s()", __FILE__, __LINE__, __func__)

// log functions
void notice(const char *fmt, ...);

void warn(const char *fmt, ...);

void func(const char *fmt, ...);

void err(const char *fmt, ...);

void act(const char *fmt, ...);



// parsetime
int relative_time(char *utc, char *out);

#define REDIS_PORT 6379

#define MAX_OUTPUT 512

/* typedef struct redisReply { */
/*     int type;            // REDIS_REPLY */
/*     long long integer;  // The integer when type is REDIS_REPLY_INTEGER */
/*     int len;           // Length of string */
/*     char *str;        // Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
/*     size_t elements; // number of elements, for REDIS_REPLY_ARRAY */
/*     struct redisReply **element; // elements vector for REDIS_REPLY_ARRAY */
/* } redisReply; */
int okredis(redisContext *r, redisReply *res);

redisReply *cmd_redis(redisContext *redis, const char *format, ...);

redisContext *connect_redis(int db);

///////////
// hashmap

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL -2 	/* Hashmap is full */
#define MAP_OMEM -1 	/* Out of Memory */
#define MAP_OK 0 	/* OK */

/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void *any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer. Returns status code..
 */
typedef int (*PFany)(any_t, any_t);

/* */
#define INITIAL_SIZE (16)
#define MAX_CHAIN_LENGTH (8)

/* We need to keep keys and values */
typedef struct _hashmap_element{
	char* key;
	int in_use;
	any_t data;
} hashmap_element;

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _hashmap_map{
	int table_size;
	int size;
	hashmap_element *data;
} hashmap_map;

/*
 * map_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
/*typedef any_t map_t;*/

/* Nicola: it's beautiful typing . Type is your friend*/
typedef hashmap_map *map_t;

/*
 * Return an empty hashmap. Returns NULL if empty.
*/
extern map_t hashmap_new();

/*
 * Iteratively call f with argument (item, data) for
 * each element data in the hashmap. The function must
 * return a map status code. If it returns anything other
 * than MAP_OK the traversal is terminated. f must
 * not reenter any hashmap functions, or deadlock may arise.
 */
extern int hashmap_iterate(map_t in, PFany f, any_t item);

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
extern int hashmap_put(map_t in, char* key, any_t value);

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
extern int hashmap_get(map_t in, char* key, any_t *arg);

/*
 * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
extern int hashmap_remove(map_t in, char* key);

/*
 * Get any element. Return MAP_OK or MAP_MISSING.
 * remove - should the element be removed from the hashmap
 */
extern int hashmap_get_one(map_t in, any_t *arg, int remove);

/*
 * Free the hashmap
 */
extern void hashmap_free(map_t in);

/*
 * Get the current size of a hashmap
 */
extern int hashmap_length(map_t in);


#endif
