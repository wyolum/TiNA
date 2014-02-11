f = open("ascii_5x7.wff")

def getchar(c):
    f.seek(6 * ord(c))
    l = f.read(1)
    lines = [
        [' ', ' ',' ', ' ',' ', ' ',' '],
        [' ', ' ',' ', ' ',' ', ' ',' '],
        [' ', ' ',' ', ' ',' ', ' ',' '],
        [' ', ' ',' ', ' ',' ', ' ',' '],
        [' ', ' ',' ', ' ',' ', ' ',' ']]

    for i in range(5):
        data = ord(f.read(1))
        for j in range(7):
            if (data >> j) & 1:
                lines[i][j] = '*'
    for l in lines[::-1]:
        print ''.join(l)

for i in range(100):
    c = chr(ord(']') + i)
    print 'char', c
    getchar(c)
    print
    print
        
