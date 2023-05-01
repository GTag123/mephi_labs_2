from collections.abc import Iterable, Generator
from collections import namedtuple
from itertools import groupby
from math import isqrt

from .flatten import flatten


def test_list():
    lst = [1, 2, [3, 4, 5], [], 6, [7], 8]
    flat_list_generator = flatten(lst)

    assert isinstance(flat_list_generator, Iterable)
    assert isinstance(flat_list_generator, Generator)

    flat_list = list(flat_list_generator)
    assert flat_list == list(range(1, 9))


def test_dict():
    dct = {1: "one", 2: "two", 3: {4: "four"}}

    assert list(flatten(dct)) == [1, 2, 3]
    assert list(flatten(dct.items())) == [1, "one", 2, "two", 3, 4]
    assert list(flatten(dct.values())) == ["one", "two", 4]


def test_generator():
    values = [1, 5, 6, 2, 14, 8, 10, 4, 3, 0, 9, 13, 12, 18, 20]
    grouped_values = groupby(sorted(values), key=lambda value: f"Greater or equal than {isqrt(value)}²")
    flat_list = list(flatten(grouped_values))
    assert flat_list == [
        "Greater or equal than 0²",
        0,
        "Greater or equal than 1²",
        1, 2, 3,
        "Greater or equal than 2²",
        4, 5, 6, 8,
        "Greater or equal than 3²",
        9, 10, 12, 13, 14,
        "Greater or equal than 4²",
        18, 20,
    ]


def test_tuple():
    T = namedtuple("T", "a,b")
    flat_list = list(flatten((1, 2, (3, 4), ((5,),), T(a=6, b=7))))
    assert flat_list == [1, 2, 3, 4, 5, 6, 7]
