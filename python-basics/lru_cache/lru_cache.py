from collections.abc import Callable, Hashable, Iterable
from collections import OrderedDict, namedtuple
from functools import wraps


class StatStruct:
    cache_hits = 0
    cache_misses = 0
def lru_cache(max_items: int) -> Callable:

    def decorator(f):
        cache = OrderedDict()
        f.stats = StatStruct()
        @wraps(f)
        def wrapper(*args, **kwargs):
            key = str((args, tuple(sorted(kwargs.items()))))
            if key in cache:
                f.stats.cache_hits += 1
                cache.move_to_end(key)
            else:
                f.stats.cache_misses += 1
                result = f(*args, **kwargs)
                cache[key] = result
                if len(cache) > max_items:
                    cache.popitem(last=False)
            return cache[key]
        return wrapper

    return decorator
