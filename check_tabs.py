#!/usr/bin/env python3
with open('/home/eli/git/amule/src/SearchDlg.cpp', 'r') as f:
    lines = f.readlines()
    for i in range(798, 809):
        print(f'Line {i+1}: {repr(lines[i])}')
