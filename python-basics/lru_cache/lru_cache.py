from collections.abc import Callable, Hashable, Iterable
from collections import OrderedDict, namedtuple
from functools import wraps

def lru_cache(max_items: int) -> Callable[[Callable], Callable]:
    """
    Функция создает декоратор, позволяющий кэшировать результаты выполнения обернутой функции по принципу LRU-кэша.
    Размер LRU кэша ограничен количеством max_items. При попытке сохранить новый результат в кэш, в том случае, когда
    размер кэша уже равен max_size, происходит удаление одного из старых элементов, сохраненных в кэше.
    Удаляется тот элемент, к которому обращались давнее всего.
    Также у обернутой функции должен появиться атрибут stats, в котором лежит объект с атрибутами cache_hits и
    cache_misses, подсчитывающие количество успешных и неуспешных использований кэша.
    :param max_items: максимальный размер кэша.
    :return: декоратор, добавляющий LRU-кэширование для обернутой функции.
    """
    def decorator(f):
        return f

    return decorator
