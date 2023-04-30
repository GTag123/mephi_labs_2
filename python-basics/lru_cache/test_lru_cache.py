from functools import reduce
from .lru_cache import lru_cache


@lru_cache(20)
def factorial(n):
    if not isinstance(n, int):
        raise ValueError("Factorial of non-integer values is not defined")
    if n < 0:
        raise ValueError("Factorial of negative numbers is not defined")
    if n == 0:
        return 1
    return n * factorial(n - 1)


def test_exception():
    misses_count_before = factorial.stats.cache_misses
    hits_count_before = factorial.stats.cache_hits
    attempts = 3
    for _ in range(attempts):
        try:
            factorial(-10)
        except ValueError:
            pass
    assert factorial.stats.cache_misses == misses_count_before + attempts
    assert factorial.stats.cache_hits == hits_count_before


def test_factorial():
    misses_count_before = factorial.stats.cache_misses
    for i in range(1, 20):
        result = factorial(i)
        expected_result = reduce(lambda x, y: x * y, range(1, i + 1))
        assert result == expected_result
    assert factorial.stats.cache_misses == misses_count_before + 20


def test_complex_arguments():
    @lru_cache(5)
    def foo(bar: list, baz: dict) -> str:
        assert isinstance(bar, list)
        assert isinstance(baz, dict)
        return f"foo({bar}, {baz})"

    result = foo([1, 2, 3], baz={"lol": ("pri", "kol")})
    assert result == "foo([1, 2, 3], {'lol': ('pri', 'kol')})"
    assert foo.stats.cache_misses == 1
    assert foo.stats.cache_hits == 0

    # same arguments again
    result = foo([1, 2, 3], baz={"lol": ("pri", "kol")})
    assert result == "foo([1, 2, 3], {'lol': ('pri', 'kol')})"
    assert foo.stats.cache_misses == 1
    assert foo.stats.cache_hits == 1

    # similar arguments but type changed
    result = foo([1, 2, 3], baz={"lol": ["pri", "kol"]})
    assert result == "foo([1, 2, 3], {'lol': ['pri', 'kol']})"
    assert foo.stats.cache_misses == 2
    assert foo.stats.cache_hits == 1

    # similar arguments but type changed
    try:
        foo((1, 2, 3), baz={"lol": ["pri", "kol"]})
    except AssertionError:
        pass
    else:
        assert False


def test_decorator_preserves_function_properties():
    @lru_cache(1)
    def foo(bar):
        """some docstring"""
        return f"{bar} baz"

    assert foo.__name__ == 'foo'
    assert foo.__doc__ == 'some docstring'
    assert foo.__module__ == __name__


def test_stats():
    real_calls_count = 0
    max_cache_items = 8

    @lru_cache(max_cache_items)
    def foo(bar):
        nonlocal real_calls_count
        real_calls_count += 1
        return bar

    args_of_size = tuple(range(max_cache_items))
    assert tuple(foo(args) for args in args_of_size) == args_of_size
    assert real_calls_count == max_cache_items
    assert foo.stats.cache_misses == real_calls_count
    assert foo.stats.cache_hits == 0

    # run again same arguments
    assert tuple(foo(args) for args in args_of_size) == args_of_size
    assert real_calls_count == max_cache_items
    assert foo.stats.cache_misses == real_calls_count
    assert foo.stats.cache_hits == max_cache_items

    # same arguments + new arguments
    calls_count_before = real_calls_count
    args_of_double_size = args_of_size + tuple(range(max_cache_items, max_cache_items * 2))
    assert tuple(foo(args) for args in args_of_double_size) == args_of_double_size
    assert real_calls_count == calls_count_before + max_cache_items
    assert foo.stats.cache_misses == real_calls_count
    assert foo.stats.cache_hits == 2 * max_cache_items

    # run again same arguments
    calls_count_before = real_calls_count
    assert tuple(foo(args) for args in args_of_size) == args_of_size
    assert real_calls_count == calls_count_before + max_cache_items
    assert foo.stats.cache_misses == real_calls_count
    assert foo.stats.cache_hits == 2 * max_cache_items

