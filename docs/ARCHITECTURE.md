# 渲染与游戏循环说明

这份文档解释「为什么这样分层」，以及几处容易踩坑的实现细节。

## 1. 主循环：固定步长 + 渲染解耦

```text
每帧：
  frameTime = 真实经过时间（QueryPerformanceCounter，钳到 0.25s 以内）
  input.poll()
  accumulator += frameTime
  while (accumulator >= 1/60 && steps < 4):
      updateFrame(1/60, input)      // 第一帧之后清掉 pressed 边沿，避免连触发
      accumulator -= 1/60
  如果跑满 4 步：accumulator = 0      // 掉帧时不追帧，避免死亡螺旋
  renderFrame()
  如果本帧还有余量：Sleep 到 1/120 帧预算       // 别把 CPU 吃满
```

好处：物理/判定/计时永远按 1/60 秒推进，和显示器刷新率无关；
画面帧率变化只影响"看起来顺不顺"，不影响"打没打中"。

## 2. 渲染：为什么不用 `putimage`

EasyX 的 `putimage` 只做 SRCCOPY，没有 alpha 混合，也没有加法混合。
原版直接 `putimage` 的结果是每个精灵都带着一块浅灰底色（素材背景是 `#C3C3C3`）。

所以这里自己写了一层：

```text
PNG --loadimage--> EasyX IMAGE --GetImageBuffer--> Sprite（0xAARRGGBB 数组）
                                                      │
                                     抠图 / 缩放 / 旋转 / 镜像 / 染色 / 描边
                                                      │
Canvas::blit / blitAdd / drawSprite  ──►  直接写显示缓冲（GetImageBuffer(NULL)）
```

* `Sprite::load()` 会从图片**四条边框**统计出最常见的两种颜色当作底色，
  再从边框做洪水填充，只挖掉与边框连通的区域 —— 于是"白底道具图"和"灰底角色图"能同一套代码处理。
* `Sprite::scaledNearest()` 做整数倍最近邻缩放：像素画放大只有这两种选择，模糊比锯齿更丑。
* `Sprite::rotated()` 是反向采样（目标像素 → 源像素），保留 alpha，所以旋转后没有黑边。
* `Sprite::outline()` 给精灵加一圈暗描边。**这一步对可读性影响极大**：
  16bit 像素小人放在噪点砖墙背景上，不加描边基本看不清。
* `Canvas::drawSprite()` 是"缩放 + 染色 + 透明度 + 混合模式"的通用版本，glow、粒子、道具图标都走它。

## 3. 文字：GDI 出字形，自己出效果

普通文字直接用 EasyX 的 `outtextxy`（GDI 渲染，中文抗锯齿质量比手写好得多），
用 `settextstyle` + 缓存避免每帧重建字体。

标题/结算界面的**霓虹字**则多一步：

```text
离屏 IMAGE 上画白字（黑底） → 逐像素取亮度当 alpha → 得到"文字蒙版" Sprite
蒙版 --drawSprite(tint=强调色, scale=1.10, additive)--> 光晕
蒙版 --drawSprite(tint=强调色, scale=1.26, additive)--> 更大的光晕
蒙版 --drawSprite(tint=强调色, scale=1.00)-----------> 字身
```

> 坑：`SetWorkingImage(&离屏IMAGE)` 之后，EasyX 会套用该图像自己的默认绘图样式，
> 所以字体/颜色/背景模式必须在切过去之后重新设置，不能依赖"上一次设过"的缓存。

## 4. 模拟与表现分离

`World` 不知道窗口、不知道像素，只认 `InputState`（纯数据结构）。
因此可以：

* `--selftest`：不开窗口直接跑几千帧，用机器人喂输入，检查不变量；
* `--capture`：开窗口按脚本跑，把关键时刻 `getimage` 存成 PNG。

机器人（`Game::makeBotInput`）用的也是同一套 `InputState`：
瞄准最近敌人（带提前量）、远离威胁、被逼到墙角时反向施压、有闲时去捡道具、被贴脸就冲刺、怪多就丢雷。

## 5. 音频：MCI + 程序化合成

没有额外音效素材：`Audio.cpp` 里有一个极简合成器（正弦/方波/锯齿/三角/噪声 + 包络 + 软削波），
启动时把 13 个音效算成 44.1kHz 单声道 WAV 写到临时目录，再用 MCI 每个音效开两条通道播放
（同一种音效可重叠两次）。

> 坑：MCI 的 `mciSendString` 是同步调用，密集调用会明显拖慢主循环
> （实测一帧能吃掉十几毫秒），所以 `Audio::play` 里对同一音效做了 28ms 限流。
> 另外 `waveaudio` 通道不支持 `setaudio volume`，音量是在合成阶段写进波形的。

## 6. 战场与 HUD 的边界

窗口 800×800，但可行走区域是内缩的 `Rect(14, 96, 772, 592)`：

* 上方留给血量/波次/分数，下方留给弹药/连击/强化；
* 玩家永远不可能被 HUD 面板挡住；
* 战场边框（含四角括号）随震屏一起偏移，抖动时不会出现"地板不动角色乱晃"的割裂感。
