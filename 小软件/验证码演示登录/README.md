# 验证码演示登录系统 v1.0

一个基于 Flask 的验证码演示登录系统，模拟了 12 个主流平台的登录/注册页面，集成图形验证码和邮箱验证码功能。

## 支持平台

**国内平台 (6)：** 微信、QQ、支付宝、淘宝、抖音、微博

**国际平台 (6)：** Google、GitHub、Apple、Facebook、X (Twitter)、Microsoft

每个平台都有独立的 UI 主题，高度还原真实平台的视觉风格。

## 功能特性

- **图形验证码** — 基于 Pillow 生成，包含干扰线和噪点，点击可刷新
- **邮箱验证码** — 通过 QQ 邮箱 SMTP 发送 6 位数字验证码，5 分钟有效期，60 秒发送间隔限制
- **用户注册** — 用户名 + 邮箱 + 密码 + 邮箱验证码 + 图形验证码
- **用户登录** — 用户名 + 密码 + 图形验证码
- **登录仪表盘** — 登录成功后展示用户信息和登录状态
- **Demo 模式** — 未配置 SMTP 密码时，验证码直接显示在页面上（方便测试）
- **平台主题** — 12 套独立 CSS，还原各平台配色、字体、布局风格

## 项目结构

```
验证码演示登录/
├── app.py                  # 应用工厂 & 启动入口
├── config.py               # 全局配置（SMTP、验证码、数据库）
├── requirements.txt        # Python 依赖
├── start.bat               # Windows 一键启动脚本
├── auth/
│   ├── __init__.py          # 蓝图注册
│   ├── captcha.py           # 图形验证码生成器（Pillow）
│   ├── email_sender.py      # 邮箱验证码发送 & 校验
│   └── routes.py            # 所有路由处理函数
├── database/
│   ├── __init__.py
│   └── db.py                # SQLite 连接管理 & 建表
├── models/
│   ├── __init__.py
│   └── user.py              # 用户 CRUD 操作
├── platforms/
│   ├── __init__.py
│   └── config.py            # 12 个平台的元数据配置
├── instance/
│   └── users.db             # SQLite 数据库文件
├── templates/
│   ├── base.html            # 基础 HTML 模板
│   ├── auth_base.html       # 登录/注册页公共模板
│   ├── index.html           # 首页（平台卡片网格）
│   ├── login.html           # 登录表单
│   ├── register.html        # 注册表单
│   └── dashboard.html       # 登录成功仪表盘
└── static/
    ├── js/
    │   ├── captcha.js        # 点击刷新验证码
    │   └── email_verify.js   # 发送验证码按钮 & 倒计时
    ├── css/
    │   ├── global.css        # 全局样式重置
    │   ├── auth_base.css     # 登录/注册页公共样式
    │   ├── index.css         # 首页样式
    │   ├── dashboard.css     # 仪表盘样式
    │   ├── email_verify.css  # 验证码按钮样式
    │   └── [12 个平台主题 CSS]
    └── img/logos/
        └── [12 个平台 SVG Logo]
```

## 快速开始

### 环境要求

- Python 3.9+
- pip

### 安装与运行

**方式一：一键启动（Windows）**

双击 `start.bat`，自动安装依赖并启动服务。

**方式二：手动启动**

```bash
# 安装依赖
pip install -r requirements.txt

# 启动服务
python app.py
```

启动后访问 http://localhost:5000

### 配置邮箱（可选）

编辑 `config.py`，填入你的 QQ 邮箱和 SMTP 授权码：

```python
SMTP_USER = '你的QQ邮箱@qq.com'
SMTP_PASSWORD = '你的SMTP授权码'    # QQ邮箱 → 设置 → 账户 → 开启SMTP服务 → 生成授权码
SENDER_EMAIL = '你的QQ邮箱@qq.com'
```

如果不配置 SMTP_PASSWORD，系统会自动进入 Demo 模式，验证码直接弹窗显示。

## 路由一览

| 路径 | 方法 | 说明 |
|---|---|---|
| `/` | GET | 首页，展示 12 个平台卡片 |
| `/<platform>/login` | GET/POST | 平台登录页 |
| `/<platform>/register` | GET/POST | 平台注册页 |
| `/api/captcha` | GET | 获取图形验证码图片 |
| `/api/send-code` | POST | 发送邮箱验证码 (JSON API) |
| `/dashboard` | GET | 登录成功仪表盘 |
| `/logout` | GET | 退出登录 |

`<platform>` 可选值：`wechat`、`qq`、`alipay`、`taobao`、`douyin`、`weibo`、`google`、`github`、`apple`、`facebook`、`twitter`、`microsoft`

## 技术栈

- **后端：** Flask 3.x + SQLite3 + Werkzeug（密码哈希）
- **前端：** HTML5 + CSS3（动画、渐变、玻璃态）+ 原生 JavaScript
- **验证码：** Pillow（图形验证码）+ QQ SMTP SSL（邮箱验证码）

## 数据库表结构

```sql
CREATE TABLE users (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    username    TEXT NOT NULL UNIQUE,
    email       TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    platform    TEXT NOT NULL,
    created_at  TEXT NOT NULL DEFAULT (datetime('now', 'localtime')),
    last_login  TEXT
);
```

## 注意事项

- 本项目仅供学习演示使用，请勿用于生产环境
- `config.py` 中包含硬编码的邮箱凭据，如需公开代码请先移除
- 未启用 CSRF 防护和登录频率限制
