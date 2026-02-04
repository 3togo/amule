# 手动编辑指南 - 添加ED2K搜索日志

## 目标
替换`ED2KSearchController::executeSearch`方法，添加详细的日志输出。

## 步骤

### 1. 打开文件
```
/home/eli/git/amule/src/search/ED2KSearchController.cpp
```

### 2. 找到executeSearch方法
这个方法从第111行开始，到第194行结束。

### 3. 替换整个方法

**删除**第111-194行的所有内容（包括这两行）。

**复制**`/home/eli/git/amule/src/search/ED2KSearchController_executeSearch_new.cpp`文件中的所有内容。

**粘贴**到刚才删除的位置。

### 4. 保存文件

### 5. 重新编译
```bash
cd /home/eli/git/amule/build
make -j$(nproc)
```

### 6. 运行并测试
运行aMule，执行ED2K搜索（Local或Global）。

### 7. 查看日志
你应该能看到类似这样的日志：
```
ED2KSearchController: executeSearch called
ED2KSearchController: oldSearchType=0
ED2KSearchController: isLocalSearch=1
ED2KSearchController: supports64bit=1
ED2KSearchController: CreateSearchPacket success=1, packetSize=19
ED2KSearchController: searchId=1
ED2KSearchController: Sending packet to server, isLocalSearch=1
ED2KSearchController: Search completed, searchId=1
```

## 关键日志说明

- `executeSearch called` - 方法被调用
- `oldSearchType=0` - 0=GlobalSearch, 1=LocalSearch
- `isLocalSearch=1` - 是否为本地搜索
- `supports64bit=1` - 服务器是否支持64位文件大小
- `CreateSearchPacket success=1` - 搜索包创建是否成功
- `searchId=1` - 搜索ID
- `Sending packet to server` - 正在发送包到服务器
- `Search completed` - 搜索完成

## 故障排查

### 如果看不到任何日志
→ `executeSearch`方法没有被调用
→ 检查`startSearch`方法是否正常

### 如果看到`executeSearch called`但看不到`Sending packet to server`
→ 检查：
1. `CreateSearchPacket`是否成功
2. 服务器是否连接
3. 是否有异常发生

### 如果看到`Sending packet to server`
→ 包已发送，问题可能在：
1. 服务器响应
2. 结果处理
3. 防火墙设置

## 下一步

添加日志后，重新运行并分享新的日志输出，我们就能准确定位问题了！
