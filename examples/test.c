int swaps = 0;

int fib(int n) {
    if (n < 2) return n;
    return fib(n - 1) + fib(n - 2);
}

void bubble(int a[], int n) {
    for (int i = 0; i < n - 1; i++)
        for (int j = 0; j < n - 1 - i; j++)
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
                swaps++;
            }
}

int main() {
    int data[6] = {5, 2, 9, 1, 7, 3};
    bubble(data, 6);
    for (int i = 0; i < 6; i++) printf("%d ", data[i]);
    printf("\nswaps=%d fib(10)=%d\n", swaps, fib(10));
    int k = 0, s = 0;
    do {
        k++;
        if (k % 2) continue;
        s += k * (k > 4 ? 2 : 1);
    } while (k < 8);
    printf("s=%d -7/2=%d %c\n", s, -7 / 2, 'A' + 1);
    return 0;
}
