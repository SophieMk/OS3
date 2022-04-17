#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //инициализация mutex по умолчанию, статика
int n_threads_creatable;

typedef struct quicksort_starter {
  int * arr; //массив
  int low; //номер первого элемента
  int high; //номер последнего элемента
}
quicksort_parameters;

void swap(int * a, int * b) { //фунция, меняющая значения элементов местами
  int t = * a;
  * a = * b;
  * b = t;
}

int partition(int * arr, int low, int high, int pivot) { //операция по разделению 
  int pivotValue = arr[pivot];
  swap( & arr[pivot], & arr[high]); // меняем местами опорный и последний элементы
  int s = low; // индекс наименьшего элемента
  for (int i = low; i < high; i++) {
    // И если текущий элемент не больше, чем опорный, то меняем местами текущий элемент и тот, который меньше
    if (arr[i] <= pivotValue) {
      swap( & arr[i], & arr[s]);
      s++;
    }
  }
  swap( & arr[s], & arr[high]);
  return s;
}

void quicksort_nonparallel(int * arr, int low, int high) { //непарраллельная сортировка
  if (low < high) {
    int pivotPosition = low + (high - low) / 2;
    pivotPosition = partition(arr, low, high, pivotPosition);
    quicksort_nonparallel(arr, low, pivotPosition - 1);
    quicksort_nonparallel(arr, pivotPosition + 1, high);
  }
}

void quicksort_parallel(int * arr, int low, int high);

void * quicksort_parallel_runner(void * initialValues) {
  quicksort_parameters * parameters = initialValues;
  quicksort_parallel(
    parameters -> arr, parameters -> low, parameters -> high
  );
  return NULL;
}

void quicksort_parallel(int * arr, int low, int high) {
  // Сортируем arr[low]..arr[high].
  if (low >= high) { //если нечего сортировать, выходим
    return;
  }

  int pivotPos = low + (high - low) / 2; //находим номер среднего элемента
  pivotPos = partition(arr, low, high, pivotPos); //выполняем операцию по разделению
  // Теперь все элементы в arr[low]..arr[pivotPos-1] не превышают все элементы
  // в arr[pivotPos]..arr[high], поэтому каждую из этих двух частей можно
  // отсортировать независимо.

  //необходимо защититься от случая, когда переменная n_threads_creatable одновременно в разных потоках модицируется
  //это можно сделать с помощью mutex

  pthread_mutex_lock(&mutex);
  char is_ok = n_threads_creatable > 0; 
  if (is_ok) {
    n_threads_creatable--;
  }
  pthread_mutex_unlock(&mutex);

  if (! is_ok) { //если потоков 0, дальше сортируем и выходим
    // Больше потоков не создать, так что продолжаем сортировку обеих частей
    // в текущем потоке.
    quicksort_nonparallel(arr, low, pivotPos - 1);
    quicksort_nonparallel(arr, pivotPos + 1, high);
    return;
  }
  
  //если нельзя создать, делаем это в том же потоке
  //если можно, создаём новый поток и там вызываем сортировку для левой части, 
  //а правую часть сортируем здесь и ждём, когда сортировка левой части закончится

  // Сортируем левую часть в новом потоке:

  quicksort_parameters thread_param = {
    arr,
    low, //номер первого элемента 
    pivotPos - 1 //номер крайнего левого от середины элемента
  };
  pthread_t thread;
  pthread_create( & thread, NULL, quicksort_parallel_runner, & thread_param);

  // Пока левая часть сортируется, сортируем правую в текущем потоке:
  quicksort_parallel(arr, pivotPos + 1, high);

  // Когда правая часть отсортируется, дожидаемся сортировки левой части
  // (если она ещё не завершена).
  pthread_join(thread, NULL);
}

int main(int argc, char ** argv) {
  if (argc != 2) { //если аргуметов не 2, выводим сообщение об ошибке
    printf("Usage: %s n_threads_creatable\n", argv[0]);
    return 1;
  }

  n_threads_creatable = strtol(argv[1], NULL, 10); //строка в long

  int size; 
  scanf("%d", & size); //считываем, сколько элементов

  int * elements = malloc(size * sizeof(int)); //выделяем память на эти элементы

  for (int i = 0; i < size; i++) { //считываем все эти элементы
    scanf("%d", & elements[i]);
  }

  quicksort_parallel(elements, 0, size - 1); //вызываем параллельную быструю сортировку

  for (int i = 0; i < size; i++) { //выводим отсортированнный массив элеметов
    printf("%d\n", elements[i]);
  }

  free(elements); //освобождаем место в памяти
}
