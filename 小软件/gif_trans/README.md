# 视频转GIF工具

一个纯前端的视频转GIF工具，使用浏览器原生视频解码 + gif.js 编码，瞬间启动。

## 功能特点

- ✅ 拖拽上传视频
- ✅ 支持多种格式 (mp4, webm, mov 等浏览器可播放的格式)
- ✅ 设置帧率 (1-30 FPS)
- ✅ 设置尺寸 (宽度/高度)
- ✅ 质量调节 (低/中/高)
- ✅ 时间裁剪 (选择起止时间)
- ✅ 批量转换
- ✅ 实时预览
- ✅ 一键下载

## 项目结构

```
gif_trans/
├── video-to-gif.html    # 主页面
├── gifjs/               # GIF编码库（~30KB）
│   ├── gif.js
│   └── gif.worker.js
├── start.bat            # 启动脚本（可选）
├── index.html           # 默认首页
└── README.md
```

## 使用方法

**方式一（推荐）：** 双击 `start.bat` 启动本地服务器，自动打开浏览器

**方式二：** 直接双击 `video-to-gif.html` 在浏览器中打开

**方式三：** 运行 `python -m http.server 8080`，访问 `http://localhost:8080`

## 技术栈

- 浏览器原生 `<video>` 解码 - 无需额外引擎
- gif.js - 轻量 GIF 编码器（Web Worker）
- 原生 JavaScript - 无框架依赖

## 注意事项

- 瞬间启动，无需等待引擎加载
- 支持浏览器能播放的视频格式（mp4、webm 等）
- 所有处理均在浏览器本地完成，视频不会上传到服务器
