/*
 * file:		include/share/ctype.h
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_CTYPE_H
#define _SHARE_CTYPE_H

int isspace(char c);
int isalnum(int ch);
int isxdigit (int c);
int isdigit( int ch );
unsigned long strtoul(const char *cp,char **endp,unsigned int base);
long strtol(const char *cp,char **endp,unsigned int base);
int isalpha(int ch);
double strtod(const char* s, char** endptr);
double atof(char *str);

#endif  /* _SHARE_CTYPE_H */
