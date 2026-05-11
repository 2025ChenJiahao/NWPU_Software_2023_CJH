import random
import string
import io
from PIL import Image, ImageDraw, ImageFont

# 排除易混淆字符
CHARS = 'ABCDEFGHJKLMNPQRSTUVWXYZabcdefghjkmnpqrstuvwxyz23456789'


def generate_captcha(width=160, height=60, length=4):
    """生成验证码图片，返回 (图片字节, 验证码文本)"""
    # 随机字符
    text = ''.join(random.choices(CHARS, k=length))

    # 创建图片
    bg_color = (random.randint(200, 255), random.randint(200, 255), random.randint(200, 255))
    image = Image.new('RGB', (width, height), bg_color)
    draw = ImageDraw.Draw(image)

    # 尝试加载字体
    try:
        font = ImageFont.truetype('arial.ttf', 36)
    except (IOError, OSError):
        try:
            font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 36)
        except (IOError, OSError):
            font = ImageFont.load_default()

    # 绘制字符
    char_width = width // (length + 1)
    for i, char in enumerate(text):
        x = char_width * i + random.randint(5, 15)
        y = random.randint(2, 15)
        color = (random.randint(0, 100), random.randint(0, 100), random.randint(0, 100))
        draw.text((x, y), char, fill=color, font=font)

    # 绘制干扰线
    for _ in range(5):
        x1 = random.randint(0, width)
        y1 = random.randint(0, height)
        x2 = random.randint(0, width)
        y2 = random.randint(0, height)
        color = (random.randint(50, 200), random.randint(50, 200), random.randint(50, 200))
        draw.line([(x1, y1), (x2, y2)], fill=color, width=2)

    # 绘制干扰点
    for _ in range(80):
        x = random.randint(0, width)
        y = random.randint(0, height)
        color = (random.randint(0, 200), random.randint(0, 200), random.randint(0, 200))
        draw.point((x, y), fill=color)

    # 输出为PNG
    buf = io.BytesIO()
    image.save(buf, format='PNG')
    buf.seek(0)

    return buf, text
