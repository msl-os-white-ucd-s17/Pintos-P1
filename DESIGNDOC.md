
            +--------------------+

            |  Operating Systems |

            | PROJECT 1: THREADS |

            |   DESIGN DOCUMENT  |

            +--------------------+
   
   
## Team White

Stefani Moore <stefani.moore@ucdenver.edu>

Shawn Johnson <shawn.a.johnson@ucdenver.edu>

Lena Banks <lena.banks@ucdenver.edu>

Sara Kim <sara.kim@ucdenver.edu>

## Tests Passed

20 of 27 tests passed

### ALARM CLOCK

#### DATA STRUCTURES

A1: Copy here the declaration of each new or changed struct or struct member, global or static variable, typedef, or enumeration. Identify the purpose of each in 25 words or less.

> * In thread.h:   
  sleepTickCount : int64_t
  - In thread struct, holds thread tick count used to determine when a thread
    is ready to be unblocked.

> * In timer.c:  
  sleep_list : static struct list
  - A list of sleeping or blocked threads that are added in timer_sleep() 
    and removed in timer_interrupt()

#### ALGORITHMS


A2: Briefly describe what happens in a call to timer_sleep(), including the effects of the timer interrupt handler.

>The timer_sleep() function begins by checking that the tick count is >= 0
because if the ticks are 0 or below then there is no need to continue on.
Our tick argument, sleepTickCount, is then calculated by adding the global
ticks to this argument. The current thread is then placed into the 
sleep_list sleeping thread queue, which is sorted by tick count with thread
to be woken up next at the front. This thread is then blocked. The interrupts
are disabled for this process so that the ticks may be reliably calculated and
the thread can be blocked.  In the timer_interrupt function, the threads 
member sleepTickCount is check against the amount of ticks that have passed
since the member was initialized.

A3: What steps are taken to minimize the amount of time spent in the timer interrupt handler?

>Keeping the list sorted helps to minimize the amount of time spent in the
timer interrupt handler. Also, within the while loop if the front thread
of the sleep_list's tick count does not need to be woken up yet, then the 
the while loop is broken. If it does need to be woken up, the next thread
in the sleeping list will be checked. Therefore, the handler does not have to 
iterate through the entire sleep list at every interrupt. 

#### SYNCHRONIZATION

A4: How are race conditions avoided when multiple threads call timer_sleep() simultaneously?

>Race conditions are avoided when multiple threads are called by disabline the interrupts.
These are disabled in timer sleep and this results in only one thread being in the function
at a time so that no other thread can take the current thread off of the CPU.

A5: How are race conditions avoided when a timer interrupt occurs during a call to timer_sleep()?

>Race conditions are avoided when a timer interrupt occurs during a call to timer_sleep in the
same way as mentioned above. The interrupts are disabled so that there is no way for
a timer interrrupt to occur until the end of the function when the interrupts are
re-enabled.

#### RATIONALE


A6: Why did you choose this design? In what ways is it superior to another design you considered?

>Initially it was considered to create a seperate wake sleeping thread method, 
which also would have worked, but our current implementation is simpler. This
design was chosen because it made the most sense when it comes down to minimizing
the time spent in the timer interrupt handler.

### PRIORITY SCHEDULING


#### DATA STRUCTURES


B1: Copy here the declaration of each new or changed struct or struct member, global or static variable, typedef, or enumeration. Identify the purpose of each in 25 words or less.
> * A macro `#define NESTED_MAX_DEPTH 9` was declared as a cutoff for the depth level of nested priority donation.
> * A variable `int effective_priority` was added to hold a thread's donated priority.
> * A structure `struct list held_locks` was declared to contain all locks held by the thread.
> * A variable `struct lock *blocking_lock` was added to indicate which lock a thread is waiting on.
> * A struct member `struct list_elem elem` was added to `struct lock` to obtain the lock structure referenced by a `struct list_elem` in `struct list held_locks`.
> * A struct member `int high_priority` was added to `struct lock` to indicate the highest donated priority of the thread currently holding the lock.

B2: Explain the data structure used to track priority donation.
> The thread and lock structure members `struct list held_locks`,`struct lock *blocking_lock`, and `struct list_elem elem` form a linked-list of priority donations that permits painless traversal. This allows us to easily implement priority donation through a loop that terminates on either of the following conditions: a lock is held by a thread with a higher priority than the current thread or the last donee thread is not blocking on a lock.


#### ALGORITHMS


B3: How do you ensure that the highest priority thread waiting for a lock, semaphore, or condition variable wakes up first?
> The highest priority thread will be woken up by `void sema_up(struct semaphore *sema)` when it is called by `void
lock_release (struct lock *lock)` because the first thread in the list of threads waiting for a lock is the one with the highest priority. This behavior is guaranteed because `struct list waiters` in `struct semaphore` is sorted before the highest priority thread is popped from the front of the list and unblocked. This ensures correct scheduling behavior for threads waiting on locks and sempahores in cases where priorities were modified via donation or directly by a component other than the scheduler. It is worth noting that threads are inserted in the correct place following any call to `void sema_down (struct semaphore *sema)` such that the thread blocks on the resource. However, this is insignificant if the priority of any thread in the wait list is modified *post-hoc* to insertion.

B4: Describe the sequence of events when a call to lock_acquire() causes a priority donation. How is nested donation handled?
> The donor (current thread) traverses through locks and lock holders; donating its priority to all lock holders with an `int effective_priority` less than that of its own. The traversal terminates when it finds a holder with a greater priority than its own or a holder that is not blocking on a lock. 

B5: Describe the sequence of events when lock_release() is called on a lock that a higher-priority thread is waiting for.
> See above (B3).


#### SYNCHRONIZATION

B6: Describe a potential race in thread_set_priority() and explain how your implementation avoids it.  Can you use a lock to avoid this race?
> The previous implementation of `thread_set_priority()` did not disable interrupts; allowing the currently executing thread to be preempted. If the thread responsible for the preemption were to get the priority of the interrupted thread and set it based on some condition, then two completely different results could arise given the scenario in which it was interrupted before it was able to finish the critical section versus the scenario in which it did finish before interruption.

#### RATIONALE

B7: Why did you choose this design?  In what ways is it superior to another design you considered?
> We attempted a direct approach by declaring a new structure to hold priorities, a reference to the donee, and references to donors. This failed because the structure was not dependent on any particular lock and the staggered layers of references made it very difficult to keep track of what structure we were currently in. Our current design makes better use of preconditions and postconditions established by sorting linked-lists such that function invariants are maintained. Our old design did not adapt to the mechanics of the system that were already in place; that is, it did not delegate a specific responsibility to a thread based on its state. Rather, it attempted to handle everything at once, as if it only had access to inputs and outputs--much like a black-box. We were trying to build a controller for something that is its own controller. Upon this realization, we were able to produce correct behavior by distributing pieces of the algorithm across different components and functions that are accessed at different times. After all, the whole is only the sum of its parts.


### ADVANCED SCHEDULER


#### DATA STRUCTURES

C1: Copy here the declaration of each new or changed struct or struct member, global or static variable, typedef, or enumeration. Identify the purpose of each in 25 words or less.


#### ALGORITHMS

C2: Suppose threads A, B, and C have nice values 0, 1, and 2. Each has a recent_cpu value of 0.  Fill in the table below showing the scheduling decision and the priority and recent_cpu values for each thread after each given number of timer ticks.
 
 
<table>
            <thead>
                        <tr>
                                    <th rowspan="2">Timer Ticks</th>
                                    <th colspan="3">Recent CPU</th>
                                    <th colspan="3">Priority</th>
                                    <th rowspan="2">Thread to Run</th>
                        </tr>
                        <tr>
                                    <th>A</th>
                                    <th>B</th>
                                    <th>C</th>
                                    <th>A</th>
                                    <th>B</th>
                                    <th>C</th>
                        </tr>
            </thead>
            <tbody>
            <tr><td>0</td><td>0</td><td>0</td><td>0</td><td>63</td><td>61</td><td>59</td><td>A</td></tr>
            <tr><td>4</td><td>4</td><td>0</td><td>0</td><td>62</td><td>61</td><td>59</td><td>A</td></tr>
            <tr><td>8</td><td>8</td><td>0</td><td>0</td><td>61</td><td>61</td><td>59</td><td>B</td></tr>
            <tr><td>12</td><td>8</td><td>4</td><td>0</td><td>61</td><td>60</td><td>59</td><td>A</td></tr>
            <tr><td>16</td><td>12</td><td>4</td><td>0</td><td>60</td><td>60</td><td>59</td><td>B</td></tr>
            <tr><td>20</td><td>12</td><td>8</td><td>0</td><td>60</td><td>59</td><td>59</td><td>A</td></tr>
            <tr><td>24</td><td>16</td><td>8</td><td>0</td><td>59</td><td>59</td><td>59</td><td>C</td></tr>
            <tr><td>28</td><td>16</td><td>8</td><td>4</td><td>59</td><td>59</td><td>58</td><td>B</td></tr>
            <tr><td>32</td><td>16</td><td>12</td><td>4</td><td>59</td><td>58</td><td>58</td><td>A</td></tr>
            <tr><td>36</td><td>20</td><td>12</td><td>4</td><td>58</td><td>58</td><td>58</td><td>C</td></tr>
            </tbody>
</table>  



C3: Did any ambiguities in the scheduler specification make values in the table uncertain?  If so, what rule did you use to resolve them? Does this match the behavior of your scheduler?

C4: How is the way you divided the cost of scheduling between code inside and outside interrupt context likely to affect performance?


#### RATIONALE

C5: Briefly critique your design, pointing out advantages and disadvantages in your design choices. If you were to have extra time to work on this part of the project, how might you choose to refine or improve your design?

C6: The assignment explains arithmetic for fixed-point math in detail, but it leaves it open to you to implement it.  Why did you decide to implement it the way you did?  If you created an abstraction layer for fixed-point math, that is, an abstract data type and/or a set of functions or macros to manipulate fixed-point numbers, why did you do so?  If not, why not?

A fixed-point.h file was created for the arithmetic implementation of the fixed-point math. While the
pintos guide does describe the math in detail, https://jeason.gitbooks.io/pintos-reference-guide-sysu/content/Advanced%20Scheduler.html
was also used as a resource in determining what functions were needed to implement the math. Standard
functions were used for better readability of the code.

### SURVEY QUESTIONS


Answering these questions is optional, but it will help us improve the course in future quarters. Feel free to tell us anything you want--these questions are just to spur your thoughts. You may also choose to respond anonymously in the course evaluations at the end of the quarter.

In your opinion, was this assignment, or any one of the three problems in it, too easy or too hard?  Did it take too long or too little time? 

Did you find that working on a particular part of the assignment gave you greater insight into some aspect of OS design? 

Is there some particular fact or hint we should give students in future quarters to help them solve the problems? Conversely, did you find any of our guidance to be misleading?

Do you have any suggestions for the TAs to more effectively assist students, either for future quarters or the remaining projects?

Any other comments?



