#ifndef FIXED_H
#define FIXED_H

#include <stdint.h>

/* Used the table found out the site below as a resource for the following functions */
/* https://jeason.gitbooks.io/pintos-reference-guide-sysu/content/Advanced%20Scheduler.html */

#define F 17

#define fixedToInt_RoundNearest(x)(x >= ? ((x + (1 << (F - 1))) >> F) \
										: ((x - (1 << (F - 1))) >> F))

/* x & y = Fixed Point Numbers */
/* n = Integer */

/* Convert n to Fixed n*f */
int intToFixed(int n);

/* Convert x to int - Rounding to Zero x/f */
int fixedToInt_RoundZero(int x);

/* Add x and y */
int addFixedFixed(int x, int y);

/* Add x and n */
int addFixedInt(int x, int n);

/* Subtract y from x */
int subFixedFixed(int x, int y);

/* Subtract n from x */
int subFixedInt(int x, int n);

/* Multiply x by y */
int mulFixedFixed(int x, int y);

/* Multiply x by n */
int mulFixedInt(int x, int n);

/* Divide x by y */
int divFixedFixed(int x, int y);

/* Divide x by n */
int divFixedInt(int x, int n);


int intToFixed(int n)
{
	return n << F;
}

int fixedToInt_RoundZero(int x)
{
	return x >> F;
}

int addFixedFixed(int x, int y)
{
	return x + y;
}

int addFixedInt(int x, int n)
{
	return x + (n << F);
}
int subFixedFixed(int x, int y)
{
	return x - y;
}

int subFixedInt(int x, int n)
{
	return x - (n << F);
}

int mulFixedFixed(int x, int y)
{
	return ((int64_t)x) * (y >> F);
}

int mulFixedInt(int x, int n)
{
	return x * n;
}

int divFixedFixed(int x, int y)
{
	return (((int64_t)x) << F) / y;
}

int divFixedInt(int x, int n)
{
	return x/n;
}


#endif
