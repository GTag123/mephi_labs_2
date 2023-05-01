from collections.abc import Iterable, Generator


def flatten(iterable: Iterable) -> Generator:
    for item in iterable:
        if isinstance(item, Iterable) and not isinstance(item, (str, bytes)):
            yield from flatten(item)
        else:
            yield item