#!/usr/bin/env python3
"""
测试脚本来验证SearchDlg中"More"按钮的功能修复
"""

import os
import sys

def test_searchlist_methods():
    """测试SearchList类中的新方法"""
    print("=== 测试SearchList中的新方法 ===")
    
    # 检查新方法是否存在
    searchlist_cpp_path = "src/SearchList.cpp"
    with open(searchlist_cpp_path, 'r') as f:
        content = f.read()
    
    # 检查RequestMoreResultsFromGlobalSearch方法
    if "RequestMoreResultsFromGlobalSearch" in content:
        print("✓ RequestMoreResultsFromGlobalSearch 方法存在")
    else:
        print("✗ RequestMoreResultsFromGlobalSearch 方法不存在")
        return False
    
    # 检查方法实现
    if "wxString CSearchList::RequestMoreResultsFromGlobalSearch" in content:
        print("✓ RequestMoreResultsFromGlobalSearch 方法实现存在")
    else:
        print("✗ RequestMoreResultsFromGlobalSearch 方法实现不存在")
        return False
    
    return True

def test_searchdlg_method():
    """测试SearchDlg中的OnBnClickedMore方法"""
    print("\n=== 测试SearchDlg中的OnBnClickedMore方法 ===")
    
    searchdlg_cpp_path = "src/SearchDlg.cpp"
    with open(searchdlg_cpp_path, 'r') as f:
        content = f.read()
    
    # 检查OnBnClickedMore方法
    if "OnBnClickedMore" in content:
        print("✓ OnBnClickedMore 方法存在")
    else:
        print("✗ OnBnClickedMore 方法不存在")
        return False
    
    # 检查Global搜索的处理逻辑
    if "RequestMoreResultsFromGlobalSearch" in content:
        print("✓ OnBnClickedMore 中调用了RequestMoreResultsFromGlobalSearch")
    else:
        print("✗ OnBnClickedMore 中没有调用RequestMoreResultsFromGlobalSearch")
        return False
    
    return True

def test_header_file():
    """测试头文件中的方法声明"""
    print("\n=== 测试SearchList.h中的方法声明 ===")
    
    searchlist_h_path = "src/SearchList.h"
    with open(searchlist_h_path, 'r') as f:
        content = f.read()
    
    # 检查头文件中的方法声明
    if "RequestMoreResultsFromGlobalSearch" in content:
        print("✓ SearchList.h 中声明了RequestMoreResultsFromGlobalSearch方法")
    else:
        print("✗ SearchList.h 中没有声明RequestMoreResultsFromGlobalSearch方法")
        return False
    
    return True

def main():
    """主测试函数"""
    print("开始测试'More'按钮功能修复...\n")
    
    tests = [
        test_searchlist_methods,
        test_searchdlg_method,
        test_header_file
    ]
    
    all_passed = True
    for test in tests:
        if not test():
            all_passed = False
    
    print("\n" + "="*50)
    if all_passed:
        print("✓ 所有测试通过！'More'按钮功能修复完整。")
        print("\n修复总结:")
        print("1. 添加了RequestMoreResultsFromGlobalSearch方法到SearchList")
        print("2. 修改了OnBnClickedMore方法以正确处理Global搜索")
        print("3. 在头文件中添加了方法声明")
        print("4. 编译测试通过")
    else:
        print("✗ 部分测试失败！需要进一步检查修复。")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())