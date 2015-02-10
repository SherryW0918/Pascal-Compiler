/* lexan.h            Gordon S. Novak Jr.           09 Feb 01 */

/* Definitions for lexical analyzer */

/* Copyright (c) 2001 Gordon S. Novak Jr. and
   The University of Texas at Austin. */

/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License (file gpl.text) for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */
#include <stdbool.h>

#define MAXCHARCLASS 256

#define ALPHA   1
#define NUMERIC 2
#define SPECIAL 3

#define DEBUGGETTOKEN 0

#define MAX_STRING_LEN 15
#define NUM_RESERVED_WORDS 29
#define NUM_OPERATORS 6
#define NUM_SPECIAL_OPERATORS 13
#define NUM_DELIMETERS 8
#define MAX_NUM_SIZE 9999
#define MAX_DIGITS 8
#define MAX_INT 2147483647LL
#define MIN_INT -2147483648LL
#define MAX_FLOAT 3.402823E+38
#define MIN_FLOAT 1.175495E-38 
#define POS true
#define NEG false

TOKEN talloc();
int peekchar();
int peek2char();
int peeknchar();
void init_charclass();
TOKEN gettoken();
void printtoken(TOKEN tok);

void skipblanks ();
void initscanner ();
int EOFFLG;
int CHARCLASS[MAXCHARCLASS];
TOKEN identifier (TOKEN tok);
TOKEN getstring (TOKEN tok);
TOKEN special (TOKEN tok);
TOKEN number (TOKEN tok);

void getWholeString(char * buf, int len, bool worded);
bool isWhiteSpace(char c);
int getInt();
void consumeToBlank();
void consumeString();
bool isEndNum(int c);
void buildNumber(char * numArr, int * trailDigs, int * leadingDigs);
void buildExponent(char * expArr);
double toDouble(char * str);
int toInt(char * str, bool print);