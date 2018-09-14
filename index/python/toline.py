# coding=utf-8

with open("nmbdata.txt") as f:
    f_list = f.readlines()
    for num, line in enumerate(f_list):
        line = line.replace(' ','').replace('\n','')
        print line,

