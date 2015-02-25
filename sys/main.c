/*
 * main.c
 *
 *  Created on: Feb 11, 2015
 *      Author: Satish
 */

#include <kernel.h>
#include <stdio.h>
#include <lab1.h>
#define LOOP 50


int prA, prB, prC, prD, prE, prF, prG, prH, prI;
int proc(char c);

int main() {
	int i;
	int count = 0;
	char buf[8];

	for (i = 0; i < 10; i++) {
		srand(i);
		printf("%d\n", rand());
	}

//	setschedclass(LINUXSCHED);
	setschedclass(MULTIQSCHED);
	resume(prA = create(proc, 2000, 5, "proc A", 1, 'A'));
	resume(prD = createReal(proc, 2000, 5, "proc D", 1, 'D'));
	resume(prB = create(proc, 2000, 50, "proc B", 1, 'B'));
	resume(prE = createReal(proc, 2000, 5, "proc E", 1, 'E'));
	resume(prC = create(proc, 2000, 90, "proc C", 1, 'C'));
	resume(prF = createReal(proc, 2000, 5, "proc F", 1, 'F'));
	while (count++ < LOOP) {
		kprintf("M");
		for (i = 0; i < 10000000; i++)
			;
		if (count == 20) {
			kprintf("\n");
			chprio(48, 95);
		}
		if (count == 5) {
			kprintf("\n");
			resume(prG = create(proc, 2000, 50, "proc G", 1, 'G'));
			resume(prH = createReal(proc, 2000, 20, "proc H", 1, 'H'));
			resume(prI = createReal(proc, 2000, 20, "proc I", 1, 'I'));
		}
	}
	return 0;
}
/*
int prA, prB, prC, prD, prE, prF;
int proc(char c);

int main() {
	int i;
	int count = 0;
	char buf[8];

	for (i = 0; i < 10; i++) {
		srand(i);
		printf("%d\n", rand());
	}

	setschedclass(LINUXSCHED);
	resume(prB = create(proc, 2000, 50, "proc B", 1, 'B'));
	resume(prC = create(proc, 2000, 90, "proc C", 1, 'C'));
	resume(prF = create(proc, 2000, 5, "proc F", 1, 'F'));
	while (count++ < LOOP) {
		kprintf("M");
		for (i = 0; i < 10000000; i++)
			;
	}
	return 0;
}*/

int proc(char c) {
	int i;
	int count = 0;

	while (count++ < LOOP) {
		kprintf("%c", c);
		for (i = 0; i < 10000000; i++)
			;
	}
	return 0;
}

