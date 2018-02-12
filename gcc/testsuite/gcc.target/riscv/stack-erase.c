/* { dg-do run } */
/* { dg-options "-O2 -fno-stack-erase" } */

/* Simple function that uses the stack */
__attribute__((stack_erase))
int f1 (int x, int y)
{
  int a[10], s;
  for (int i = 0; i < x; i++)
  {
    a[i] = a[i] + x;
  }

  return a[y];
}

/* For preventing VLA in f2 getting optimized away.  */
int f2_glob = 7;

__attribute__((noinline))
int f2_sub ()
{
  return f2_glob;
}

/* Function using VLA */
__attribute__((stack_erase))
int f2 (int x, int y)
{
  int l = x + 2;
  int a[l];
  int s = 0;

  for (int i = 0; i < l; i++)
  {
    a[i] = f2_sub ();
  }

  return a[y];
}

/* Stack erase function calling another stack erase function.  */
__attribute__((stack_erase))
int f3 (int x, int y)
{
  int a = f1 (x, y);
  return a + 2;
}


/* Non-stack erase function calling a stack erase function does not result in
   a clear stack.  */
int f4 (int x, int y)
{
  int a = f1 (x, y);
  return a + 3;
}

/* Non-stack erase function does not result in a clear stack.  */
int f5 (int x, int y)
{
  int a[10], s;
  for (int i = 0; i < x; i++)
  {
    a[i] = a[i] + x;
  }

  return a[y];
}

/* Function using VLA inside a scope.  */
__attribute__((stack_erase))
int f6 (int x, int y)
{
  int l = x + 2;
  int a[l];

  for (int i = 0; i < l; i++)
  {
    int b[i];
    int s = 0;
    for (int j = 0; j < i; j++)
      s += f2_sub();
    a[i] = s;
  }

  return a[y];
}


int test(int (*func) (int, int))
{
  // Zero 2kb of stack space and record SP
  void *initial_sp;
  __asm__ volatile (
    "  addi    t1, sp, -2048\n"
    "1:\n"
    "  beq     t1, sp, 2f\n"
    "  sw      x0, 0(t1)\n"
    "  addi    t1, t1, 4\n"
    "  j       1b\n"
    "2:\n"
    "  mv      %0, sp\n"
    : "=r" (initial_sp) : : );

  // Call test function
  func(7, 5);

  // Check stack pointer is unchanged after return and that stack is zeroed.
  int stack_check;
  __asm__ volatile (
  "  bne     sp, %1, 3f\n"
  "  addi    t1, sp, -2048\n"
  "1:\n"
  "  beq     t1, sp, 4f\n"
  "  lw      t2, 0(t1)\n"
  "  bne     x0, t2, 2f\n"
  "  addi    t1, t1, 4\n"
  "  j 1b\n"
  "2:\n"
  "  li      %0, 1\n"
  "  j       5f\n"
  "3:\n"
  "  li      %0, 2\n"
  "  j       5f\n"
  "4:\n"
  "  li      %0, 0\n"
  "  j       5f\n"
  "5:"
  : "=r" (stack_check)
  : "r" (initial_sp) : );

  return stack_check;
}

int main (void) {
  int r;
  if ((r = test (f1)))
    return 1;
  if ((r = test (f2)))
    return 2;
  if ((r = test (f3)))
    return 3;
  /* Non-stack erase function calling stack erase function.  */
  if ((r = test (f4)) != 1)
    return 4;
  /* Non-stack erase function.  */
  if ((r = test (f5)) != 1)
    return 5;
  if ((r = test (f6)))
    return 6;
  return 0;
}
