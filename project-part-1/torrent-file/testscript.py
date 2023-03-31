def max_ideal_sculptures(a, X, T):
    cnt = 0
    for i in range(len(a)):
        if abs(a[i] - X) <= 1:  # текущая скульптура близка к идеальной
            cnt += 1
        elif a[i] < X:  # попытаемся увеличить вес скульптуры на 1 килограмм
            if T >= 1:
                cnt += 1
                T -= 1
        else:  # попытаемся уменьшить вес скульптуры на 1 килограмм
            if T >= 2:
                cnt += 1
                T -= 2
    return cnt
a = [2, 3, 4, 7, 10]
X = 7
T = 3
print(max_ideal_sculptures(a, X, T))  # выведет 3

