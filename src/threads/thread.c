#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
/********NEW CHANGE ******************************/
/* fixed-point needed for MLFQs calculation */
#include "threads/fixed-point.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;

/********NEW CHANGE ******************************/
/* Fixed Integer to hold system load average */
//intToFixed (load_avg);
int load_avg;

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.
   Also initializes the run queue and the tid lock.
   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().
   It is not safe to call thread_current() until this function
   finishes. */
void//Not changed
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void//Chnaged
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  /********NEW CHANGE ******************************/
  /* At thread start, load average is initialized to 0 */
  load_avg = intToFixed(0);

  /********NEW CHANGE! ******************************/
  /* At thread start, load average is initialized to 0 */
  //recent_cpu = intToFixed(0);

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void//Not changed
thread_tick (void) 
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}

/* Prints thread statistics. */
void//Not changed
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.
   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.
   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t//changed
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  /* Add to run queue. */
  thread_unblock (t);

  thread_preempt ();

  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().
   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void//Unchanged
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)
   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void//Changed
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  /* Add thread to the ready list */
  list_insert_ordered (&ready_list, &t->elem,priority_compare, NULL);

  t->status = THREAD_READY;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char * // unchanged
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread * //unchanged
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t//unchanged
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void //unchanged
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void //Changed
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
    /* add the thread to the ready list */
    list_insert_ordered (&ready_list, &cur->elem, priority_compare, NULL);
  /* Change Thread status */
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void //unchanged
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/* Sets the current thread's priority to NEW_PRIORITY.
   If the priority is donated by another thread, it may
   not change immediately. */
void //Chnaged
thread_set_priority (int new_priority) 
{
 
  if (thread_mlfqs)
    return;
  
  enum intr_level old_level = intr_disable ();
  struct thread *current_thread = thread_current ();
  int last_priority = current_thread->effective_priority;

  current_thread->priority = new_priority;

  if (last_priority > new_priority && list_empty (&current_thread->donors))
    {
      current_thread->effective_priority = new_priority;
      thread_preempt ();
    }

  intr_set_level (old_level);
}


int
thread_get_priority (void) 
{
  return thread_current ()->effective_priority;
}

void 
donate_priority (struct thread *t)
{
  enum intr_level old_level = intr_disable ();
  update_priority (t);

  if (t->status == THREAD_READY)
    {
      list_remove (&t->elem);
      list_insert_ordered (&ready_list, &t->elem, priority_compare, NULL);
    }
  intr_set_level (old_level);
}

void 
update_priority (struct thread *t)
{
  enum intr_level old_level = intr_disable ();
  int highPriority = t->priority;
  int lock_priority;

  if (!list_empty (&t->donors))
    {
     
      list_sort (&t->donors, priority_lock_compare, NULL);
      lock_priority = list_entry (list_front (&t->donors),
                                  struct lock, elem)->high_priority;
     
      if ( highPriority < lock_priority)
       highPriority = lock_priority;
    }

  t->effective_priority = highPriority;
  intr_set_level (old_level);
}

/* Test if current thread should be preempted. */
void 
thread_preempt (void)
{
  enum intr_level old_level = intr_disable ();

  if (!list_empty (&ready_list) && thread_current ()->effective_priority < 
      list_entry (list_front (&ready_list), struct thread, elem)->effective_priority)
        thread_yield ();
  intr_set_level (old_level);
}

/* ADDED
 * Compares the priorities of two threads
 */
bool priority_compare (const struct list_elem *a,
                      const struct list_elem *b,
                      void *aux UNUSED)
{
  struct thread *t1 = list_entry (a, struct thread, elem);
  struct thread *t2 = list_entry (b, struct thread, elem);
  return t1->effective_priority > t2->effective_priority;
}


/********NEW CHANGE ******************************/
/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice UNUSED) 
{
  thread_current () ->nice = nice;
  mlfqs_calc_priority(thread_current() );
}

/********NEW CHANGE ******************************/
/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
  /*  */
  return thread_current ()->nice;
}

/********NEW CHANGE ******************************/
/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  int valHundred = intToFixed(100);     // Change 100 to fixed point
  return mulFixedFixed(load_avg, valHundred);
}

/********NEW CHANGE ******************************/
/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
  int valHundred = intToFixed(100);
  return mulFixedFixed(thread_current()->recent_cpu, valHundred);
}



/* Idle thread.  Executes when no other thread is ready to run.
   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.
         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.
         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  enum intr_level old_level;

  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;
  t->priority = priority;
  t->effective_priority = priority;
  list_init (&t->donors);
  t->blocking_lock = NULL;

  /********NEW CHANGE ******************************/
  /* Added initialize MLFQ values to 0 */
  t->nice = intToFixed(0);
  t->recent_cpu = intToFixed(0);

  t->magic = THREAD_MAGIC;

  old_level = intr_disable ();
  list_push_back (&all_list, &t->allelem);
  intr_set_level (old_level);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.
   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).
   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.
   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.
   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);

/************************IN PROGRESS************************/
/* Still need to implement recalculation of new priority of 
 * thread from mlfqs_calc_priority, set priority again

 * Recalculation of recent cpu made when 
 * timer_ticks () % TIMER_FREQ == 0 -> implemented in timer.c

 * Calculating load_avg updated every second when
 * timer_ticks () % TIMER_FREQ == 0 -> implemented in timer.c

/********NEW CHANGE! ******************************/
/* Calculates the thread's priority based on new nice 
priority = PRI_MAX - (recent_cpu / 4) - (nice * 2) */
void mlfqs_calc_priority (struct thread *t)
{
//    if (t != idle_thread)   // Make sure thread is running
//    {
  	int multiply;
    int num4 = intToFixed(4);
    int divider;
    int subtracter;
    int niceValue = t->nice;

      	/* divider = recent_cpu / 4 */
      	divider = divFixedFixed(intToFixed(t->recent_cpu), num4);
      	/* multiply = nice * 2 */
        multiply = mulFixedInt(intToFixed(niceValue, 2);
        /* subtracter = PRI_MAX - divider */
        subtracter = subFixedFixed(intToFixed(PRI_MAX), intToFixed(divider));
        t->priority = subFixedFixed(intToFixed(subtracter), intToFixed(multiply));
      //  t->priority = fixedToInt_RoundNearest(t->priority);

        /* Compare thread priority to make sure it is withing range of PRI_MIN and PRI_MAX */
        if(t->priority < PRI_MIN)     // If priority is smaller than defaul PRI_MIN, make it PRI_MIN
          t->priority = PRI_MIN;      // If priority is bigger than defaul PRI_MAX, make it PRI_MAX
        if(t->priority > PRI_MAX)
          t->priority = PRI_MAX;
//    }
    /* CLEAN UP/COMMENT*/
}

/********NEW CHANGE ******************************/
/* Calculate recent CPU using calculated load_avg and recent_cpu 
recent_cpu = (2 * load_avg) / (2 * load_avg + 1) * recent_cpu + nice */
void mlfqs_calc_cpu (struct thread *t)
{
  int numTwo = intToFixed(2);             // Fixed point number 2
  int numOne = intToFixed(1);             // Fixed point number 1
//    intToFixed(divided);                    // Fixed point to hold divided value
//    intToFixed(divider);                    // Fixed point to hold divider value
//    intToFixed(addedVal);                   // Fixed point to hold added value
//    intToFixed(multiplyVal);                // Fixed point to hold multiplied value
  int divided;
  int divider;
  int multiplyVal;

      /* divided = (2 * load_avg) */
      divided = mulFixedInt(numTwo, load_avg);

      /* divider = (2 * load_avg) + 1 */
      divider = addFixedFixed(mulFixedInt(numTwo, load_avg), 1);

      /* multiplyVal = (divided / divider) * recent_cpu */
      multiplyVal = mulFixedInt(divFixedFixed(divided, divider), t->recent_cpu);

      /* recent_cpu = multiplyVal + nice -> (2 * load_avg) / (2 * load_avg + 1) * recent_cpu + nice */
      t->recent_cpu = addFixedInt(multiplyVal, t->nice);
}

/********NEW CHANGE ******************************/
/* Calculate load_avg using number of ready_threads and current load_avg 
    load_avg = (59/60) * load_avg + (1/60) * ready_threads */
void mlfqs_calc_load_avg (void)
{
    int numOne = intToFixed(1);           // Fixed point number 1
    int numSixty = intToFixed(60);        // Fixed point number 60
    int numFiftyNine = intToFixed(59);    // Fixed point number 59
//      intToFixed(multiply1);                // Fixed point to hold first multiplication value
//      intToFixed(multiply2);                // Fixed point to hold second multiplication value
    int multiply1;
    int multiply2;

    /* Use list_size from list.c to find number of ready threads */
    int ready_threads = list_size(&ready_list); 

    /* multiply1 = (59/60) * loadAvg */    
    multiply1 = mulFixedInt(divFixedFixed(numFiftyNine, numSixty), load_avg);

    /* multiply2 = (1/60) * ready_threads */   
    multiply2 = mulFixedInt(divFixedFixed(numOne, numSixty), ready_threads); 

    /* load_avg = multiply1 + multiply2 -> (59/60) * load_avg + (1/60) * ready_threads */
    load_avg = addFixedFixed(multiply1, multiply2);
}

/********NEW CHANGE! ******************************/
/* Recent cpu for runnning thread increment by 1 for each interrupt */
void mlfqs_increment (void)
{
	int numOne = intToFixed(1);
	thread_current()->recent_cpu = addFixedInt(numOne, thread_current()->recent_cpu);
}

/********NEW CHANGE! ******************************/
/* Recalculates and updates priority and recent_cpu all other threads in list */
/* pintos page 62: list_elem */
void mlfqs_recalculate (void)
{
	mlfqs_calc_load_avg();
	struct thread *t;
	struct list_elem *e;
		/* For all process recalculate cpu and priority
			->Iteration algorithm found from kernel/list.h example */
		for(e = list_begin(&all_list); e != list_end(&all_list); e = list_next(e))
		{
			/* Conversion from list_elem back to structure object */
			//t = list_entry(e, struct thread, allelem);
			mlfqs_calc_cpu(t);
			mlfqs_calc_priority(t);
		}
}

