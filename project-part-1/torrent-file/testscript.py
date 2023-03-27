R, C = map(int, input().split())
board = [list(input()) for _ in range(R)]

bombs = [[0] * C for _ in range(R)]
for i in range(R):
    for j in range(C):
        if board[i][j].isdigit():
            bombs[i][j] = int(board[i][j])

for i in range(R):
    for j in range(C):
        if bombs[i][j] > 0: # . in Manhattan distance
            for k in range(R):
                for l in range(C):
                    if abs(k-i) + abs(l-j) <= bombs[i][j]:
                        board[k][l] = '.'
for row in board:
    print(''.join(row))
