#!/usr/bin/env python3
import sys

def check_brackets(filename):
    with open(filename, 'r') as f:
        content = f.read()

    # Count brackets
    open_braces = content.count('{')
    close_braces = content.count('}')

    print(f"Open braces: {open_braces}")
    print(f"Close braces: {close_braces}")
    print(f"Difference: {open_braces - close_braces}")

    if open_braces != close_braces:
        print("ERROR: Brackets are not balanced!")
        return False
    else:
        print("OK: Brackets are balanced!")
        return True

if __name__ == "__main__":
    if len(sys.argv) > 1:
        check_brackets(sys.argv[1])
    else:
        check_brackets("SearchList.cpp")
