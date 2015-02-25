/* ready.c - ready */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>

/*------------------------------------------------------------------------
 * ready  --  make a process eligible for CPU service
 *------------------------------------------------------------------------
 */
int ready(int pid, int resch)
{
	register struct	pentry	*pptr;

	if (isbadpid(pid))
		return(SYSERR);
	pptr = &proctab[pid];
	pptr->pstate = PRREADY;
/*
 * PA1
 * Since processes are to be dequeued in the order
 * they came in to ready list for round-robin, equal key for all processes.
 */
	if(pptr->real == 1)
		insert(pid,realrdyhead,1);
	else
		insert(pid,rdyhead,pptr->pprio);
	if (resch)
		resched();
	return(OK);
}
