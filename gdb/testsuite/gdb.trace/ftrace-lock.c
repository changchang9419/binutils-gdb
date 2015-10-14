/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <pthread.h>

#ifdef SYMBOL_PREFIX
#define SYMBOL(str)     SYMBOL_PREFIX #str
#else
#define SYMBOL(str)     #str
#endif

/* Called if the testcase failed.  */
static void
fail (void)
{
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* This function overrides gdb_collect in the in-process agent library.
   See gdbserver/tracepoint.c (gdb_collect).  We want this function to
   be ran instead of the one from the library to easily check that only
   one thread is tracing at a time.

   This works as expected because GDBserver will ask GDB about symbols
   present in the inferior with the 'qSymbol' packet.  And GDB will
   reply with the address of this function instead of the one from the
   in-process agent library.  */

void
gdb_agent_gdb_collect (void *tpoint, unsigned char *regs)
{
  /* If we cannot acquire a lock, then this means another thread is
     tracing and the lock implemented by the jump pad is not working!  */
  if (pthread_mutex_trylock (&mutex) != 0)
    {
      fail ();
      return;
    }

  sleep (1);

  if (pthread_mutex_unlock (&mutex) != 0)
    {
      fail ();
      return;
    }
}

/* Called from asm.  */
static void __attribute__((used))
func (void)
{
}

static void *
thread_function (void *arg)
{
  /* `set_point' is the label at which to set a fast tracepoint.  The
     insn at the label must be large enough to fit a fast tracepoint
     jump.  */
  asm ("    .global " SYMBOL (set_point) "\n"
       SYMBOL (set_point) ":\n"
#if (defined __x86_64__ || defined __i386__)
       "    call " SYMBOL (func) "\n"
#elif (defined __aarch64__)
       "    nop\n"
#endif
       );
}

static void
end (void)
{
}

int
main (int argc, char *argv[], char *envp[])
{
  pthread_t threads[NUM_THREADS];
  int i;

  for (i = 0; i < NUM_THREADS; i++)
    pthread_create (&threads[i], NULL, thread_function, NULL);

  for (i = 0; i < NUM_THREADS; i++)
    pthread_join (threads[i], NULL);

  end ();

  return 0;
}