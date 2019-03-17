#include <iostream>
#include <vector>
#include <chrono>
#include <numeric>
#include <thread>
#include <stdlib.h>
#include <cmath>

using namespace std;

//-------------------------------------------
struct CThreadData
{
  CThreadData ( int begin, int end )
  : m_Begin ( begin ), m_End ( end ), m_Result ( 0 )
  {
  }
  void sumOfValues ( void )
  {
    for ( unsigned long long i = m_Begin; i < m_End; i++ )
      //m_Result += ( sqrt( i + 1 ) + i ) / (pow(2, i) - i);
      m_Result += ( sqrt( i + 1 ) + i ) / (sqrt( i * i + 2 * i + 1));
      //m_Result += sin(1/(i+1)) * tan (sqrt(i+1));
  }
  unsigned long long    m_Begin;
  unsigned long long    m_End;
  double                m_Result;
};

//-------------------------------------------
int main ( int argc, char * argv [] )
{
  unsigned long long    valuesNum;
  int          		threadsNum;
  vector<thread>        threads;

  // Check arguments 
  if ( argc != 3 
       || sscanf ( argv[1], "%llu", &valuesNum )  != 1 
       || valuesNum <= 0
       || sscanf ( argv[2], "%d",   &threadsNum ) != 1 
       || threadsNum <= 0 )
  {
    printf ( "Usage: %s <number of values> <number of threads>\n", argv[0] );
    return 1;
  }
 
  // Allocate and initialize threadData
  vector<CThreadData> threadData;
  unsigned long long   position        = 0;
  const int                  remainder       = (int) valuesNum % threadsNum;
  const unsigned long long   numberPerThread = valuesNum / threadsNum;
  
  for ( int i = 0; i < threadsNum; i ++ ) 
  {
    threadData . emplace_back ( position, position + numberPerThread + (i < remainder) );
    position  += numberPerThread + (i < remainder);
  }

  printf("Main: Start of calculation.\n");

  // Remember the current time
  auto start = chrono::system_clock::now();

  // Create threads
  for ( int i = 0; i < threadsNum; i++ )
    threads.emplace_back( &CThreadData::sumOfValues, &threadData[i] );

  // Wait for threads
  for ( auto & x : threads )
    x . join ();
  double sum = accumulate ( threadData . begin (), threadData . end (), 0.0, 
    [] ( double x, const CThreadData & y )
    {
      return x + y . m_Result;
    });

  // Calculate the duration of calculation 
  chrono::duration<double> durationOfSum = chrono::system_clock::now() - start;

  printf("Main: number of values = %llu num of thread = %d sum = %f  duration = %f [s]\n", valuesNum, threadsNum, sum, durationOfSum.count());
}


