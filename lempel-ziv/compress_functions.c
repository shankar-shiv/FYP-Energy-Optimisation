/*
 * compress_functions.c
 * Description: A program to demonstrate the various compression functions involved in when using LZ4_compress_default().  The idea
 *              is to show how each step in the call stack can be used directly, if desired.  There is also some benchmarking for
 *              each function to demonstrate the (probably lack of) performance difference when jumping the stack.
 *              (If you're new to lz4, please read simple_buffer.c to understand the fundamentals)
 *
 *              The call stack (before theoretical compiler optimizations) for LZ4_compress_default is as follows:
 *                LZ4_compress_default
 *                  LZ4_compress_fast
 *                    LZ4_compress_fast_extState
 *                      LZ4_compress_generic
 *
 *              LZ4_compress_default()
 *                This is the recommended function for compressing data.  It will serve as the baseline for comparison.
 *              LZ4_compress_fast()
 *                Despite its name, it's not a "fast" version of compression.  It simply decides if HEAPMODE is set and either
 *                allocates memory on the heap for a struct or creates the struct directly on the stack.  Stack access is generally
 *                faster but this function itself isn't giving that advantage, it's just some logic for compile time.
 *              LZ4_compress_fast_extState()
 *                This simply accepts all the pointers and values collected thus far and adds logic to determine how
 *                LZ4_compress_generic should be invoked; specifically: can the source fit into a single pass as determined by
 *                LZ4_64Klimit.
 *              LZ4_compress_generic()
 *                As the name suggests, this is the generic function that ultimately does most of the heavy lifting.  Calling this
 *                directly can help avoid some test cases and branching which might be useful in some implementation-specific
 *                situations, but you really need to know what you're doing AND what you're asking lz4 to do!  You also need a
 *                wrapper function because this function isn't exposed with lz4.h.
 *
 *              The call stack for decompression functions is shallow.  There are 2 options:
 *                LZ4_decompress_safe  ||  LZ4_decompress_fast
 *                  LZ4_decompress_generic
 *
 *               LZ4_decompress_safe
 *                 This is the recommended function for decompressing data.  It is considered safe because the caller specifies
 *                 both the size of the compressed buffer to read as well as the maximum size of the output (decompressed) buffer
 *                 instead of just the latter.
 *               LZ4_decompress_fast
 *                 Again, despite its name it's not a "fast" version of decompression.  It simply frees the caller of sending the
 *                 size of the compressed buffer (it will simply be read-to-end, hence it's non-safety).
 *               LZ4_decompress_generic
 *                 This is the generic function that both of the LZ4_decompress_* functions above end up calling.  Calling this
 *                 directly is not advised, period.  Furthermore, it is a static inline function in lz4.c, so there isn't a symbol
 *                 exposed for anyone using lz4.h to utilize.
 *
 *               Special Note About Decompression:
 *               Using the LZ4_decompress_safe() function protects against malicious (user) input.  If you are using data from a
 *               trusted source, or if your program is the producer (P) as well as its consumer (C) in a PC or MPMC setup, you can
 *               safely use the LZ4_decompress_fast function.
 */

/* Since lz4 compiles with c99 and not gnu/std99 we need to enable POSIX linking for time.h structs and functions. */
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif
#define _POSIX_C_SOURCE 199309L

/* Includes, for Power! */
#define LZ4_DISABLE_DEPRECATE_WARNINGS /* LZ4_decompress_fast */
#include "lz4.h"
#include <stdio.h>    /* for printf() */
#include <stdlib.h>   /* for exit() */
#include <string.h>   /* for atoi() memcmp() */
#include <stdint.h>   /* for uint_types */
#include <inttypes.h> /* for PRIu64 */
#include <time.h>     /* for clock_gettime() */
#include <locale.h>   /* for setlocale() */

/* We need to know what one billion is for clock timing. */
#define BILLION 1000000000L

/* Create a crude set of test IDs so we can switch on them later  (Can't switch() on a char[] or char*). */
#define ID__LZ4_COMPRESS_DEFAULT 1
#define ID__LZ4_COMPRESS_FAST 2
#define ID__LZ4_COMPRESS_FAST_EXTSTATE 3
#define ID__LZ4_COMPRESS_GENERIC 4
#define ID__LZ4_DECOMPRESS_SAFE 5
#define ID__LZ4_DECOMPRESS_FAST 6

/*
 * Easy show-error-and-bail function.
 */
void run_screaming(const char *message, const int code)
{
  printf("%s\n", message);
  exit(code);
}

/*
 * Centralize the usage function to keep main cleaner.
 */
void usage(const char *message)
{
  printf("Usage: ./argPerformanceTesting <iterations>\n");
  run_screaming(message, 1);
  return;
}

/*
 * Runs the benchmark for LZ4_compress_* based on function_id.
 */
uint64_t bench(
    const char *known_good_dst,
    const int function_id,
    const int iterations,
    const char *src,
    char *dst,
    const size_t src_size,
    const size_t max_dst_size,
    const size_t comp_size)
{
  uint64_t time_taken = 0;
  int rv = 0;
  const int warm_up = 5000;
  struct timespec start, end;
  const int acceleration = 1;
  LZ4_stream_t state;

  // Select the right function to perform the benchmark on.  We perform 5000 initial loops to warm the cache and ensure that dst
  // remains matching to known_good_dst between successive calls.
  switch (function_id)
  {
  case ID__LZ4_COMPRESS_DEFAULT:
    printf("Starting benchmark for function: LZ4_compress_default()\n");
    for (int junk = 0; junk < warm_up; junk++)
      rv = LZ4_compress_default(src, dst, src_size, max_dst_size);
    if (rv < 1)
      run_screaming("Couldn't run LZ4_compress_default()... error code received is in exit code.", rv);
    if (memcmp(known_good_dst, dst, max_dst_size) != 0)
      run_screaming("According to memcmp(), the compressed dst we got doesn't match the known_good_dst... ruh roh.", 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 1; i <= iterations; i++)
      LZ4_compress_default(src, dst, src_size, max_dst_size);
    break;

  case ID__LZ4_COMPRESS_FAST:
    printf("Starting benchmark for function: LZ4_compress_fast()\n");
    for (int junk = 0; junk < warm_up; junk++)
      rv = LZ4_compress_fast(src, dst, src_size, max_dst_size, acceleration);
    if (rv < 1)
      run_screaming("Couldn't run LZ4_compress_fast()... error code received is in exit code.", rv);
    if (memcmp(known_good_dst, dst, max_dst_size) != 0)
      run_screaming("According to memcmp(), the compressed dst we got doesn't match the known_good_dst... ruh roh.", 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 1; i <= iterations; i++)
      LZ4_compress_fast(src, dst, src_size, max_dst_size, acceleration);
    break;

  case ID__LZ4_COMPRESS_FAST_EXTSTATE:
    printf("Starting benchmark for function: LZ4_compress_fast_extState()\n");
    for (int junk = 0; junk < warm_up; junk++)
      rv = LZ4_compress_fast_extState(&state, src, dst, src_size, max_dst_size, acceleration);
    if (rv < 1)
      run_screaming("Couldn't run LZ4_compress_fast_extState()... error code received is in exit code.", rv);
    if (memcmp(known_good_dst, dst, max_dst_size) != 0)
      run_screaming("According to memcmp(), the compressed dst we got doesn't match the known_good_dst... ruh roh.", 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 1; i <= iterations; i++)
      LZ4_compress_fast_extState(&state, src, dst, src_size, max_dst_size, acceleration);
    break;

    //    Disabled until LZ4_compress_generic() is exposed in the header.
    //    case ID__LZ4_COMPRESS_GENERIC:
    //      printf("Starting benchmark for function: LZ4_compress_generic()\n");
    //      LZ4_resetStream((LZ4_stream_t*)&state);
    //      for(int junk=0; junk<warm_up; junk++) {
    //        LZ4_resetStream((LZ4_stream_t*)&state);
    //        //rv = LZ4_compress_generic_wrapper(&state, src, dst, src_size, max_dst_size, notLimited, byU16, noDict, noDictIssue, acceleration);
    //        LZ4_compress_generic_wrapper(&state, src, dst, src_size, max_dst_size, acceleration);
    //      }
    //      if (rv < 1)
    //        run_screaming("Couldn't run LZ4_compress_generic()... error code received is in exit code.", rv);
    //      if (memcmp(known_good_dst, dst, max_dst_size) != 0)
    //        run_screaming("According to memcmp(), the compressed dst we got doesn't match the known_good_dst... ruh roh.", 1);
    //      for (int i=1; i<=iterations; i++) {
    //        LZ4_resetStream((LZ4_stream_t*)&state);
    //        //LZ4_compress_generic_wrapper(&state, src, dst, src_size, max_dst_size, notLimited, byU16, noDict, noDictIssue, acceleration);
    //        LZ4_compress_generic_wrapper(&state, src, dst, src_size, max_dst_size, acceleration);
    //      }
    //      break;

  case ID__LZ4_DECOMPRESS_SAFE:
    printf("Starting benchmark for function: LZ4_decompress_safe()\n");
    for (int junk = 0; junk < warm_up; junk++)
      rv = LZ4_decompress_safe(src, dst, comp_size, src_size);
    if (rv < 1)
      run_screaming("Couldn't run LZ4_decompress_safe()... error code received is in exit code.", rv);
    if (memcmp(known_good_dst, dst, src_size) != 0)
      run_screaming("According to memcmp(), the compressed dst we got doesn't match the known_good_dst... ruh roh.", 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 1; i <= iterations; i++)
      LZ4_decompress_safe(src, dst, comp_size, src_size);
    break;

  case ID__LZ4_DECOMPRESS_FAST:
    printf("Starting benchmark for function: LZ4_decompress_fast()\n");
    for (int junk = 0; junk < warm_up; junk++)
      rv = LZ4_decompress_fast(src, dst, src_size);
    if (rv < 1)
      run_screaming("Couldn't run LZ4_decompress_fast()... error code received is in exit code.", rv);
    if (memcmp(known_good_dst, dst, src_size) != 0)
      run_screaming("According to memcmp(), the compressed dst we got doesn't match the known_good_dst... ruh roh.", 1);
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 1; i <= iterations; i++)
      LZ4_decompress_fast(src, dst, src_size);
    break;

  default:
    run_screaming("The test specified isn't valid.  Please check your code.", 1);
    break;
  }

  // Stop timer and return time taken.
  clock_gettime(CLOCK_MONOTONIC, &end);
  time_taken = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;

  return time_taken;
}

/*
 * main()
 * We will demonstrate the use of each function for simplicity sake.  Then we will run 2 suites of benchmarking:
 * Test suite A)  Uses generic Lorem Ipsum text which should be generally compressible insomuch as basic human text is
 *                compressible for such a small src_size
 * Test Suite B)  For the sake of testing, see what results we get if the data is drastically easier to compress.  IF there are
 *                indeed losses and IF more compressible data is faster to process, this will exacerbate the findings.
 */
int main(int argc, char **argv)
{
  // Get and verify options.  There's really only 1:  How many iterations to run.
  int iterations = 1000000;
  if (argc > 1)
    iterations = atoi(argv[1]);
  if (iterations < 1)
    usage("Argument 1 (iterations) must be > 0.");

  // First we will create 2 sources (char *) of 2000 bytes each.  One normal text, the other highly-compressible text.
  const char *src = "9.779411554336547852e-01,9.264705777168273926e-01,6.813725233078002930e-01,2.450980395078659058e-01,1.544117629528045654e-01,1.911764740943908691e-01,1.519607901573181152e-01,8.578431606292724609e-02,5.882352963089942932e-02,4.901960864663124084e-02,4.411764815449714661e-02,6.127450987696647644e-02,6.617647409439086914e-02,6.127450987696647644e-02,4.901960864663124084e-02,7.352941483259201050e-02,6.127450987696647644e-02,6.127450987696647644e-02,6.617647409439086914e-02,6.862745434045791626e-02,9.558823704719543457e-02,7.598039507865905762e-02,9.313725680112838745e-02,1.053921580314636230e-01,1.151960790157318115e-01,1.029411777853965759e-01,1.176470592617988586e-01,1.250000000000000000e-01,1.421568691730499268e-01,1.274509876966476440e-01,1.519607901573181152e-01,1.446078419685363770e-01,1.642156839370727539e-01,1.446078419685363770e-01,1.593137234449386597e-01,1.519607901573181152e-01,1.544117629528045654e-01,1.421568691730499268e-01,1.519607901573181152e-01,1.519607901573181152e-01,1.470588296651840210e-01,1.323529481887817383e-01,1.274509876966476440e-01,1.348039209842681885e-01,1.372549086809158325e-01,1.127450987696647644e-01,1.078431382775306702e-01,1.053921580314636230e-01,1.078431382775306702e-01,9.803921729326248169e-02,9.313725680112838745e-02,1.029411777853965759e-01,1.004901975393295288e-01,1.053921580314636230e-01,1.029411777853965759e-01,1.176470592617988586e-01,1.053921580314636230e-01,1.225490197539329529e-01,1.274509876966476440e-01,1.421568691730499268e-01,1.470588296651840210e-01,1.446078419685363770e-01,1.740196049213409424e-01,2.303921580314636230e-01,2.377450913190841675e-01,2.475490123033523560e-01,2.303921580314636230e-01,2.254901975393295288e-01,1.985294073820114136e-01,1.764705926179885864e-01,1.323529481887817383e-01,1.250000000000000000e-01,1.176470592617988586e-01,1.225490197539329529e-01,1.127450987696647644e-01,1.299019604921340942e-01,1.151960790157318115e-01,8.333333581686019897e-02,0.000000000000000000e+00,6.617647409439086914e-02,3.063725531101226807e-01,6.127451062202453613e-01,8.602941036224365234e-01,1.000000000000000000e+00,9.583333134651184082e-01,7.450980544090270996e-01,3.039215803146362305e-01,1.642156839370727539e-01,2.058823555707931519e-01,1.642156839370727539e-01,1.029411777853965759e-01,9.558823704719543457e-02,9.068627655506134033e-02,1.004901975393295288e-01,9.558823704719543457e-02,9.803921729326248169e-02,9.313725680112838745e-02,9.803921729326248169e-02,9.558823704719543457e-02,1.127450987696647644e-01,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00";
  const char *hc_src = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  // Set and derive sizes.  Since we're using strings, use strlen() + 1 for \0.
  const size_t src_size = strlen(src) + 1;
  const size_t max_dst_size = LZ4_compressBound(src_size);
  int bytes_returned = 0;
  // Now build allocations for the data we'll be playing with.
  char *dst = calloc(1, max_dst_size);
  char *known_good_dst = calloc(1, max_dst_size);
  char *known_good_hc_dst = calloc(1, max_dst_size);
  if (dst == NULL || known_good_dst == NULL || known_good_hc_dst == NULL)
    run_screaming("Couldn't allocate memory for the destination buffers.  Sad :(", 1);

  // Create known-good buffers to verify our tests with other functions will produce the same results.
  bytes_returned = LZ4_compress_default(src, known_good_dst, src_size, max_dst_size);
  if (bytes_returned < 1)
    run_screaming("Couldn't create a known-good destination buffer for comparison... this is bad.", 1);
  const size_t src_comp_size = bytes_returned;
  bytes_returned = LZ4_compress_default(hc_src, known_good_hc_dst, src_size, max_dst_size);
  if (bytes_returned < 1)
    run_screaming("Couldn't create a known-good (highly compressible) destination buffer for comparison... this is bad.", 1);
  const size_t hc_src_comp_size = bytes_returned;

  /* LZ4_compress_default() */
  // This is the default function so we don't need to demonstrate how to use it.  See basics.c if you need more basic information.

  /* LZ4_compress_fast() */
  // Using this function is identical to LZ4_compress_default except we need to specify an "acceleration" value.  Defaults to 1.
  memset(dst, 0, max_dst_size);
  bytes_returned = LZ4_compress_fast(src, dst, src_size, max_dst_size, 1);
  if (bytes_returned < 1)
    run_screaming("Failed to compress src using LZ4_compress_fast.  echo $? for return code.", bytes_returned);
  if (memcmp(dst, known_good_dst, bytes_returned) != 0)
    run_screaming("According to memcmp(), the value we got in dst from LZ4_compress_fast doesn't match the known-good value.  This is bad.", 1);

  /* LZ4_compress_fast_extState() */
  // Using this function directly requires that we build an LZ4_stream_t struct ourselves.  We do NOT have to reset it ourselves.
  memset(dst, 0, max_dst_size);
  LZ4_stream_t state;
  bytes_returned = LZ4_compress_fast_extState(&state, src, dst, src_size, max_dst_size, 1);
  if (bytes_returned < 1)
    run_screaming("Failed to compress src using LZ4_compress_fast_extState.  echo $? for return code.", bytes_returned);
  if (memcmp(dst, known_good_dst, bytes_returned) != 0)
    run_screaming("According to memcmp(), the value we got in dst from LZ4_compress_fast_extState doesn't match the known-good value.  This is bad.", 1);

  /* LZ4_compress_generic */
  // When you can exactly control the inputs and options of your LZ4 needs, you can use LZ4_compress_generic and fixed (const)
  // values for the enum types such as dictionary and limitations.  Any other direct-use is probably a bad idea.
  //
  // That said, the LZ4_compress_generic() function is 'static inline' and does not have a prototype in lz4.h to expose a symbol
  // for it.  In other words: we can't access it directly.  I don't want to submit a PR that modifies lz4.c/h.  Yann and others can
  // do that if they feel it's worth expanding this example.
  //
  // I will, however, leave a skeleton of what would be required to use it directly:
  /*
    memset(dst, 0, max_dst_size);
    // LZ4_stream_t state:  is already declared above.  We can reuse it BUT we have to reset the stream ourselves between each call.
    LZ4_resetStream((LZ4_stream_t *)&state);
    // Since src size is small we know the following enums will be used:  notLimited (0), byU16 (2), noDict (0), noDictIssue (0).
    bytes_returned = LZ4_compress_generic(&state, src, dst, src_size, max_dst_size, notLimited, byU16, noDict, noDictIssue, 1);
    if (bytes_returned < 1)
      run_screaming("Failed to compress src using LZ4_compress_generic.  echo $? for return code.", bytes_returned);
    if (memcmp(dst, known_good_dst, bytes_returned) != 0)
      run_screaming("According to memcmp(), the value we got in dst from LZ4_compress_generic doesn't match the known-good value.  This is bad.", 1);
  */

  /* Benchmarking */
  /* Now we'll run a few rudimentary benchmarks with each function to demonstrate differences in speed based on the function used.
   * Remember, we cannot call LZ4_compress_generic() directly (yet) so it's disabled.
   */
  // Suite A - Normal Compressibility
  char *dst_d = calloc(1, src_size);
  memset(dst, 0, max_dst_size);
  printf("\nStarting suite A:  Normal compressible text.\n");
  uint64_t time_taken__default = bench(known_good_dst, ID__LZ4_COMPRESS_DEFAULT, iterations, src, dst, src_size, max_dst_size, src_comp_size);
  uint64_t time_taken__fast = bench(known_good_dst, ID__LZ4_COMPRESS_FAST, iterations, src, dst, src_size, max_dst_size, src_comp_size);
  uint64_t time_taken__fast_extstate = bench(known_good_dst, ID__LZ4_COMPRESS_FAST_EXTSTATE, iterations, src, dst, src_size, max_dst_size, src_comp_size);
  // uint64_t time_taken__generic       = bench(known_good_dst, ID__LZ4_COMPRESS_GENERIC,       iterations, src,            dst,   src_size, max_dst_size, src_comp_size);
  uint64_t time_taken__decomp_safe = bench(src, ID__LZ4_DECOMPRESS_SAFE, iterations, known_good_dst, dst_d, src_size, max_dst_size, src_comp_size);
  uint64_t time_taken__decomp_fast = bench(src, ID__LZ4_DECOMPRESS_FAST, iterations, known_good_dst, dst_d, src_size, max_dst_size, src_comp_size);
  // Suite B - Highly Compressible
  memset(dst, 0, max_dst_size);
  printf("\nStarting suite B:  Highly compressible text.\n");
  uint64_t time_taken_hc__default = bench(known_good_hc_dst, ID__LZ4_COMPRESS_DEFAULT, iterations, hc_src, dst, src_size, max_dst_size, hc_src_comp_size);
  uint64_t time_taken_hc__fast = bench(known_good_hc_dst, ID__LZ4_COMPRESS_FAST, iterations, hc_src, dst, src_size, max_dst_size, hc_src_comp_size);
  uint64_t time_taken_hc__fast_extstate = bench(known_good_hc_dst, ID__LZ4_COMPRESS_FAST_EXTSTATE, iterations, hc_src, dst, src_size, max_dst_size, hc_src_comp_size);
  // uint64_t time_taken_hc__generic       = bench(known_good_hc_dst, ID__LZ4_COMPRESS_GENERIC,       iterations, hc_src,            dst,   src_size, max_dst_size, hc_src_comp_size);
  uint64_t time_taken_hc__decomp_safe = bench(hc_src, ID__LZ4_DECOMPRESS_SAFE, iterations, known_good_hc_dst, dst_d, src_size, max_dst_size, hc_src_comp_size);
  uint64_t time_taken_hc__decomp_fast = bench(hc_src, ID__LZ4_DECOMPRESS_FAST, iterations, known_good_hc_dst, dst_d, src_size, max_dst_size, hc_src_comp_size);

  // Report and leave.
  setlocale(LC_ALL, "");
  const char *format = "|%-14s|%-30s|%'14.9f|%'16d|%'14d|%'13.2f%%|\n";
  const char *header_format = "|%-14s|%-30s|%14s|%16s|%14s|%14s|\n";
  const char *separator = "+--------------+------------------------------+--------------+----------------+--------------+--------------+\n";
  printf("\n");
  printf("%s", separator);
  printf(header_format, "Source", "Function Benchmarked", "Total Seconds", "Iterations/sec", "ns/Iteration", "% of default");
  printf("%s", separator);
  printf(format, "Normal Text", "LZ4_compress_default()", (double)time_taken__default / BILLION, (int)(iterations / ((double)time_taken__default / BILLION)), (int)time_taken__default / iterations, (double)time_taken__default * 100 / time_taken__default);
  printf(format, "Normal Text", "LZ4_compress_fast()", (double)time_taken__fast / BILLION, (int)(iterations / ((double)time_taken__fast / BILLION)), (int)time_taken__fast / iterations, (double)time_taken__fast * 100 / time_taken__default);
  printf(format, "Normal Text", "LZ4_compress_fast_extState()", (double)time_taken__fast_extstate / BILLION, (int)(iterations / ((double)time_taken__fast_extstate / BILLION)), (int)time_taken__fast_extstate / iterations, (double)time_taken__fast_extstate * 100 / time_taken__default);
  // printf(format, "Normal Text", "LZ4_compress_generic()",       (double)time_taken__generic       / BILLION, (int)(iterations / ((double)time_taken__generic       /BILLION)), (int)time_taken__generic       / iterations, (double)time_taken__generic       * 100 / time_taken__default);
  printf(format, "Normal Text", "LZ4_decompress_safe()", (double)time_taken__decomp_safe / BILLION, (int)(iterations / ((double)time_taken__decomp_safe / BILLION)), (int)time_taken__decomp_safe / iterations, (double)time_taken__decomp_safe * 100 / time_taken__default);
  printf(format, "Normal Text", "LZ4_decompress_fast()", (double)time_taken__decomp_fast / BILLION, (int)(iterations / ((double)time_taken__decomp_fast / BILLION)), (int)time_taken__decomp_fast / iterations, (double)time_taken__decomp_fast * 100 / time_taken__default);
  printf(header_format, "", "", "", "", "", "");
  printf(format, "Compressible", "LZ4_compress_default()", (double)time_taken_hc__default / BILLION, (int)(iterations / ((double)time_taken_hc__default / BILLION)), (int)time_taken_hc__default / iterations, (double)time_taken_hc__default * 100 / time_taken_hc__default);
  printf(format, "Compressible", "LZ4_compress_fast()", (double)time_taken_hc__fast / BILLION, (int)(iterations / ((double)time_taken_hc__fast / BILLION)), (int)time_taken_hc__fast / iterations, (double)time_taken_hc__fast * 100 / time_taken_hc__default);
  printf(format, "Compressible", "LZ4_compress_fast_extState()", (double)time_taken_hc__fast_extstate / BILLION, (int)(iterations / ((double)time_taken_hc__fast_extstate / BILLION)), (int)time_taken_hc__fast_extstate / iterations, (double)time_taken_hc__fast_extstate * 100 / time_taken_hc__default);
  // printf(format, "Compressible", "LZ4_compress_generic()",       (double)time_taken_hc__generic       / BILLION, (int)(iterations / ((double)time_taken_hc__generic       /BILLION)), (int)time_taken_hc__generic       / iterations, (double)time_taken_hc__generic       * 100 / time_taken_hc__default);
  printf(format, "Compressible", "LZ4_decompress_safe()", (double)time_taken_hc__decomp_safe / BILLION, (int)(iterations / ((double)time_taken_hc__decomp_safe / BILLION)), (int)time_taken_hc__decomp_safe / iterations, (double)time_taken_hc__decomp_safe * 100 / time_taken_hc__default);
  printf(format, "Compressible", "LZ4_decompress_fast()", (double)time_taken_hc__decomp_fast / BILLION, (int)(iterations / ((double)time_taken_hc__decomp_fast / BILLION)), (int)time_taken_hc__decomp_fast / iterations, (double)time_taken_hc__decomp_fast * 100 / time_taken_hc__default);
  printf("%s", separator);
  printf("\n");
  printf("All done, ran %d iterations per test.\n", iterations);
  return 0;
}
