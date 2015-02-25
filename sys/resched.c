/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lab1.h>
#define NEWEPOCH 2
int scheduled_class = 0;
unsigned long currSP; /* REAL sp of current process */

void setschedclass(int);
int getschedclass();

extern int ctxsw(int, int, int, int);
/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched() {
	switch (getschedclass()) {
	case LINUXSCHED:
		return Linux_22_Scheduling();
	case MULTIQSCHED:
		return Multiqueue_Scheduling();
	default:
		return Xinu_Default_Scheduling();
	}
	return OK;

}

void setschedclass(int sched_class) {
	scheduled_class = sched_class;
}

int getschedclass() {
	return scheduled_class;
}

int Multiqueue_Scheduling() {

	register struct pentry *optr; /* pointer to old process entry */
	register struct pentry *nptr; /* pointer to new process entry */
	int retVal = SYSERR;
	optr = &proctab[currpid];
	if (currpid == NULLPROC) { /* If the current process running is null process, then newEpoch has to begin*/
		if (useReal() == TRUE) {
			if (!isempty(realrdyhead))
				return RealTimeProcess_Scheduling(TRUE, optr, nptr);
			else
				return NormalQueue_Scheduling(TRUE, optr, nptr);
		} else
			return NormalQueue_Scheduling(TRUE, optr, nptr);
	}
	if (optr->real == FALSE) {
		/* If current process is normal process, then
		 * it can be epoch that is scheduling normal processes only or this can be last process in this epoch of normal processes
		 * If it returns NEWEPOCH, then randomNumber is generated and based on that either Real-time queue (if it is not empty)
		 * or Normal processes queue is scheduled.
		 * Similarly in case of current process being real.
		 * */
		retVal = NormalQueue_Scheduling(FALSE, optr, nptr);
		if (retVal != NEWEPOCH) { //If 2, it is newEpoch
			return retVal;
		} else { //New Epoch
			if (useReal() == FALSE) {
				return NormalQueue_Scheduling(TRUE, optr, nptr);
			} else {
				if (!isempty(realrdyhead))
					return RealTimeProcess_Scheduling(TRUE, optr, nptr);
				else
					return NormalQueue_Scheduling(TRUE, optr, nptr);
			}
		}
	} else if (optr->real == TRUE) { //
		retVal = RealTimeProcess_Scheduling(FALSE, optr, nptr);
		if (retVal != NEWEPOCH) {
			return retVal;
		} else {
			if (useReal() == TRUE) {
				if (!isempty(realrdyhead) || (isempty(realrdyhead)==TRUE && optr->pstate==PRCURR))
					return RealTimeProcess_Scheduling(TRUE, optr, nptr);
				else
					return NormalQueue_Scheduling(TRUE, optr, nptr);
			} else
				return NormalQueue_Scheduling(TRUE, optr, nptr);
		}
	}
	return retVal;
}

int RealTimeProcess_Scheduling(int newEpoch, struct pentry *optr, struct pentry *nptr) {

	optr->quantumleft = preempt;
	if (optr->quantumleft <= 0)
		optr->quantumleft = 0;

	if (newEpoch == TRUE) {
		int i = 0;
		struct pentry *p;
		for (i = 0; i < NPROC; i++) {
			p = &proctab[i];
			if (p->pstate == PRFREE || p->real == FALSE)
				continue;
			else {
				p->quantum = 100;
				p->quantumleft = p->quantum;
			}
		}
	}

	if (optr->real == TRUE && optr->quantumleft > 0 && optr->pstate == PRCURR && ((newEpoch == FALSE) || isempty(realrdyhead))) {
		/*
		 * Current process has quantum left and wants to be in PCURR state
		 * Or
		 * It is a newEpoch, then as per Round-Robin, next process must be selected not optr again.
		 * But if this is the only process, then it has to be chosen.
		 * Also, if it is newEpoch it definitely means current process does not want to return.
		 */
		preempt = optr->quantumleft;
		return OK;
	} else if (optr->pstate != PRCURR || optr->quantumleft <= 0 || optr->real == FALSE || (!isempty(realrdyhead) && newEpoch == TRUE && optr->quantumleft > 0 && optr->pstate == PRCURR)) {
		/*
		 * Currpid does not want to be PRCURR state any more or its quantum exhausted for this epoch.
		 * Also, currpid want to execute but RoundRobin-wise next process is to be selected
		 */
		if (isempty(realrdyhead))
			return NEWEPOCH;
		else {
			/*
			 * Choosing next process with quantumleft > 0
			 */
			int newprocPID = 0;
			int k = 0;
			for (k = q[realrdytail].qprev; k != realrdyhead; k = q[k].qprev) {
				if (proctab[k].quantumleft > 0) {
					newprocPID = k;
					break;
				}
			}
			if (newprocPID != realrdyhead) {
				if (optr->real == TRUE && optr->pstate == PRCURR) { //Just like in Xinu Scheduling
					optr->pstate = PRREADY;
					insert(currpid, realrdyhead, 1);
				} else if (optr->real == FALSE && optr->pstate == PRCURR) {
					optr->pstate = PRREADY;
					insert(currpid, rdyhead, optr->pprio);
				}
				nptr = &proctab[newprocPID];
				nptr->pstate = PRCURR;
				dequeue(newprocPID);
				currpid = newprocPID;
				preempt = nptr->quantumleft;
				ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
				return OK;
			} else
				return NEWEPOCH;
		}
	}
	return SYSERR;
}

int useReal() {
	static int randomNumber = 0;
	srand(randomNumber);
	randomNumber++;
	int x = rand() % 10;
	if (x >= 0 && x <= 6) {
		return TRUE;
	} else if (x >= 7 && x <= 9)
		return FALSE;
	return TRUE;
}

int NormalQueue_Scheduling(int newEpoch, struct pentry *optr, struct pentry *nptr) {

	if (newEpoch) {
		int i = 0;
		struct pentry *p;
		for (i = 0; i < NPROC; i++) {
			p = &proctab[i];
			if (p->pstate == PRFREE || p->real == TRUE)
				continue;
			else if (p->quantumleft == 0 || p->quantumleft == p->quantum) { //Process either exhausted or not run at all;
				p->quantum = p->pprio;
			} else {
				p->quantum = (p->quantumleft) / 2 + p->pprio;
			}
			p->quantumleft = p->quantum;
			p->goodness = p->quantumleft + p->pprio;
		}
		preempt = optr->quantumleft;
	}
	int optrOldPrio = optr->goodness - optr->quantumleft; //Even If optr's prio has been changed, difference gives old priority
	optr->goodness = optrOldPrio + preempt;
	optr->quantumleft = preempt;

	if (optr->quantumleft <= 0) {
		optr->quantumleft = 0; //No more CPU time left
		optr->goodness = 0; //Exhausted allocated CPU time
	}

	if (optr->real == TRUE)
		optr->goodness = 0;
	//Find, of all runnable processes which has highest goodness
	int maxGoodness = 0;
	int k = 0;
	int newprocPID = 0;
	for (k = q[rdyhead].qnext; k != rdytail; k = q[k].qnext) {
		if (proctab[k].goodness > maxGoodness) {
			newprocPID = k;
			maxGoodness = proctab[k].goodness;
		}
	}

	if (maxGoodness == 0 && (optr->pstate != PRCURR || optr->quantumleft == 0)) {
		/* NEW EPOCH
		 * MaxGoodness is zero, it means processes in readylist
		 * have exhausted CPU and new epoch has to begin
		 * For Current process, if counter = 0, exhausted CPU, forced to next epoch
		 * If it does not want to stay PRCURR despite CPU time, next epoch*/
		if (newEpoch == TRUE && isempty(realrdyhead)) {
			if (optr->real == TRUE && (optr->pstate == PRCURR || optr->pstate == PRREADY)) {
				return NEWEPOCH;
			} else if (currpid == NULLPROC) {
//				kprintf("\tNUM OF USER PROCESSES : %d \n", numproc);
				return OK;
			} else {
				newprocPID = NULLPROC;
				if (optr->real == FALSE && optr->pstate == PRCURR) {
					optr->pstate = PRREADY;
					insert(currpid, rdyhead, optr->pprio);
				} else if (optr->real == TRUE && optr->pstate == PRCURR) {
					optr->pstate = PRREADY;
					insert(currpid, realrdyhead, 1);
				}
				//kprintf("\t switched to Null process\t");
				nptr = &proctab[newprocPID];
				nptr->pstate = PRCURR;
				dequeue(newprocPID);
				currpid = newprocPID;
#ifdef	RTCLOCK
				preempt = QUANTUM; //reset preemption counter
#endif
				ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
				return OK;
			}
		} else
			return NEWEPOCH;
	} else if (optr->real == FALSE && optr->goodness >= maxGoodness && optr->goodness > 0 && optr->pstate == PRCURR) {

		/*
		 * Current process has maximum goodness,
		 * Its goodness is greater than 0.
		 * Currpid wants to be CURR state.
		 * In the same epoch, execute till its quantumleft exhausts i.e, reduces to 0.
		 */
		preempt = optr->quantumleft;
		return OK;
	} else if (maxGoodness > 0 && (optr->pstate != PRCURR || optr->quantumleft == 0 || optr->goodness < maxGoodness)) {
		/*
		 * Currpid does not want to be PRCURR state any more even though it has more goodness and CPU time
		 * OR, Exhausted allocated CPU time, but wants to be PRCURR and has maxGoodness
		 * OR, Its goodness is less than maxGoodness, and has CPU time and wants to be PRCURR
		 * Process must be rescheduled and next process must begin.
		 */
		if (optr->real == FALSE && optr->pstate == PRCURR) { //Just like in Xinu Scheduling
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		} else if (optr->real == TRUE && optr->pstate == PRCURR) { //Just like in Xinu Scheduling
			optr->pstate = PRREADY;
			insert(currpid, realrdyhead, 1);
		}
		nptr = &proctab[newprocPID];
		nptr->pstate = PRCURR;
		dequeue(newprocPID);
		currpid = newprocPID;
		preempt = nptr->quantumleft;
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		return OK;
	} else
		return SYSERR;
}

int Linux_22_Scheduling() {
	/*
	 * preempt = no. of clock ticks remaining
	 */
	register struct pentry *optr; /* pointer to old process entry */
	register struct pentry *nptr; /* pointer to new process entry */
	int newEpoch = FALSE;
	choose: optr = &proctab[currpid];
	int optrOldPrio = optr->goodness - optr->quantumleft; //Even If optr's prio has been changed, difference gives old priority
	optr->goodness = optrOldPrio + preempt;
	optr->quantumleft = preempt;

	if (optr->quantumleft <= 0 || currpid == NULLPROC) {
		optr->quantumleft = 0; //No more CPU time left
		optr->goodness = 0; //Exhausted allocated CPU time
	}
	//Find, of all runnable processes which has highest goodness
	int maxGoodness = 0;
	int k = 0;
	int newprocPID = 0;
	for (k = q[rdytail].qprev; k != rdyhead; k = q[k].qprev) {
		if (proctab[k].goodness > maxGoodness) {
			newprocPID = k;
			maxGoodness = proctab[k].goodness;
		}
	}

	if (maxGoodness == 0 && (optr->pstate != PRCURR || optr->quantumleft == 0)) {
		/*
		 * NEW EPOCH
		 * MaxGoodness is zero, it means runnable processes are other old process
		 * have exhausted CPU and new epoch has to begin
		 * For Current old process, if counter = 0, exhausted CPU, forced to next epoch
		 * If it does not want to stay PRCURR despite CPU time, next epoch
		 */
		if (newEpoch == FALSE) {
			newEpoch = TRUE;
			int i;
			struct pentry *p;
			for (i = 0; i < NPROC; i++) {
				p = &proctab[i];
				if (p->pstate == PRFREE)
					continue;
				else if (p->quantumleft == 0 || p->quantumleft == p->quantum) { //Process either exhausted or not run at all;
					p->quantum = p->pprio;
				} else {
					p->quantum = (p->quantumleft) / 2 + p->pprio;
				}
				p->quantumleft = p->quantum;
				p->goodness = p->quantumleft + p->pprio;
			}

			preempt = optr->quantumleft;
			goto choose;
		}
		/*
		 * Now that quantum, counter and goodness are set, choose process with best goodness.
		 * Since optr is still current process, preempt is set to optr->counter
		 */
		if (maxGoodness == 0) {
			if (currpid == NULLPROC) {
				return OK;
			} else {
				newprocPID = NULLPROC;
				if (optr->pstate == PRCURR) { //Just like in Xinu Scheduling
					optr->pstate = PRREADY;
					insert(currpid, rdyhead, optr->pprio);
				}
				nptr = &proctab[newprocPID];
				nptr->pstate = PRCURR;
				dequeue(newprocPID);
				currpid = newprocPID;
#ifdef	RTCLOCK
				preempt = QUANTUM;
#endif
				ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
				return OK;
			}
		}
	} else if (optr->goodness >= maxGoodness && optr->goodness > 0 && optr->pstate == PRCURR) {
		/*
		 * Current process has maximum goodness,
		 * Its goodness is greater than 0.
		 * If goodness is 0, then all runnable processes have same goodness and they have to
		 * chosen in Round-Robin manner. Goodness being 0 also means exhausted CPU time.
		 * So, no need to run it again in this epoch.
		 * Currpid wants to be CURR state.
		 * In the same epoch, execute till its counter exhausts i.e, reduces to 0.
		 */
		preempt = optr->quantumleft;
		return OK;
	}
	/*
	 * There can be multiple processes with non-zero maxGoodness, in that case
	 * while iterating readyList from head to tail, we will get the last one and execute it
	 * Then next from the last with same maxGoodness. Meanwhile if any process enter readyList
	 * it will not be executed till next epoch as its goodness stays 0.
	 */
	else if (maxGoodness > 0 && (optr->pstate != PRCURR || optr->quantumleft == 0 || optr->goodness < maxGoodness)) {
		/*
		 * Currpid does not want to be PRCURR state any more even though it has more goodness and CPU time
		 * Exhausted allocated CPU time, but wants to be PRCURR and has maxGoodness
		 * Its goodness is less than maxGoodness, and has CPU time and wants to be PRCURR
		 * Process must be rescheduled and next process must begin.
		 */
		if (optr->pstate == PRCURR) { //Just like in Xinu Scheduling
			optr->pstate = PRREADY;
			insert(currpid, rdyhead, optr->pprio);
		}
		nptr = &proctab[newprocPID];
		nptr->pstate = PRCURR;
		dequeue(newprocPID);
		currpid = newprocPID;
		preempt = nptr->quantumleft;
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		return OK;
	} else
		return SYSERR;
}

int Xinu_Default_Scheduling() {

	register struct pentry *optr; /* pointer to old process entry */
	register struct pentry *nptr; /* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/

	if (((optr = &proctab[currpid])->pstate == PRCURR) && (lastkey(rdytail) < optr->pprio)) {
		return (OK);
	}

	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		insert(currpid, rdyhead, optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	nptr = &proctab[(currpid = getlast(rdytail))];
	nptr->pstate = PRCURR; /* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM; /* reset preemption counter	*/
#endif

	ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);

	/* The OLD process returns here when resumed. */
	return OK;

}
