import time
from functools import wraps
from flask import render_template, request, redirect, url_for, session, flash, jsonify, send_file, abort
from werkzeug.security import generate_password_hash, check_password_hash
from auth import bp
from platforms.config import PLATFORMS
from models.user import create_user, get_user_by_username, username_exists, email_exists, update_last_login
from auth.captcha import generate_captcha
from auth.email_sender import generate_verification_code, send_verification_code, store_code_in_session, verify_code
import config


def platform_required(f):
    """验证平台slug是否有效"""
    @wraps(f)
    def decorated(platform, *args, **kwargs):
        if platform not in PLATFORMS:
            abort(404)
        return f(platform, *args, **kwargs)
    return decorated


# ============ 主页 ============

@bp.route('/')
def index():
    domestic = {k: v for k, v in PLATFORMS.items() if v['category'] == 'domestic'}
    international = {k: v for k, v in PLATFORMS.items() if v['category'] == 'international'}
    return render_template('index.html', domestic=domestic, international=international, platforms=PLATFORMS)


# ============ 登录 ============

@bp.route('/<platform>/login')
@platform_required
def login_page(platform):
    pc = PLATFORMS[platform]
    return render_template('login.html',
                           platform_slug=platform,
                           platform_config=pc,
                           page_type='登录',
                           title=pc['login_title'])


@bp.route('/<platform>/login', methods=['POST'])
@platform_required
def login_action(platform):
    pc = PLATFORMS[platform]
    username = request.form.get('username', '').strip()
    password = request.form.get('password', '').strip()
    captcha = request.form.get('captcha', '').strip().lower()

    # 验证码校验
    if captcha != session.get('captcha_text', ''):
        flash('验证码错误', 'error')
        return redirect(url_for('auth.login_page', platform=platform))

    # 用户名校验
    if not username:
        flash('请输入用户名', 'error')
        return redirect(url_for('auth.login_page', platform=platform))

    # 密码校验
    user = get_user_by_username(username)
    if not user or not check_password_hash(user['password_hash'], password):
        flash('用户名或密码错误', 'error')
        return redirect(url_for('auth.login_page', platform=platform))

    # 登录成功
    session['user_id'] = user['id']
    session['username'] = user['username']
    session['platform'] = platform
    update_last_login(user['id'])
    return redirect(url_for('auth.dashboard'))


# ============ 注册 ============

@bp.route('/<platform>/register')
@platform_required
def register_page(platform):
    pc = PLATFORMS[platform]
    return render_template('register.html',
                           platform_slug=platform,
                           platform_config=pc,
                           page_type='注册',
                           title=pc['register_title'])


@bp.route('/<platform>/register', methods=['POST'])
@platform_required
def register_action(platform):
    pc = PLATFORMS[platform]
    username = request.form.get('username', '').strip()
    email = request.form.get('email', '').strip()
    password = request.form.get('password', '').strip()
    confirm_password = request.form.get('confirm_password', '').strip()
    captcha = request.form.get('captcha', '').strip().lower()
    vcode = request.form.get('verification_code', '').strip()

    # 验证码校验
    if captcha != session.get('captcha_text', ''):
        flash('验证码错误', 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    # 邮箱验证码校验
    valid, msg = verify_code(vcode)
    if not valid:
        flash(msg, 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    # 表单校验
    if not username or not email or not password:
        flash('请填写所有必填项', 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    if password != confirm_password:
        flash('两次密码不一致', 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    if len(password) < 6:
        flash('密码至少6位', 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    if username_exists(username):
        flash('用户名已存在', 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    if email_exists(email):
        flash('邮箱已被注册', 'error')
        return redirect(url_for('auth.register_page', platform=platform))

    # 创建用户
    password_hash = generate_password_hash(password)
    create_user(username, email, password_hash, platform)

    flash('注册成功，请登录', 'success')
    return redirect(url_for('auth.login_page', platform=platform))


# ============ 验证码API ============

@bp.route('/api/captcha')
def get_captcha():
    image_bytes, text = generate_captcha()
    session['captcha_text'] = text.lower()
    session['captcha_time'] = time.time()
    return send_file(image_bytes, mimetype='image/png')


# ============ 邮箱验证码API ============

@bp.route('/api/send-code', methods=['POST'])
def send_code():
    data = request.get_json()
    email = data.get('email', '').strip()

    if not email:
        return jsonify({'ok': False, 'error': '请输入邮箱'}), 400

    # 频率限制：60秒
    last_time = session.get('last_send_time', 0)
    if time.time() - last_time < 60:
        remaining = int(60 - (time.time() - last_time))
        return jsonify({'ok': False, 'error': f'{remaining}秒后才能重新发送'}), 429

    code = generate_verification_code(config.VERIFICATION_CODE_LENGTH)

    if config.SMTP_PASSWORD:
        success, msg = send_verification_code(email, code)
        if success:
            store_code_in_session(code)
            session['last_send_time'] = time.time()
            return jsonify({'ok': True, 'message': '验证码已发送到邮箱'})
        else:
            return jsonify({'ok': False, 'error': f'发送失败: {msg}'}), 500
    else:
        # SMTP未配置，直接显示验证码（演示模式）
        store_code_in_session(code)
        session['last_send_time'] = time.time()
        return jsonify({'ok': True, 'demo': True, 'code': code, 'message': f'演示模式 - 验证码: {code}'})


# ============ Dashboard ============

@bp.route('/dashboard')
def dashboard():
    if 'user_id' not in session:
        return redirect(url_for('auth.index'))
    return render_template('dashboard.html',
                           username=session.get('username'),
                           platform=session.get('platform'),
                           platform_config=PLATFORMS.get(session.get('platform', ''), {}))


# ============ 退出 ============

@bp.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('auth.index'))
