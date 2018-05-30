/* { dg-do run } */
/* { dg-options "-O0 -fno-stack-erase" } */
/* { dg-options "-O1 -fno-stack-erase" } */
/* { dg-options "-O2 -fno-stack-erase" } */
/* { dg-options "-O3 -fno-stack-erase" } */
/* { dg-options "-O0 -fno-stack-erase -mno-red-zone" } */
/* { dg-options "-O1 -fno-stack-erase -mno-red-zone" } */
/* { dg-options "-O2 -fno-stack-erase -mno-red-zone" } */
/* { dg-options "-O3 -fno-stack-erase -mno-red-zone" } */

/* Simple function that uses the stack */
__attribute__((stack_erase))
int f1 (int x, int y)
{
  int a[300], s;
  for (int i = 0; i < x; i++)
  {
    a[i] = a[i] + x;
  }
  return a[y];
}

/* For preventing VLA in f2 getting optimized away.  */
int f2_glob = 7;

__attribute__((noinline, stack_erase))
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
    // The caller is responsible for removing the return address used by f2_sub
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
  int a[300], s;
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

/* tail call */
/* f7 pushes 200 more ints onto the stack than f1 */
__attribute__((stack_erase))
int f7 (int x, int y) {
  volatile int a[500], s;
  for (int i = 0; i < x; i++)
  {
    a[i] = a[i] + x;
  }
  return f1(x, y);
}

/* recursion */
__attribute__((stack_erase))
int fib(int i)
{
  volatile int a = 5678;
  if(i==0)
    return 1;
  if(i==1)
    return 1;
  return fib(i-1) + fib(i-2);
}

__attribute__((stack_erase))
int f8 (int x, int y) {
  volatile int a = 1234;
  return fib(x-y);
}

__attribute__((stack_erase))
int f9_args(int a0,  int a1,  int a2,  int a3,  int a4,  int a5,  int a6,  int a7,
             int a8,  int a9,  int a10, int a11, int a12, int a13, int a14, int a15,
             int a16, int a17, int a18, int a19, int a20, int a21, int a22, int a23,
             int a24, int a25) {
  int a = f1 (a0, a1);
  return a + 7;  
}

/* test that caller clears pushed arguments */
__attribute__((stack_erase))
int f9 (int x, int y) {
  int a = f1 (x, y);
  int b = f9_args(0,  1,  2,  3,  4,  5,  6,  7, 
                   8,  9,  10, 11, 12, 13, 14, 15,
                   16, 17, 18, 19, 20, 21, 22, 23,
                   24, 25);
  return a + b + 10;
}

int test(int (*func) (int, int))
{
  // Zero 2kb of stack space and record SP
  void *initial_sp;
  void *t1;
  __asm__ volatile (
    "  movq    %%rsp, %1\n"
    "  subq    $2048, %1\n"
    "1:\n"
    "  cmpq    %%rsp, %1\n"
    "  je      2f\n"
    "  pushq   $0\n"
    "  jmp     1b\n"
    "2:\n"
    "  addq    $2048, %%rsp\n" // restore sp to initial value
    "  movq    %%rsp, %0" // restore sp to initial value
    : "=r" (initial_sp), "+r" (t1) : : );

  // Call test function
  func(250, 240);

  // Check stack pointer is unchanged after return and that stack is zeroed.
  int stack_check;
  void *t2;
  __asm__ volatile (
  "  cmpq    %%rsp, %3\n" // check rsp unchanged
  "  jne     3f\n"
  "  movq    %%rsp, %1\n"
  "  subq    $2048, %%rsp\n" // goto to top of region to check
  "  subq    $8, %1\n" // ignore callee return address
  "1:\n"
  "  cmpq    %1, %%rsp\n"
  "  je      4f\n"
  "  popq    %2\n"
  "  cmpq    $0, %2\n"
  "  jne     2f\n"
  "  jmp 1b\n"
  "2:\n"
  "  movl    $1, %0\n"
  "  jmp     5f\n"
  "3:\n"
  "  movl    $2, %0\n"
  "  jmp     5f\n"
  "4:\n"
  "  movl    $0, %0\n"
  "  jmp     5f\n"
  "5:\n"
  "  movq    %3, %%rsp"
  : "+r" (stack_check), "+r" (t1), "+r" (t2)
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
  if ((r = test (f7)))
    return 7;
  if ((r = test (f8)))
    return 8;
  if ((r = test (f9)))
    return 9;
  return 0;
}
