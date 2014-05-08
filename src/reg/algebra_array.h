/*******************************************************************************
 Array Algebra
 
 Defines 

 Programmer: Dima V. Fedorov <mailto:dima@dimin.net> <http://www.dimin.net/>
 Some functions by: Chao Huang 

 History:
   07/14/2001 21:53:31 - First creation
   10/02/2001 19:12:00 - GCC warnings and Linux treatment

 Ver : 2
*******************************************************************************/

#ifndef ALGEBRA_ARRAY_H
#define ALGEBRA_ARRAY_H

template <typename T>
void a_BubbleSort_index(T *g, int beg, int len, int *index);

template <typename T>
int a_sort_index(T *g, int beg, int len, int *index);

template <typename T>
double a_mean(T *g, int beg, int len);

//this function finds the Standard deviation of an matrix
//It normalises by N-1, where N is the number of elements of matrix
template <typename T>
double a_std(T *g, int beg, int len);

template <typename T>
double a_std(T *g, int beg, int len, double avg);

template <typename T>
int a_diff(T *arr1, T *arr2, T *arr3, int beg, int len);

template <typename T>
int a_div(T *arr1, T *arr2, T *arr3, int beg, int len);

template <typename T>
int a_abs(T *in, T *out, int beg, int len);

template <typename T>
double a_norm(T *a, int beg, int len);

template <typename T>
T a_min(T *input, int beg, int len);

template <typename T>
T a_max(T *input, int beg, int len);


//****************************************************************************
// implementations
//****************************************************************************
template <typename T>
inline void swap_values(T &v1, T &v2) {
  T t = v1;
  v1 = v2;
  v2 = t;
}

template <typename T>
void a_BubbleSort_index(T *g, int beg, int len, int *index) {
  for (int i=beg; i<=len; i++) index[i] = i;
  for (int i=len; i>=beg; i--)
    for (int j=beg; j<=len-1; j++)
      if (g[j] > g[j + 1]) {
        swap_values(g[j], g[j+1]);
        swap_values(index[j], index[j+1]);
      }
}

template <typename T>
int a_sort_index(T *g, int beg, int len, int *index) {
  if ((g == NULL) || (index == NULL) )  return 1;
  a_BubbleSort_index(g, beg, len, index);
  return 0;
}

template <typename T>
double a_mean(T *g, int beg, int len) {
   double avg = 0.0;
   for (int i=beg; i<=len; i++) avg += g[i];
   return (avg/(len-beg+1));
}

//this function finds the Standard deviation of an matrix
//It normalises by N-1, where N is the number of elements of matrix
template <typename T>
double a_std(T *g, int beg, int len) {
   double std = 0.0;
   if (len < 2) return std;
   double avg = a_mean(g, beg, len);
   for (int i=beg; i<=len; i++) 
     std += (g[i]-avg)*(g[i]-avg);   
   return (sqrt(std/(len-beg)));
}

template <typename T>
double a_std(T *g, int beg, int len, double avg) {
   double std = 0.0;
   if (len < 2) return std;
   for (int i=beg; i<=len; i++) 
     std += (g[i]-avg)*(g[i]-avg);   
   return (sqrt(std/(len-beg)));
}

template <typename T>
int a_diff(T *arr1, T *arr2, T *arr3, int beg, int len) {
  if (arr1 == NULL || arr2 == NULL || arr3 == NULL) return 1;
  for (int i=beg; i<=len; i++) arr3[i] = arr1[i] - arr2[i]; 
  return 0;
}     

template <typename T>
int a_div(T *arr1, T *arr2, T *arr3, int beg, int len) {
  if (arr1 == NULL || arr2 == NULL || arr3 == NULL) return 1;
  for (int i=beg; i<=len; i++) arr3[i] = arr1[i] / arr2[i]; 
  return 0;
} 

template <typename T>
int a_abs(T *in, T *out, int beg, int len) {
  for (int i=beg; i<=len; i++) out[i] = (T) fabs(in[i]);
  return 0;
}

template <typename T>
double a_norm(T *a, int beg, int len) {
  double sum=0;
  for (int i=beg; i<=len; i++) sum = sum + (a[i]*a[i]);
  return sqrt(sum);
}

template <typename T>
T a_min(T *input, int beg, int len) {
  T output = input[beg];
  for(int i=beg+1; i<=len; i++)
    if (output > input[i]) output = input[i];
  return output;
}

template <typename T>
T a_max(T *input, int beg, int len) {
  T output = input[beg];
  for(int i=beg+1; i<=len; i++)
    if (output < input[i]) output = input[i];
  return output;
}


#endif // ALGEBRA_ARRAY_H

