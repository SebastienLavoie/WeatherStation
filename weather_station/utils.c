/*
 * Utility functions
 * 
 */

void quicksort(float* tab, int L, int R);
int pivot_split(float* tab, int L, int R);


void quicksort(float* tab, int L, int R){
    if(L < R){
    int P = pivot_split(tab, L, R);
    quicksort(tab, L, P-1);
    quicksort(tab, P+1, R);
  }
}

int pivot_split(float* tab, int L, int R){
  int x = tab[R];
  int i = L-1;
  int tmp;
  
  for(int j=L; j <= R-1; j++){
    if (tab[j] < x){
      i++;
      tmp = tab[i];
      tab[i] = tab[j];
      tab[j] = tmp;
    }
  }
  tmp = tab[i+1];
  tab[i+1] = tab[R];
  tab[R] = tmp;

  return i+1;
} 
