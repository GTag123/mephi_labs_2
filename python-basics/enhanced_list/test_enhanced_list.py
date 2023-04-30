from .enhanced_list import EnhancedList


def test_basic_list_operations():
    lst = EnhancedList()
    for i in range(1, 6):
        lst.append(i)
    assert lst == [1, 2, 3, 4, 5]
    assert len(lst) == 5

    lst.extend([6, 7, 8])
    assert lst == [1, 2, 3, 4, 5, 6, 7, 8]
    assert len(lst) == 8

    lst.remove(5)
    assert lst == [1, 2, 3, 4, 6, 7, 8]
    assert len(lst) == 7

    lst.pop(2)
    assert lst == [1, 2, 4, 6, 7, 8]
    assert len(lst) == 6
    assert 7 in lst
    assert 10 not in lst


def test_first_item():
    lst = EnhancedList([1, 2, 3, 4, 5])
    assert lst.first == 1

    lst[0] = 10
    assert lst.first == 10

    lst.first = 1
    assert lst.first == 1
    assert lst[0] == 1

    lst.pop(0)
    assert lst[0] == 2
    assert lst.first == 2
    assert len(lst) == 4


def test_last_item():
    lst = EnhancedList([1, 2, 3, 4, 5])
    assert lst.last == 5

    lst[-1] = 10
    assert lst.last == 10

    lst.last = 5
    assert lst.last == 5
    assert lst[-1] == 5

    lst.pop(-1)
    assert lst[-1] == 4
    assert lst.last == 4
    assert len(lst) == 4


def test_empty_list():
    lst = EnhancedList([])

    try:
        lst.first = 5
    except IndexError:
        pass
    else:
        assert False

    try:
        lst.last = 5
    except IndexError:
        pass
    else:
        assert False

    assert lst.size == 0
    assert len(lst) == 0


def test_list_with_single_item():
    lst = EnhancedList()
    lst.append(1)

    assert len(lst) == 1
    assert lst.size == 1
    assert lst[0] == lst[-1]
    assert lst.first == lst.last
    assert lst.first == lst[0]
    assert lst.last == lst[-1]
    assert lst[0] == 1


def test_size():
    lst = EnhancedList()

    assert lst.size == 0
    assert len(lst) == 0

    lst.extend([1, 2, 3])
    assert lst.size == 3
    assert len(lst) == lst.size

    lst.size = 5
    assert lst == [1, 2, 3, None, None]
    assert lst.size == 5
    assert len(lst) == lst.size

    lst.size = 2
    assert lst == [1, 2]
    assert lst.size == 2
    assert len(lst) == lst.size

    lst.size = 5
    assert lst == [1, 2, None, None, None]
    assert lst.size == 5
    assert len(lst) == lst.size

    lst.size = 0
    assert lst == []
    assert lst.size == 0
    assert len(lst) == lst.size
