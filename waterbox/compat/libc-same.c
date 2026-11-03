/* libc-same.c - C library calls whose answers differ between C libraries,
 * answered by the core itself in both builds (the link's --wrap sends every
 * call here), so the native reference and the sandbox compute the same:
 *
 * - qsort and bsearch: glibc's and musl's qsort order equal elements
 *   differently (Flight of the Amazon Queen sorts its sprites with it), and
 *   two bsearch can find different ones of equal keys. Here: a stable merge
 *   sort, and a binary search that finds the first match.
 * - rand and srand: musl's generator (a 64-bit LCG), in both builds (the
 *   Lua of Grim Fandango's engine calls it).
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void merge_sort(char *base, char *tmp, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
	if (n < 2)
		return;
	if (n <= 8) {
		/* insertion sort, stable */
		for (size_t i = 1; i < n; i++) {
			size_t j = i;
			memcpy(tmp, base + i * size, size);
			while (j > 0 && cmp(base + (j - 1) * size, tmp) > 0) {
				memcpy(base + j * size, base + (j - 1) * size, size);
				j--;
			}
			memcpy(base + j * size, tmp, size);
		}
		return;
	}
	const size_t half = n / 2;
	merge_sort(base, tmp, half, size, cmp);
	merge_sort(base + half * size, tmp, n - half, size, cmp);
	size_t i = 0, j = half, k = 0;
	while (i < half && j < n) {
		if (cmp(base + j * size, base + i * size) < 0)
			memcpy(tmp + (k++) * size, base + (j++) * size, size);
		else
			memcpy(tmp + (k++) * size, base + (i++) * size, size);
	}
	while (i < half)
		memcpy(tmp + (k++) * size, base + (i++) * size, size);
	while (j < n)
		memcpy(tmp + (k++) * size, base + (j++) * size, size);
	memcpy(base, tmp, n * size);
}

void __wrap_qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
	if (n < 2 || size == 0)
		return;
	char stackTmp[4096];
	char *tmp = n * size <= sizeof stackTmp ? stackTmp : (char *)malloc(n * size);
	if (!tmp)
		abort();
	merge_sort((char *)base, tmp, n, size, cmp);
	if (tmp != stackTmp)
		free(tmp);
}

void *__wrap_bsearch(const void *key, const void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
	size_t lo = 0, hi = n;
	while (lo < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		if (cmp(key, (const char *)base + mid * size) > 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	if (lo < n && cmp(key, (const char *)base + lo * size) == 0)
		return (char *)base + lo * size;
	return NULL;
}

static uint64_t g_rand_seed;

void __wrap_srand(unsigned s) { g_rand_seed = s - 1; }

int __wrap_rand(void)
{
	g_rand_seed = 6364136223846793005ULL * g_rand_seed + 1;
	return (int)(g_rand_seed >> 33);
}
