# coding=utf-8
with open("test") as f:
    f_line = f.readline()
    f_list = f_line.split('\3')
    for i in range(0, len(f_list), 3):
        print f_list[i]+'\3',
        try:
            print f_list[i+1]+'\3',
            print f_list[i+2]
        except:
            break
