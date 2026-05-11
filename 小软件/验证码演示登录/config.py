import os

BASE_DIR = os.path.abspath(os.path.dirname(__file__))

SECRET_KEY = 'captcha-demo-secret-key-change-in-production'
DATABASE_PATH = os.path.join(BASE_DIR, 'instance', 'users.db')

# QQ邮箱 SMTP 配置
# 请填入你的QQ邮箱和SMTP授权码（不是QQ密码）
# 授权码获取方式：QQ邮箱 → 设置 → 账户 → 开启SMTP服务 → 生成授权码
SMTP_SERVER = 'smtp.qq.com'
SMTP_PORT = 465
SMTP_USER = '2364602010@qq.com'
SMTP_PASSWORD = 'xmvncuzewtzieaab'  # ← 请填入SMTP授权码
SENDER_EMAIL = '2364602010@qq.com'

# 验证码配置
CAPTCHA_LENGTH = 4
VERIFICATION_CODE_LENGTH = 6
VERIFICATION_CODE_EXPIRY = 300  # 5分钟过期
