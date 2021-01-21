// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------
Scheduler::Scheduler()
{ 
    //readyList = new List<Thread *>; 
    readyListL1 = new SortedList<Thread *>(comp_burst_time);
    readyListL2 = new SortedList<Thread *>(comp_priority);
    readyListL3 = new List<Thread *>;

    toBeDestroyed = NULL;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //delete readyList; 
    delete readyListL1;
    delete readyListL2;
    delete readyListL3;
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    thread->setStatus(READY);
    //readyList->Append(thread);

    ///mod
    int insert_level;
    if (thread->getPriority() <= 49) { ///L3
        readyListL3->Append(thread);
        insert_level = 3;
        thread->setReadyTime();
    }
    else if (thread->getPriority() >= 50 && thread->getPriority() <= 99) { ///L2
        readyListL2->Insert(thread);
        insert_level = 2;
        thread->setReadyTime();
    }
    else if (thread->getPriority() >= 100 && thread->getPriority() <= 149) { ///L1
        readyListL1->Insert(thread);
        insert_level = 1;
        thread->setReadyTime();
    }
    DEBUG('z',"[A] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<thread->getID()<<"] is inserted into queue L["<<insert_level<<"]");
    ///
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    /*if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }*/

    //Aging();

    ///priority L1>L2>L3
    int remove_level;
    Thread* removed_thread;
    if (!readyListL1->IsEmpty()) {
        removed_thread = readyListL1->RemoveFront();
        remove_level = 1;
    }
    else if (!readyListL2->IsEmpty()) {
        removed_thread = readyListL2->RemoveFront();
        remove_level = 2;
    }
    else if (!readyListL3->IsEmpty()) {
        removed_thread = readyListL3->RemoveFront();
        remove_level = 3;
    }
    else{
        return NULL;
    }
    DEBUG('z',"[B] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<removed_thread->getID()<<"] is removed from queue L["<<remove_level<<"]");
    return removed_thread;
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    DEBUG('z',"[E] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<kernel->currentThread->getID()<<"] is now selected for execution, thread ["<<oldThread->getID()<<"] is replaced, and it has executed ["<<kernel->stats->totalTicks-oldThread->getStartBurst()<<"] ticks");

    kernel->currentThread->setStartBurst();
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".
    ///old sleep
    SWITCH(oldThread, nextThread);
    // we're back, running oldThread
    oldThread->setStartBurst();
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyListL3->Apply(ThreadPrint);
}

void Scheduler::Aging()
{
    ListIterator<Thread*> * l1 = new ListIterator<Thread *>((List<Thread *> *)readyListL1);
    ListIterator<Thread*> * l2 = new ListIterator<Thread *>((List<Thread *> *)readyListL2);
    ListIterator<Thread*> * l3 = new ListIterator<Thread *>((List<Thread *> *)readyListL3);
    
    for(; !l1->IsDone(); l1->Next()){
        Thread *curThread = l1->Item();
        curThread->waitingTime+=100;
        if(curThread->waitingTime >= 1500){
            curThread->waitingTime -= 1500;
            //cout<<"L1Aging!\n";
            curThread->setReadyTime(); /// reflash wiating time = 0
            int old_priority = curThread->getPriority();
            int new_priority = curThread->getPriority()+10;
            curThread->setPriority(new_priority);
            //DEBUG('z',"[C] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] changes its priority from ["<<old_priority<<"] to ["<<curThread->getPriority()<<"]");
        }
    }

    for(; !l2->IsDone(); l2->Next()){
        Thread *curThread = l2->Item();
        curThread->waitingTime+=100;
        if(curThread->waitingTime >= 1500){
            curThread->waitingTime -= 1500;
            curThread->setReadyTime(); /// reflash wiating time = 0
            int old_priority = curThread->getPriority();
            int new_priority = curThread->getPriority()+10;
            if(new_priority >= 100){ ///move to L1 from L2
                //cout<<"L2Aging!\n";
                curThread->setPriority(new_priority);
                readyListL2->Remove(curThread);
                DEBUG('z',"[B] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] is removed from queue L["<<2<<"]");
                readyListL1->Insert(curThread);
                DEBUG('z',"[A] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] is inserted into queue L["<<1<<"]");
            }
            else curThread->setPriority(new_priority); /// remain in L2
            //DEBUG('z',"[C] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] changes its priority from ["<<old_priority<<"] to ["<<curThread->getPriority()<<"]");
        }
    }

    for(; !l3->IsDone(); l3->Next()){
        Thread *curThread = l3->Item();
        curThread->waitingTime += 100;
        if(curThread->waitingTime  >= 1500){
            curThread->waitingTime -= 1500;
            //cout<<"L3Aging!\n";
            curThread->setReadyTime(); /// reflash wiating time = 0
            int old_priority = curThread->getPriority();
            int new_priority = curThread->getPriority()+10;
            if(new_priority >= 50){ ///move to L2 from L3
                curThread->setPriority(new_priority);
                readyListL3->Remove(curThread);
                DEBUG('z',"[B] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] is removed from queue L["<<3<<"]");
                readyListL2->Insert(curThread);
                DEBUG('z',"[A] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] is inserted into queue L["<<2<<"]")
            }
            else curThread->setPriority(new_priority);///remian in L3

            //DEBUG('z',"[C] Tick ["<<kernel->stats->totalTicks<<"]: Thread ["<<curThread->getID()<<"] changes its priority from ["<<old_priority<<"] to ["<<curThread->getPriority()<<"]");
        }
    }
}