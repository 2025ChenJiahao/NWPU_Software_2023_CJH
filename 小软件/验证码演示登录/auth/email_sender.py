import random
import smtplib
import time
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from flask import session
import config


def generate_verification_code(length=6):
    """生成随机数字验证码"""
    return ''.join(random.choices('0123456789', k=length))


def send_verification_code(email, code):
    """通过QQ邮箱SMTP发送验证码"""
    if not config.SMTP_PASSWORD:
        return False, 'SMTP未配置'

    msg = MIMEMultipart()
    msg['From'] = config.SENDER_EMAIL
    msg['To'] = email
    msg['Subject'] = '验证码 - 演示登录系统'

    body = f'''
    <html>
    <body style="font-family: Arial, sans-serif; padding: 20px;">
        <h2 style="color: #333;">验证码</h2>
        <p>您正在注册演示登录系统，您的验证码是：</p>
        <div style="font-size: 32px; font-weight: bold; color: #1677FF;
                    background: #F0F8FF; padding: 15px 30px;
                    border-radius: 8px; display: inline-block;
                    letter-spacing: 8px;">
            {code}
        </div>
        <p style="color: #666; margin-top: 20px;">验证码5分钟内有效，请勿泄露给他人。</p>
        <hr style="border: none; border-top: 1px solid #eee; margin: 20px 0;">
        <p style="color: #999; font-size: 12px;">此邮件由演示登录系统自动发送，请勿回复。</p>
    </body>
    </html>
    '''
    msg.attach(MIMEText(body, 'html', 'utf-8'))

    try:
        server = smtplib.SMTP_SSL(config.SMTP_SERVER, config.SMTP_PORT)
        server.login(config.SMTP_USER, config.SMTP_PASSWORD)
        server.sendmail(config.SENDER_EMAIL, email, msg.as_string())
        server.quit()
        return True, '发送成功'
    except Exception as e:
        return False, str(e)


def store_code_in_session(code):
    """将验证码存入session"""
    session['email_code'] = code
    session['email_code_time'] = time.time()
    session['email_code_expires'] = time.time() + config.VERIFICATION_CODE_EXPIRY


def verify_code(submitted_code):
    """验证邮箱验证码，返回 (是否有效, 错误信息)"""
    if time.time() > session.get('email_code_expires', 0):
        return False, '验证码已过期，请重新发送'
    if submitted_code != session.get('email_code', ''):
        return False, '验证码错误'
    return True, ''
