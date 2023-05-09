from collections.abc import Iterable, Generator


def flatten(iterable: Iterable) -> Generator:
    for item in iterable:
        if isinstance(item, Iterable) and not isinstance(item, (str, bytes)):
            yield from flatten(item)
        else:
            yield item

[1, [1,2,3], [4,5,6]]