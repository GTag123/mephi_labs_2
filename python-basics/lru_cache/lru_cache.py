from collections.abc import Callable, Hashable, Iterable
from collections import OrderedDict, namedtuple
from functools import wraps


class StatStruct:
    cache_hits = 0
    cache_misses = 0
def lru_cache(max_items: int) -> Callable:
    cache = OrderedDict()
    stats = StatStruct()
    def decorator(f):
        @wraps(f)
        def wrapper(*args, **kwargs):
            key = (args, tuple(sorted(kwargs.items())))
            if key in cache:
                stats.cache_hits += 1
                cache.move_to_end(key)
            else:
                stats.cache_misses += 1
                result = f(*args, **kwargs)
                cache[key] = result
                if len(cache) > max_items:
                    cache.popitem(last=False)
                return result

        wrapper.cache = cache
        wrapper.stats = stats
        return wrapper

    return decorator
