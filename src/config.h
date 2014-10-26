#ifndef CONFIG_H_
#define CONFIG_H_

/*
 * Author: jrahm
 * created: 2014/10/24
 * config.h: <description>
 *
 * parse a very simple conf file:
 * var val
 */

typedef enum {
	BOOL, STRING, INT, COLOR
} types_t ;

typedef int (*conf_lambda_t)(void* closure, const char* key, const char* val);

/* Fold over the key value pairs found in a conf */
int read_config(const char* filename, conf_lambda_t f, void* closure) ;

#endif /* CONFIG_H_ */
