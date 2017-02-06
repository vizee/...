# envext
notepad++ plugin for custom environment variables

## 使用说明
### 安装
复制到 notepad++ 的 plugin 目录下，重启 notepad++

### 配置
在 envext.dll 同目录下创建 envext.ini，在 envext 被加载时会读取 envext.ini 的 envext 节
例如：
```ini
[envext]
a=1
b=%a%;2
```

### 注意
按照 notepad++ 的尿性，会根据插件文件名**字典序**加载，也就是在 envext.dll 并**不能影响**它的文件名字典序之前的插件。改成 a.dll 也许是个解决办法。