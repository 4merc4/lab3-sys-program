import sys

def _div(a, b):
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q

def _mod(a, b):
    return a - _div(a, b) * b

def _add(a, b): return a + b
def _sub(a, b): return a - b
def _mul(a, b): return a * b
def _and(a, b): return a & b
def _or(a, b): return a | b
def _xor(a, b): return a ^ b
def _shl(a, b): return a << b
def _shr(a, b): return a >> b

def _aset(arr, i, v):
    arr[i] = v
    return v

def _aop(arr, i, f, v):
    arr[i] = f(arr[i], v)
    return arr[i]

def _preinc(arr, i, d):
    arr[i] += d
    return arr[i]

def _postinc(arr, i, d):
    arr[i] += d
    return arr[i] - d

def _printf(fmt, *args):
    s = fmt % args
    sys.stdout.write(s)
    return len(s)

swaps = 0

def fib(n):
    global swaps
    if (n < 2):
        return n
    return (fib((n - 1)) + fib((n - 2)))

def bubble(a, n):
    global swaps
    i = 0
    while (i < (n - 1)):
        j = 0
        while (j < ((n - 1) - i)):
            if (a[j] > a[(j + 1)]):
                t = a[j]
                a[j] = a[(j + 1)]
                a[(j + 1)] = t
                swaps += 1
            j += 1
        i += 1

def main():
    global swaps
    data = [5, 2, 9, 1, 7, 3]
    bubble(data, 6)
    i = 0
    while (i < 6):
        _printf("%d ", data[i])
        i += 1
    _printf("\nswaps=%d fib(10)=%d\n", swaps, fib(10))
    k = 0
    s = 0
    while True:
        k += 1
        if _mod(k, 2):
            if not (k < 8):
                break
            continue
        s += (k * (2 if (k > 4) else 1))
        if not (k < 8):
            break
    _printf("s=%d -7/2=%d %c\n", s, _div((-7), 2), (65 + 1))
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
