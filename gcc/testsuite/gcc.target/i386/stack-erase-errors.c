/* { dg-do compile } */
/* { dg-options "-O2 -fno-stack-erase" } */

/* Non-stack erase function does not result in a clear stack.  */
int f1_sub (int x, int y)
{
  int a[300], s;
  for (int i = 0; i < x; i++)
  {
    a[i] = a[i] + x;
  }

  return a[y];
}

/* Stack erase function calling non-stack erase function */
/* should we generate a warning/error for this scenario? */
__attribute__((stack_erase))
int f1 (int x, int y)
{
  int a = f1_sub (x, y); /* { dg-error "Cannot call non-stack-erase function 'f1_sub' from stack-erase function" } */
  return a + 4;
}

// __attribute__((stack_erase))
// int f2_sub (int (*myfunc) (int, int) __attribute__((stack_erase)), int x, int y)
// {
//   myfunc(x, y); // should be an error here
// }

// __attribute__((stack_erase))
// int f2 (int x, int y)
// {
//   f2_sub (f1_sub, x ,y);
// }

/* calling by pointer-to-function not allowed in stack-erase function */
__attribute__((stack_erase))
int f2_sub (int (*myfunc) (int, int), int x, int y)
{
  myfunc(x, y); /* { dg-error "cannot call using function pointer 'myfunc' from stack-erase function" } */
}

__attribute__((stack_erase))
int f2 (int x, int y)
{
  f2_sub (f1_sub, x ,y);
}

__attribute__((stack_erase))
int f3 (int x, int y); /* { dg-error "'stack_erase' attribute present on 'f3'" } */

int f3 (int x, int y) /* { dg-error "but not here" } */
{
  return x + y;
}

int f4 (int x, int y); /* { dg-error "but not here" } */

__attribute__((stack_erase))
int f4 (int x, int y) /* { dg-error "'stack_erase' attribute present on 'f4'" } */
{
  return x + y;
}

/* always inline stack erase function error */
// __attribute__((stack_erase, always_inline))
// int f5 (int x, int y) {
//   return x + y;
// }

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
  /* Stack erase function calling non-stack erase function. */
  if ((r = test (f1)))
    return 1;
  if ((r = test (f3)))
    return 3;
  return 0;
}