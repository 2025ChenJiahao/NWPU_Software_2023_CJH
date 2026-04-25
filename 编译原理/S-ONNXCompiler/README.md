# S-ONNX编译器

一个基于C语言实现的S-ONNX模型编译器，支持词法分析、语法分析、语义分析和三地址码生成。

## 项目介绍

S-ONNX编译器是一个完整的编译器实现，用于处理S-ONNX（Simplified ONNX）模型文件。该编译器实现了以下功能：

- **词法分析**：基于有穷自动机(DFA)的词法分析器
- **语法分析**：递归下降语法分析器，构建抽象语法树(AST)
- **语义分析**：符号表管理、命名冲突检测、未定义使用检测、类型检查
- **中间代码生成**：生成三地址码(TAC)
- **错误处理**：全面的错误报告机制

## 技术栈

- **编程语言**：C语言 (C99标准)
- **构建系统**：GCC + Makefile
- **开发环境**：Windows (MinGW GCC 6.3.0+)

## 项目结构

```
S-ONNXCompiler/
├── include/          # 头文件目录
│   ├── globals.h     # 全局类型定义、枚举、结构体
│   ├── scanner.h     # 词法分析器接口
│   ├── parser.h      # 语法分析器和AST接口
│   ├── symboltable.h # 符号表管理接口
│   ├── semantic.h    # 语义分析接口
│   ├── tac.h        # 三地址码生成器接口
│   └── error.h      # 错误处理接口
├── src/             # 源文件目录
│   ├── scanner.c     # 词法分析器实现 (基于DFA)
│   ├── parser.c      # 递归下降语法分析器和AST
│   ├── symboltable.c # 符号表实现（哈希表）
│   ├── semantic.c    # 语义分析实现
│   ├── tac.c        # 三地址码生成器实现
│   ├── error.c      # 错误处理实现
│   └── main.c       # 主程序
├── test/            # 测试用例
│   ├── test1.txt    # 测试用例1
│   ├── test2.txt    # 测试用例2
│   └── ...          # 共10个测试用例
├── bat/             # 批处理脚本
│   ├── start_cmd_server.bat  # 编译器运行脚本
│   └── start_web_server.bat # Web服务器启动脚本
├── web/             # Web界面文件
│   ├── app.py       # Flask应用
│   ├── wsgi.py      # WSGI入口点
│   ├── requirements.txt # Python依赖项文件
│   └── templates/   # HTML模板
│       └── index.html # 主Web界面
├── obj/             # 目标文件目录
├── bin/             # 可执行文件目录
└── Makefile         # 编译脚本
```

## 安装和编译

### 前提条件

- **GCC编译器**：推荐MinGW GCC 6.3.0或更高版本
- **Make工具**：用于执行Makefile

### 编译步骤

1. **克隆项目**（如果从版本控制中获取）：
   ```bash
   
   git clone https://github.com/2025ChenJiahao/NWPU_Software_2023_CJH/编译原理/S-ONNXCompiler
   cd S-ONNXCompiler
   ```

2. **执行编译**：
   ```bash
   make
   ```

3. **清理编译产物**（可选）：
   ```bash
   make clean
   ```

4. **重新编译**：
   ```bash
   make rebuild
   ```

## 使用方法

### 使用批处理脚本

项目提供了两个批处理脚本来简化使用：

#### 1. 编译器运行脚本 (`start_cmd_server.bat`)
1. 进入 `bat` 目录
2. 双击运行 `start_cmd_server.bat` 文件
3. 按照菜单提示选择操作：
   - 1: 运行测试用例 1
   - 2: 运行所有测试用例
   - 3: 显示帮助信息
   - 4: 退出

#### 2. Web服务器启动脚本 (`start_web_server.bat`)
1. 进入 `bat` 目录
2. 双击运行 `start_web_server.bat` 文件
3. 脚本会安装所需的依赖项（Flask和Waitress）
4. Web服务器将在 http://localhost:5000 上启动
5. 打开浏览器并导航到 http://localhost:5000 使用Web界面
6. 按 Ctrl+C 停止服务器

### Web界面文件
Web界面文件组织在 `web/` 目录中：
- `web/app.py` - 主Flask应用
- `web/wsgi.py` - 生产服务器的WSGI入口点
- `web/requirements.txt` - Python依赖项文件
- `web/templates/index.html` - Web界面HTML模板

### 命令行使用

#### 基本用法

```bash
sonnx-compiler <source_file>
```

#### 命令行选项

| 选项 | 描述 |
|------|------|
| `-o <output_file>` | 指定输出文件（默认：stdout） |
| `-e <echo_file>` | 将源码回显到文件 |
| `-t` | 启用词法分析跟踪 |
| `-p` | 启用语法分析跟踪 |
| `-a` | 启用语义分析跟踪 |
| `-c` | 启用代码生成跟踪 |
| `-h` | 显示帮助信息 |

#### 示例

##### 基本编译
```bash
sonnx-compiler test/test1.txt
```

##### 详细输出
```bash
sonnx-compiler -a -c test/test1.txt
```

##### 输出到文件
```bash
sonnx-compiler -o output.txt test/test1.txt
```

## S-ONNX语法示例

```s-onnx
ir {
    name = "test_model"
    ver = 1
    producer_name = "TestProducer"
    producer_version = "1.0"
    domain = "test.domain"
    model_version = 1
    doc_string = "Simple test model"
    input {
        name = "input1"
        type = tensor_type {
            elem_type = int
            shape {
                dim { dim_value = 1 }
                dim { dim_value = 3 }
                dim { dim_value = 224 }
                dim { dim_value = 224 }
            }
        }
    }
    output {
        name = "output1"
        type = tensor_type {
            elem_type = float
            shape {
                dim { dim_value = 1 }
                dim { dim_value = 1000 }
            }
        }
    }
    opset_import {
        domain = "ai.onnx"
        version = 13
    }
}
```

## 编译流程

1. **词法分析**：将源码分割成token序列
2. **语法分析**：构建抽象语法树(AST)
3. **语义分析**：符号表构建、类型检查
4. **中间代码生成**：生成三地址码

## 测试

项目包含10个测试用例，位于`test/`目录：

- `test1.txt`：基本模型结构
- `test2.txt`：多输入模型
- `test3.txt`：卷积层模型
- `test4.txt`：全连接层模型
- `test5.txt`：池化层模型
- `test6.txt`：批归一化模型
- `test7.txt`：ReLU激活模型
- `test8.txt`：Softmax模型
- `test9.txt`：Concat模型
- `test10.txt`：GEMM模型

执行测试：
```bash
make test
```

## 错误处理

编译器能够检测并报告以下类型的错误：

- **词法错误**：非法字符、未闭合的字符串等
- **语法错误**：缺少必要符号、结构不匹配等
- **语义错误**：命名冲突、未定义引用、类型不匹配等

错误信息包含：
- 错误类型
- 错误位置（行号、列号）
- 详细的错误描述

## 扩展和定制

### 添加新的操作符

1. 在`globals.h`中添加新的token类型
2. 在`scanner.c`的reserved_words数组中添加关键字
3. 在`parser.c`中添加相应的解析函数
4. 在`semantic.c`中添加语义检查
5. 在`tac.c`中添加代码生成逻辑

### 添加新的语法规则

1. 在`parser.h`中定义新的节点类型
2. 在`parser.c`中实现相应的解析函数
3. 更新AST构建逻辑

## 性能考虑

- **词法分析**：基于DFA，线性时间复杂度
- **语法分析**：递归下降，线性时间复杂度
- **符号表**：哈希表实现，平均O(1)查找时间
- **内存使用**：动态内存管理，支持大型模型

## 限制

- 仅支持S-ONNX语法，不支持完整的ONNX规范
- 语义分析主要关注基本的类型检查和符号管理
- 错误恢复机制有限，遇到错误可能会终止编译

## 未来计划

- [ ] 支持完整的ONNX语法
- [ ] 实现代码优化通道
- [ ] 添加更多的中间代码优化
- [ ] 支持目标代码生成（如LLVM IR）
- [ ] 提供图形化AST可视化工具
- [ ] 增加更多的测试用例和边界情况

## 贡献

欢迎提交issue和pull request！

## 许可证

MIT License

## 联系方式

- 项目维护者：Compiler Principles Project Team
- 日期：2026-04-25
